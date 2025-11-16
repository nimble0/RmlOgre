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

	auto* noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->getDatablock("NoTexture"));
	if(!noTextureDatablock)
		noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			this->hlms->createDatablock(
				"NoTexture",
				"NoTexture",
				this->macroblock,
				this->blendblock,
				Ogre::HlmsParamVec())
		);
	noTextureDatablock->setUseColour(true);
	Material noTextureMaterial{nullptr, noTextureDatablock};
	noTextureMaterial.calculateHlmsHash();
	this->materials.insert(std::move(noTextureMaterial));

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
		auto& material = this->materials.at(texture);
		auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(material.datablock);
		auto* textureGpu = datablock->getTexture(0);

		assert(datablock->getLinkedRenderables().empty());

		this->hlms->destroyDatablock(datablock->getName());
		if(!this->workspace.freeRenderTexture(textureGpu))
			textureManager.destroyTexture(textureGpu);

		this->materials.erase(texture);
	}
	this->releaseTextures.clear();

	for(Ogre::TextureGpu* texture : this->releaseRenderTextures)
		this->workspace.freeRenderTexture(texture);
	this->releaseRenderTextures.clear();
}

Layer RenderInterface::getLayerBuffer(int index)
{
	if(index < 0)
		index += this->numActiveLayers;

	if(index >= static_cast<int>(this->layerBuffers.size()))
		throw std::out_of_range("Layer buffer index out of range");

	return this->layerBuffers[index].take();
}
void RenderInterface::putLayerBuffer(int index, Layer layer)
{
	if(index < 0)
		index += this->numActiveLayers;

	if(index >= static_cast<int>(this->layerBuffers.size()))
		throw std::out_of_range("Layer buffer index out of range");

	this->layerBuffers[index] = layer;
}
Layer RenderInterface::acquireLayerBuffer()
{
	if(this->numActiveLayers == static_cast<int>(this->layerBuffers.size()))
	{
		Layer layer{this->addConnection(), -1};
		this->layerBuffers.push_back(layer);
		this->passes.push_back(NewBufferPass(layer.connectionId));
	}

	++this->numActiveLayers;
	return this->getLayerBuffer(-1);
}
void RenderInterface::releaseLayerBuffer(Layer layer)
{
	this->putLayerBuffer(-1, layer);
	--this->numActiveLayers;
}


void RenderInterface::BeginFrame()
{
	this->workspace.clearAll();
	this->releaseBufferedGeometries();
	this->releaseBufferedTextures();

	this->numActiveLayers = 1;
	this->layerBuffers.push_back(Layer{-1, -1});
}

void RenderInterface::EndFrame()
{
	this->workspace.populateWorkspace(this->passes);

	this->passes.clear();
	this->layerBuffers.clear();
	this->numActiveLayers = 0;
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
	BaseRenderPass* pass = nullptr;
	if(this->renderPassSettings.enableStencil)
		pass = &this->getRenderPass<RenderWithStencilPass>();
	else
		pass = &this->getRenderPass<RenderPass>();

	auto& material = this->materials.at(texture);
	if(material.needsHashing())
		material.calculateHlmsHash();
	pass->queue.push_back({
		reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
		translation,
		material
	});
	if(material.textureDependency)
		pass->textureDependencies.push_back(material.textureDependency);
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

	auto material = Material{nullptr, datablock};
	material.calculateHlmsHash();
	auto handle = this->materials.insert(std::move(material));
	return handle;
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

	texture->scheduleTransitionTo(Ogre::GpuResidency::Resident, image);

	auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->createDatablock(id, id, this->macroblock, this->blendblock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture, &this->samplerblock);
	datablock->setUseColour(true);

	auto material = Material{nullptr, datablock};
	material.calculateHlmsHash();
	auto handle = this->materials.insert(std::move(material));
	return handle;
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
			this->materials[0]
		});
		break;
	case Rml::ClipMaskOperation::SetInverse:
		this->getRenderPass<RenderToStencilSetInversePass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->materials[0]
		});
		break;
	case Rml::ClipMaskOperation::Intersect:
		this->getRenderPass<RenderToStencilIntersectPass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->materials[0]
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
	Layer oldTopLayer{this->addConnection(), -1};
	this->putLayerBuffer(-1, oldTopLayer);
	Layer newLayer = this->acquireLayerBuffer();
	this->passes.push_back(SwapPass(newLayer.connectionId, oldTopLayer.connectionId));
	this->passes.push_back(StartLayerPass{});

	return Rml::LayerHandle(this->numActiveLayers - 1);
}
void RenderInterface::CompositeLayers(
	Rml::LayerHandle source,
	Rml::LayerHandle destination,
	Rml::BlendMode blend_mode,
	Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	Layer topLayer{this->addConnection(), -1};
	Layer sourceLayer;
	Layer destinationLayer;

	bool sourceIsTopLayer = static_cast<int>(source) == this->numActiveLayers - 1;
	bool destinationIsTopLayer = static_cast<int>(destination) == this->numActiveLayers - 1;

	Layer tempLayer = this->acquireLayerBuffer();
	if(sourceIsTopLayer)
	{
		topLayer.copyPass = this->passes.size();
		this->passes.push_back(CopyPass(tempLayer.connectionId, topLayer.connectionId));
	}
	else
	{
		this->passes.push_back(SwapPass(this->getLayerBuffer(source).connectionId, topLayer.connectionId));
		sourceLayer = Layer{this->addConnection(), static_cast<int>(this->passes.size())};
		this->passes.push_back(CopyPass(tempLayer.connectionId, sourceLayer.connectionId));
	}

	if(destinationIsTopLayer)
	{
		destinationLayer = topLayer;
		topLayer = Layer{};
	}
	else if(source == destination)
	{
		destinationLayer = sourceLayer;
		sourceLayer = Layer{};
	}
	else
		destinationLayer = this->layerBuffers.at(destination);

	// Change source layer to copy (if destination != source)
	if(!sourceLayer.isTaken())
		this->putLayerBuffer(source, sourceLayer);


	for(auto filter : filters)
		this->filters.at(filter)->apply(*this);


	tempLayer = Layer{this->addConnection(), -1};
	if(this->renderPassSettings.enableStencil)
	{
		this->passes.push_back(CompositeWithStencilPass(
			destinationLayer.connectionId,
			tempLayer.connectionId,
			blend_mode == Rml::BlendMode::Replace,
			this->renderPassSettings));
	}
	else
	{
		this->passes.push_back(CompositePass(
			destinationLayer.connectionId,
			tempLayer.connectionId,
			blend_mode == Rml::BlendMode::Replace,
			this->renderPassSettings));
	}
	this->releaseLayerBuffer(tempLayer);

	if(!destinationIsTopLayer)
	{
		destinationLayer = Layer{this->addConnection(), static_cast<int>(this->passes.size())};
		this->passes.push_back(SwapPass(topLayer.connectionId, destinationLayer.connectionId));
		this->putLayerBuffer(destination, destinationLayer);
		this->putLayerBuffer(-1, Layer{-1, topLayer.copyPass});
	}
}
void RenderInterface::PopLayer()
{
	Layer poppedLayer = this->getLayerBuffer(-1);
	Layer newTopLayer = this->getLayerBuffer(-2);
	auto* lastPass = std::get_if<SwapPass>(&this->passes.back());
	if(lastPass)
	{
		CopyPass* copyPass = nullptr;
		if(poppedLayer.copyPass != -1)
			copyPass = std::get_if<CopyPass>(&this->passes[poppedLayer.copyPass]);
		if(copyPass && lastPass->swapIn == copyPass->copyOut)
		{
			this->releaseLayerBuffer(Layer{copyPass->copyIn, -1});
			this->passes[poppedLayer.copyPass] = NullPass{};
			if(newTopLayer.connectionId == lastPass->swapOut)
				this->passes.pop_back();
			else
				this->passes.back() = SwapPass(newTopLayer.connectionId, lastPass->swapOut);
		}
		else
		{
			this->releaseLayerBuffer(Layer{lastPass->swapIn, -1});
			if(newTopLayer.connectionId == lastPass->swapOut)
				this->passes.pop_back();
			else
				this->passes.back() = SwapPass(newTopLayer.connectionId, lastPass->swapOut);
		}
	}
	else
	{
		Layer poppedLayer{this->addConnection(), -1};
		this->releaseLayerBuffer(poppedLayer);
		this->passes.push_back(SwapPass{newTopLayer.connectionId, poppedLayer.connectionId});
	}
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

	auto material = Material{nullptr, datablock, texture};
	material.calculateHlmsHash();
	auto handle = this->materials.insert(std::move(material));
	return handle;
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
	shader->setMacroblock(this->macroblock);
	shader->setBlendblock(this->blendblock);
	auto material = Material{shader};
	material.calculateHlmsHash();
	auto handle = this->shaders.insert(std::move(material));
	return handle;
}
void RenderInterface::RenderShader(
	Rml::CompiledShaderHandle shader,
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation,
	Rml::TextureHandle texture)
{
	auto& material = this->shaders.at(shader);
	if(material.needsHashing())
		material.calculateHlmsHash();

	std::vector<QueuedGeometry>* queue = nullptr;
	if(this->renderPassSettings.enableStencil)
		queue = &this->getRenderPass<RenderWithStencilPass>().queue;
	else
		queue = &this->getRenderPass<RenderPass>().queue;

	queue->push_back({
		reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
		translation,
		material
	});
}
void RenderInterface::ReleaseShader(Rml::CompiledShaderHandle shader)
{
	this->shaders.erase(shader);
}
