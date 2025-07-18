#include "RenderInterface.hpp"

#include "CompositorPassGeometry.hpp"
#include "CompositorPassGeometryDef.hpp"
#include "geometry.hpp"

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
		textureManager.destroyTexture(textureGpu);
	}
	this->releaseTextures.clear();
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
		texture ? reinterpret_cast<Ogre::HlmsUnlitDatablock*>(texture) : this->noTextureDatablock
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
			Ogre::TextureFlags::AutomaticBatching | Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
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
		Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB,
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
	texture->setPixelFormat(Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB);

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
		break;
	}

	switch(operation)
	{
	case Rml::ClipMaskOperation::Set:
		this->getRenderPass<RenderToStencilSetPass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock
		});
		break;
	case Rml::ClipMaskOperation::SetInverse:
		this->getRenderPass<RenderToStencilSetInversePass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock
		});
		break;
	case Rml::ClipMaskOperation::Intersect:
		this->getRenderPass<RenderToStencilIntersectPass>().queue.push_back({
			reinterpret_cast<Ogre::VertexArrayObject*>(geometry),
			translation,
			this->noTextureDatablock
		});
		break;
	}

	switch(operation)
	{
	case Rml::ClipMaskOperation::Set:
	case Rml::ClipMaskOperation::SetInverse:
		this->renderPassSettings.stencilRefValue = 1;
		break;
	case Rml::ClipMaskOperation::Intersect:
		++this->renderPassSettings.stencilRefValue;
		break;
	}
}
