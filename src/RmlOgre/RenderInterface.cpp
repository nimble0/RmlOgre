#include "RenderInterface.hpp"

#include "Compositor/CompositorPassGeometry.hpp"
#include "Compositor/CompositorPassGeometryDef.hpp"
#include "geometry.hpp"
#include "shaders.hpp"

#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Hlms/Unlit/OgreHlmsUnlitDatablock.h>
#include <OgreCamera.h>
#include <OgreHlmsManager.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreRenderQueue.h>
#include <OgreRoot.h>
#include <OgreTextureFilters.h>
#include <OgreTextureGpuManager.h>
#include <Vao/OgreVaoManager.h>
#include <Vao/OgreVertexArrayObject.h>

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>


using namespace nimble::RmlOgre;

RenderInterface::RenderInterface(
	const Ogre::String& name,
	Ogre::SceneManager* sceneManager,
	Ogre::TextureGpu* output,
	Ogre::TextureGpu* background
) :
	hlms{static_cast<Ogre::HlmsUnlit*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT))},
	workspace(name, sceneManager, output, background)
{
	this->macroblock.mScissorTestEnabled = true;
	this->macroblock.mDepthCheck = false;
	this->macroblock.mDepthWrite = false;
	this->macroblock.mCullMode = Ogre::CULL_NONE;

	this->blendblock.mSeparateBlend = false;
	this->blendblock.mBlendOperation = Ogre::SceneBlendOperation::SBO_ADD;
	this->blendblock.mDestBlendFactor = Ogre::SceneBlendFactor::SBF_ONE_MINUS_SOURCE_ALPHA;
	this->blendblock.mSourceBlendFactor = Ogre::SceneBlendFactor::SBF_ONE;

	this->samplerblock.mU = Ogre::TextureAddressingMode::TAM_WRAP;
	this->samplerblock.mV = Ogre::TextureAddressingMode::TAM_WRAP;

	this->noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->getDatablock("NoTexture"));
	if(!this->noTextureDatablock)
		this->noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			this->hlms->createDatablock(
				"NoTexture",
				"NoTexture",
				this->macroblock,
				this->blendblock,
				Ogre::HlmsParamVec())
		);
	this->noTextureDatablock->setUseColour(true);

	this->AddFilterMaker("blur", std::make_unique<BlurFilterMaker>());
	this->AddFilterMaker("drop-shadow", std::make_unique<DropShadowFilterMaker>());
	this->AddFilterMaker("opacity", std::make_unique<OpacityFilterMaker>());
	this->AddFilterMaker("brightness", std::make_unique<BrightnessFilterMaker>());
	this->AddFilterMaker("contrast", std::make_unique<ContrastFilterMaker>());
	this->AddFilterMaker("invert", std::make_unique<InvertFilterMaker>());
	this->AddFilterMaker("grayscale", std::make_unique<GrayscaleFilterMaker>());
	this->AddFilterMaker("sepia", std::make_unique<SepiaFilterMaker>());
	this->AddFilterMaker("hue-rotate", std::make_unique<HueRotateFilterMaker>());
	this->AddFilterMaker("saturate", std::make_unique<SaturateFilterMaker>());
	this->filters.insert({});

	this->AddShaderMaker("linear-gradient", std::make_unique<LinearGradientMaker>());
	this->AddShaderMaker("radial-gradient", std::make_unique<RadialGradientMaker>());
	this->AddShaderMaker("conic-gradient", std::make_unique<ConicGradientMaker>());
	this->shaders.insert({});
}

void RenderInterface::releaseBufferedGeometries()
{
	Ogre::Root& root = Ogre::Root::getSingleton();
	Ogre::RenderSystem* renderSystem = root.getRenderSystem();
	Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

	for(Rml::CompiledGeometryHandle geometry : this->releaseGeometries)
	{
		auto* vao = reinterpret_cast<Ogre::VertexArrayObject*>(geometry);

		for(auto* buffer : vao->getVertexBuffers())
			vaoManager->destroyVertexBuffer(buffer);

		if(vao->getIndexBuffer())
			vaoManager->destroyIndexBuffer(vao->getIndexBuffer());
		vaoManager->destroyVertexArrayObject(vao);
	}
	this->releaseGeometries.clear();
}

void RenderInterface::releaseBufferedTextures()
{
	Ogre::TextureGpuManager& textureManager = *Ogre::Root::getSingleton().getRenderSystem()
		->getTextureGpuManager();

	for(Rml::TextureHandle texture : this->releaseTextures)
	{
		auto* datablock = reinterpret_cast<Ogre::HlmsUnlitDatablock*>(texture);
		auto* textureGpu = datablock->getTexture(0);

		for(auto* r : datablock->getLinkedRenderables())
			r->setDatablock(this->noTextureDatablock);

		this->hlms->destroyDatablock(datablock->getName());
		if(!this->workspace.freeRenderTexture(textureGpu))
			textureManager.destroyTexture(textureGpu);
	}
	this->releaseTextures.clear();

	for(Ogre::TextureGpu* texture : this->releaseRenderTextures)
		this->workspace.freeRenderTexture(texture);
	this->releaseRenderTextures.clear();
}


void RenderInterface::BeginFrame()
{
	this->workspace.clearAll();
	this->releaseBufferedGeometries();
	this->releaseBufferedTextures();
}

void RenderInterface::EndFrame()
{
	this->workspace.populateWorkspace(this->passes);

	this->passes.clear();
	this->layerBuffers.clear();
	this->renderPassSettings = RenderPassSettings{};
	this->connectionId = 0;
}


void RenderInterface::AddFilterMaker(Rml::String name, std::unique_ptr<FilterMaker> filterMaker)
{
	this->filterMakers.emplace(std::move(name), std::move(filterMaker));
}
void RenderInterface::AddShaderMaker(Rml::String name, std::unique_ptr<ShaderMaker> shaderMaker)
{
	this->shaderMakers.emplace(std::move(name), std::move(shaderMaker));
}


void RenderInterface::addPass(Pass&& pass)
{
	this->passes.push_back(std::move(pass));
}
void RenderInterface::releaseRenderTexture(Ogre::TextureGpu* texture)
{
	this->releaseRenderTextures.push_back(texture);
}


Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(
	Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices)
{
	return reinterpret_cast<Rml::CompiledGeometryHandle>(create_vao(vertices, indices));
}
void RenderInterface::RenderGeometry(
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation,
	Rml::TextureHandle texture)
{
	std::vector<QueuedGeometry>* queue = nullptr;
	if(this->renderPassSettings.enableStencil)
		queue = &this->getRenderPass<RenderWithStencilPass>().queue;
	else
		queue = &this->getRenderPass<RenderPass>().queue;

	queue->push_back({
		reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
		translation,
		texture ? reinterpret_cast<Ogre::HlmsUnlitDatablock*>(texture) : this->noTextureDatablock,
		nullptr
	});
}
void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	this->releaseGeometries.push_back(geometry);
}


Rml::TextureHandle RenderInterface::LoadTexture(
	Rml::Vector2i& texture_dimensions,
	const Rml::String& source)
{
	Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingleton()
		.getRenderSystem()
		->getTextureGpuManager();
	Ogre::TextureGpu* texture = nullptr;
	if(!source.empty())
	{
		texture = textureManager->createTexture(
			source,
			Ogre::GpuPageOutStrategy::Discard,
			Ogre::TextureFlags::AutomaticBatching,
			Ogre::TextureTypes::Type2D,
			Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
			Ogre::TextureFilter::TypePremultiplyAlpha);
		texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
	}

	if(!texture)
		return Rml::TextureHandle{};

	texture->waitForMetadata();
	texture_dimensions = Rml::Vector2i(texture->getWidth(), texture->getHeight());

	Ogre::String id = this->workspace.getNameStr();
	id.append("_Texture_");
	id.append(std::to_string(this->datablockId++));
	auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->createDatablock(id, id, this->macroblock, this->blendblock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture, &this->samplerblock);
	datablock->setUseColour(true);

	return reinterpret_cast<Rml::TextureHandle>(datablock);
}
Rml::TextureHandle RenderInterface::GenerateTexture(
	Rml::Span<const Rml::byte> source,
	Rml::Vector2i source_dimensions)
{
	Ogre::String id = this->workspace.getNameStr();
	id.append("_Texture_");
	id.append(std::to_string(this->datablockId++));

	std::size_t size = Ogre::PixelFormatGpuUtils::calculateSizeBytes(
        source_dimensions.x, source_dimensions.y, 1u, 1u, Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB, 1u, 4u);
	Ogre::uint8* data = reinterpret_cast<Ogre::uint8*>(
		OGRE_MALLOC_SIMD(size, Ogre::MEMCATEGORY_GENERAL));
	std::copy(source.begin(), source.end(), data);

	auto* image = OGRE_NEW Ogre::Image2;
	image->loadDynamicImage(
		data,
		source_dimensions.x,
		source_dimensions.y,
		1u,
		Ogre::TextureTypes::Type2D,
		Ogre::PixelFormatGpu::PFG_RGBA8_UNORM,
		true,
		1u);

	Ogre::TextureGpuManager& textureManager = *Ogre::Root::getSingleton().getRenderSystem()
		->getTextureGpuManager();
	Ogre::TextureGpu* texture = textureManager.createTexture(
		id,
		Ogre::GpuPageOutStrategy::AlwaysKeepSystemRamCopy,
		Ogre::TextureFlags::AutomaticBatching,
		Ogre::TextureTypes::Type2D,
		Ogre::BLANKSTRING);
	texture->setNumMipmaps(1);
	texture->setResolution(source_dimensions.x, source_dimensions.y);
	texture->setPixelFormat(Ogre::PixelFormatGpu::PFG_RGBA8_UNORM);

	bool canUseSynchronousUpload =
		texture->getNextResidencyStatus() == Ogre::GpuResidency::Resident
		&& texture->isDataReady();
	if(!canUseSynchronousUpload)
		texture->waitForData();
	texture->scheduleTransitionTo(Ogre::GpuResidency::Resident, image, false);

	auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->createDatablock(id, id, this->macroblock, this->blendblock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture, &this->samplerblock);
	datablock->setUseColour(true);

	return reinterpret_cast<Rml::TextureHandle>(datablock);
}
void RenderInterface::ReleaseTexture(Rml::TextureHandle texture)
{
	this->releaseTextures.push_back(texture);
}


void RenderInterface::EnableScissorRegion(bool enable)
{
	this->renderPassSettings.enableScissor = enable;
}
void RenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
	this->renderPassSettings.scissorRegion = region;
}


void RenderInterface::SetTransform(const Rml::Matrix4f* transform)
{
	this->renderPassSettings.transform = transform
		? Ogre::Matrix4(transform->data()).transpose()
		: Ogre::Matrix4::IDENTITY;
}


void RenderInterface::EnableClipMask(bool enable)
{
	this->renderPassSettings.enableStencil = enable;
}
void RenderInterface::RenderToClipMask(
	Rml::ClipMaskOperation operation,
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation)
{
	switch(operation)
	{
	case Rml::ClipMaskOperation::Set:
		this->renderPassSettings.stencilRefValue = 1;
		break;
	case Rml::ClipMaskOperation::SetInverse:
		this->renderPassSettings.stencilRefValue = 0;
		break;
	case Rml::ClipMaskOperation::Intersect:
		++this->renderPassSettings.stencilRefValue;
		break;
	}

	switch(operation)
	{
	case Rml::ClipMaskOperation::Set:
		this->getRenderPass<RenderToStencilSetPass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock,
			nullptr
		});
		break;
	case Rml::ClipMaskOperation::SetInverse:
		this->getRenderPass<RenderToStencilSetInversePass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock,
			nullptr
		});
		break;
	case Rml::ClipMaskOperation::Intersect:
		this->getRenderPass<RenderToStencilIntersectPass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock,
			nullptr
		});
		break;
	}

	switch(operation)
	{
	case Rml::ClipMaskOperation::Set:
		break;
	case Rml::ClipMaskOperation::SetInverse:
		this->renderPassSettings.stencilRefValue = 1;
		break;
	case Rml::ClipMaskOperation::Intersect:
		break;
	}
}


Rml::LayerHandle RenderInterface::PushLayer()
{
	int oldTopLayer = this->addConnection();
	this->layerBuffers.push_back(oldTopLayer);
	this->passes.push_back({StartLayerPass(oldTopLayer)});
	return Rml::LayerHandle{this->layerBuffers.size()};
}
void RenderInterface::CompositeLayers(
	Rml::LayerHandle source,
	Rml::LayerHandle destination,
	Rml::BlendMode blend_mode,
	Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	int topLayer = this->addConnection();
	int sourceLayer = -1;
	int destinationLayer = -1;

	bool sourceIsTopLayer = source == this->layerBuffers.size();
	bool destinationIsTopLayer = destination == this->layerBuffers.size();

	if(!sourceIsTopLayer)
	{
		this->passes.push_back(SwapPass(this->layerBuffers.at(source), topLayer));
		sourceLayer = this->addConnection();
		this->passes.push_back(CopyPass(sourceLayer));
	}
	else
		this->passes.push_back(CopyPass(topLayer));

	if(destinationIsTopLayer)
	{
		destinationLayer = topLayer;
		topLayer = -1;
	}
	else if(source == destination)
	{
		destinationLayer = sourceLayer;
		sourceLayer = -1;
	}
	else
		destinationLayer = this->layerBuffers.at(destination);

	if(sourceLayer != -1)
		this->layerBuffers.at(source) = sourceLayer;


	for(auto filter : filters)
		this->filters.at(filter)->apply(*this);


	if(this->renderPassSettings.enableStencil)
		this->passes.push_back(CompositeWithStencilPass(
			destinationLayer,
			blend_mode == Rml::BlendMode::Replace,
			this->renderPassSettings));
	else
		this->passes.push_back(CompositePass(
			destinationLayer,
			blend_mode == Rml::BlendMode::Replace,
			this->renderPassSettings));

	if(!destinationIsTopLayer)
	{
		destinationLayer = this->addConnection();
		this->passes.push_back(SwapPass(topLayer, destinationLayer));
		this->layerBuffers.at(destination) = destinationLayer;
	}
}
void RenderInterface::PopLayer()
{
	this->passes.push_back({SwapPass(this->layerBuffers.back())});
	this->layerBuffers.pop_back();
}


Rml::CompiledFilterHandle RenderInterface::CompileFilter(
	const Rml::String& name,
	const Rml::Dictionary& parameters)
{
	auto maker = this->filterMakers.find(name);
	if(maker == this->filterMakers.end())
			return {};

	auto filter = maker->second->make(parameters);
	auto handle = this->filters.insert(std::move(filter));
	return handle;
}
void RenderInterface::ReleaseFilter(Rml::CompiledFilterHandle filter)
{
	auto& compiledFilter = this->filters.at(filter);
	compiledFilter->release(*this);
	this->filters.erase(filter);
}


Rml::TextureHandle RenderInterface::SaveLayerAsTexture()
{
	Rml::Vector2i dimensions;
	if(this->renderPassSettings.enableScissor)
		dimensions = this->renderPassSettings.scissorRegion.Size();
	else
		dimensions = Rml::Vector2i{
			int(this->workspace.width()),
			int(this->workspace.height())
		};

	auto renderTexture = this->workspace.getRenderTexture();
	Ogre::TextureGpu* texture = renderTexture.first;
	// This looks wrong but it works?
	// Don't want to invalidate texture pointer so don't recreate texture
	if(texture->getResidencyStatus() == Ogre::GpuResidency::Resident)
	{
		texture->_transitionTo(Ogre::GpuResidency::OnStorage, nullptr);
		texture->_setNextResidencyStatus(Ogre::GpuResidency::OnStorage);
	}
	texture->setResolution(dimensions.x, dimensions.y);
	if(texture->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
	{
		texture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
		texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
	}

	Ogre::String id = this->workspace.getNameStr();
	id.append("_Texture_");
	id.append(std::to_string(this->datablockId++));
	auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->createDatablock(id, id, this->macroblock, this->blendblock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture);
	datablock->setUseColour(true);

	this->passes.push_back(RenderToTexturePass(renderTexture.second, this->renderPassSettings));

	return reinterpret_cast<Rml::TextureHandle>(datablock);
}

Rml::CompiledFilterHandle RenderInterface::SaveLayerAsMaskImage()
{
	Rml::Vector2i dimensions;
	if(this->renderPassSettings.enableScissor)
		dimensions = this->renderPassSettings.scissorRegion.Size();
	else
		dimensions = Rml::Vector2i{
			int(this->workspace.width()),
			int(this->workspace.height())
		};

	auto renderTexture = this->workspace.getRenderTexture();
	Ogre::TextureGpu* texture = renderTexture.first;
	// This looks wrong but it works?
	// Don't want to invalidate texture pointer so don't recreate texture
	if(texture->getResidencyStatus() == Ogre::GpuResidency::Resident)
	{
		texture->_transitionTo(Ogre::GpuResidency::OnStorage, nullptr);
		texture->_setNextResidencyStatus(Ogre::GpuResidency::OnStorage);
	}
	texture->setResolution(dimensions.x, dimensions.y);
	if(texture->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
	{
		texture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
		texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
	}

	this->passes.push_back(RenderToTexturePass(
		renderTexture.second,
		this->renderPassSettings));

	auto filter = this->maskImageFilterMaker.make(texture);
	auto handle = this->filters.insert(std::make_unique<SingleMaterialFilter>(filter));
	return handle;
}


Rml::CompiledShaderHandle RenderInterface::CompileShader(
	const Rml::String& name,
	const Rml::Dictionary& parameters)
{
	auto maker = this->shaderMakers.find(name);
	if(maker == this->shaderMakers.end())
		return {};

	auto shader = maker->second->make(parameters);
	auto handle = this->shaders.insert(std::move(shader));
	return handle;
}
void RenderInterface::RenderShader(
	Rml::CompiledShaderHandle shader,
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation,
	Rml::TextureHandle texture)
{
	auto& material = this->shaders.at(shader);

	std::vector<QueuedGeometry>* queue = nullptr;
	if(this->renderPassSettings.enableStencil)
		queue = &this->getRenderPass<RenderWithStencilPass>().queue;
	else
		queue = &this->getRenderPass<RenderPass>().queue;

	queue->push_back({
		reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
		translation,
		nullptr,
		material
	});
}
void RenderInterface::ReleaseShader(Rml::CompiledShaderHandle shader)
{
	this->shaders.erase(shader);
}
