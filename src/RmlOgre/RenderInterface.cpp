#include "RenderInterface.hpp"

#include "CompositorPassGeometry.hpp"
#include "CompositorPassGeometryDef.hpp"
#include "Geometry.hpp"

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

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>

#include <OgreTextureGpuManager.h>


using namespace nimble::RmlOgre;

RenderInterface::RenderInterface(
	const Ogre::String& name,
	Ogre::SceneManager* sceneManager,
	Rml::Vector2i resolution
) :
	hlms{static_cast<Ogre::HlmsUnlit*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT))},
	sceneManager{sceneManager}
{
	this->camera = this->sceneManager->createCamera(name + "_Camera");
	this->output = Ogre::Root::getSingleton()
		.getRenderSystem()
		->getTextureGpuManager()
		->createOrRetrieveTexture(
			name,
			Ogre::GpuPageOutStrategy::Discard,
			Ogre::TextureFlags::RenderToTexture,
			Ogre::TextureTypes::Type2D);
	this->output->setPixelFormat(Ogre::PFG_RGBA16_FLOAT);
	this->SetViewport(resolution);

	Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingleton().getCompositorManager2();
	this->workspaceDef = compositorManager->addWorkspaceDefinition(name);
	this->workspaceDef->connectExternal(0, "End", 1);
	// Prevent warnings
	// Can't disable nodes in scripts
	compositorManager->getNodeDefinitionNonConst("UiRender")->setStartEnabled(false);

	this->buildWorkspace(8);

	this->passes.push_back({});

	this->macroblock.mScissorTestEnabled = true;
	this->macroblock.mDepthCheck = false;
	this->macroblock.mDepthWrite = false;
	this->macroblock.mCullMode = Ogre::CULL_NONE;

	this->blendBlock.mSeparateBlend = false;
	this->blendBlock.mBlendOperation = Ogre::SceneBlendOperation::SBO_ADD;
	this->blendBlock.mDestBlendFactor = Ogre::SceneBlendFactor::SBF_ONE_MINUS_SOURCE_ALPHA;
	this->blendBlock.mSourceBlendFactor = Ogre::SceneBlendFactor::SBF_ONE;

	this->noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->getDatablock("NoTexture"));
	if(!this->noTextureDatablock)
		this->noTextureDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			this->hlms->createDatablock(
				"NoTexture",
				"NoTexture",
				this->macroblock,
				this->blendBlock,
				Ogre::HlmsParamVec())
		);
	this->noTextureDatablock->setUseColour(true);
}

void RenderInterface::buildWorkspace(std::size_t numGeometryNodes)
{
	Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingleton().getCompositorManager2();

	// This is just to prevent warnings about unconnected channels,
	// populateWorkspace will overwrite this connection
	this->workspaceDef->connect("Start", 0, "End", 0);

	// Clear existing
	for(auto& node : this->geometryNodes)
		this->workspaceDef->removeNodeAlias(node->getName());
	this->geometryNodes.clear();
	if(this->workspace)
		compositorManager->removeWorkspace(this->workspace);

	std::vector<Ogre::IdString> nodeNames;
	for(std::size_t i = 0; i < numGeometryNodes; ++i)
	{
		auto name = Ogre::IdString("UiRender_") + Ogre::IdString(i);
		this->workspaceDef->addNodeAlias(name, "UiRender");
		nodeNames.push_back(name);
	}

	this->workspace = compositorManager->addWorkspace(
		this->sceneManager,
		this->output,
		this->camera,
		this->workspaceDef->getName(),
		true);

	this->geometryNodes.reserve(nodeNames.size());
	for(auto& nodeName : nodeNames)
	{
		auto* node = this->workspace->findNode(nodeName);
		node->setEnabled(false);
		this->geometryNodes.push_back(node);
	}

	// This is just to prevent warnings about multiple connections
	this->workspaceDef->clearAllInterNodeConnections();
}

void RenderInterface::populateWorkspace()
{
	if(this->passes.size() > geometryNodes.size())
	{
		auto numGeometryNodes = geometryNodes.size();
		while(numGeometryNodes < this->passes.size())
			numGeometryNodes *= 2;
		this->buildWorkspace(numGeometryNodes);
	}
	this->workspaceDef->clearAllInterNodeConnections();

	for(auto& node : this->geometryNodes)
	{
		auto& passes = node->_getPasses();
		if(passes.size() >= 1)
		{
			auto* nodePass = static_cast<CompositorPassGeometry*>(passes.at(0));
			nodePass->renderQueue->clear();
		}
	}

	std::size_t totalRenderObjects = 0;
	for(auto& pass : passes)
		totalRenderObjects += pass.queue.size();

	this->sceneNodes.clear();
	this->sceneNodes.reserve(totalRenderObjects);
	this->renderObjects.clear();
	this->renderObjects.reserve(totalRenderObjects);

	this->releaseBufferedGeometry();
	this->releaseBufferedTextures();


	// Connect required nodes in workspace, which creates passes
	auto geometryNode = this->geometryNodes.begin();
	Ogre::IdString lastNode = "Start";
	std::vector<Ogre::CompositorNode*> activeNodes;
	activeNodes.reserve(this->passes.size());
	for(std::size_t i = 0; i < this->passes.size(); ++i)
	{
		auto& node = *geometryNode++;
		node->setEnabled(true);

		auto nodeName = node->getName();
		activeNodes.push_back(node);
		// Ogre bug stops this working
		// this->workspaceDef->connect(lastNode, nodeName);
		// Workaround is using overload that specifies node channels
		this->workspaceDef->connect(lastNode, 0, nodeName, 0);
		this->workspaceDef->connect(lastNode, 1, nodeName, 1);
		lastNode = nodeName;
	}
	for(; geometryNode != this->geometryNodes.end(); ++geometryNode)
		(*geometryNode)->setEnabled(false);

	this->workspaceDef->connect(lastNode, 0, "End", 0);
	this->workspace->reconnectAllNodes();


	// Fill in passes with data
	auto nodeIter = activeNodes.begin();
	for(auto& pass : this->passes)
	{
		auto* node = *nodeIter++;
		auto& passes = node->_getPasses();
		if(passes.empty())
			continue;
		auto* nodePass = static_cast<CompositorPassGeometry*>(passes.at(0));
		for(auto& queueObject : pass.queue)
		{
			this->renderObjects.emplace_back(
				Ogre::Id::generateNewId<Ogre::MovableObject>(),
				&this->objectMemoryManager,
				nullptr,
				RenderObject::RENDER_QUEUE_ID
			);
			auto& object = this->renderObjects.back();
			object.setVao(queueObject.geometry->vao);
			object.setDatablock(queueObject.datablock);

			this->sceneNodes.emplace_back(
				Ogre::Id::generateNewId<Ogre::Node>(),
				nullptr,
				&this->nodeMemoryManager,
				nullptr);
			auto& sceneNode = this->sceneNodes.back();
			sceneNode.setPosition(Ogre::Vector3{queueObject.translation.x, queueObject.translation.y, 0.0f});
			sceneNode.attachObject(&object);

			for(auto renderable : object.mRenderables)
			{
				nodePass->renderQueue->addRenderableV2(
					0,
					RenderObject::RENDER_QUEUE_ID,
					false,
					renderable,
					&object);
			}
		}

		if(pass.settings.enableScissor)
		{
			float width = this->output->getWidth();
			float height = this->output->getHeight();
			nodePass->scissorRegion = Ogre::Vector4{
				pass.settings.scissorRegion.Left() / width,
				pass.settings.scissorRegion.Top() / height,
				pass.settings.scissorRegion.Width() / width,
				pass.settings.scissorRegion.Height() / height
			};
		}
		else
			nodePass->scissorRegion = Ogre::Vector4{0.0f, 0.0f, 1.0f, 1.0f};
	}


	if(!this->sceneNodes.empty())
	{
		Ogre::Transform t;
		std::size_t numNodes = this->nodeMemoryManager.getFirstNode(t, 0);
		Ogre::Node::updateAllTransforms(numNodes, t);
	}
}

void RenderInterface::releaseBufferedGeometry()
{
	for(Rml::CompiledGeometryHandle geometry : this->releaseGeometry)
		std::unique_ptr<Geometry>(reinterpret_cast<Geometry*>(geometry));
	this->releaseGeometry.clear();
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

void RenderInterface::EndFrame()
{
	this->populateWorkspace();

	this->passes.clear();
	this->passes.push_back({});
}

void RenderInterface::SetViewport(Rml::Vector2i dimensions)
{
	this->SetViewport(dimensions.x, dimensions.y);
}
void RenderInterface::SetViewport(int width, int height)
{
	assert(width > 0);
	assert(height > 0);

	// This looks wrong but it works?
	// Don't want to invalidate texture pointer so don't recreate texture
	if(this->output->getResidencyStatus() == Ogre::GpuResidency::Resident)
	{
		this->output->_transitionTo(Ogre::GpuResidency::OnStorage, nullptr);
		this->output->_setNextResidencyStatus(Ogre::GpuResidency::OnStorage);
	}
	this->output->setResolution(width, height);
	if(this->output->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
	{
		this->output->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
		this->output->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
	}

	float scaleX = 2.0f / width;
	float scaleY = -2.0f / height;
	this->camera->setCustomProjectionMatrix(true, Ogre::Matrix4
	{
		scaleX, 0.0,    0.0, -1.0,
		0.0,    scaleY, 0.0, 1.0,
		0.0,    0.0,    1.0, 0.0,
		0.0,    0.0,    0.0, 1.0
	});
}


Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(
	Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices)
{
	auto geometry = std::make_unique<Geometry>(vertices, indices);
	return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry.release());
}
void RenderInterface::RenderGeometry(
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation,
	Rml::TextureHandle texture)
{
	this->passes.back().queue.push_back({
		reinterpret_cast<Geometry*>(geometry),
		translation,
		texture ? reinterpret_cast<Ogre::HlmsUnlitDatablock*>(texture) : this->noTextureDatablock
	});
}
void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	this->releaseGeometry.push_back(geometry);
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
		texture = textureManager->createOrRetrieveTexture(
			source,
			Ogre::GpuPageOutStrategy::Discard,
			Ogre::TextureFlags::AutomaticBatching | Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
			Ogre::TextureTypes::Type2D,
			Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
	}

	if(!texture)
		return Rml::TextureHandle{};

	texture->waitForMetadata();
	texture_dimensions = Rml::Vector2i(texture->getWidth(), texture->getHeight());

	Ogre::String id = this->workspaceDef->getNameStr();
	id.append("_Texture_");
	id.append(std::to_string(this->datablockId++));
	auto* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
		this->hlms->createDatablock(id, id, this->macroblock, this->blendBlock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture);
	datablock->setUseColour(true);

	return reinterpret_cast<Rml::TextureHandle>(datablock);
}
Rml::TextureHandle RenderInterface::GenerateTexture(
	Rml::Span<const Rml::byte> source,
	Rml::Vector2i source_dimensions)
{
	Ogre::String id = this->workspaceDef->getNameStr();
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
	Ogre::TextureGpu* texture = textureManager.createOrRetrieveTexture(
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
		this->hlms->createDatablock(id, id, this->macroblock, this->blendBlock, Ogre::HlmsParamVec()));
	datablock->setTexture(0, texture);
	datablock->setUseColour(true);

	return reinterpret_cast<Rml::TextureHandle>(datablock);
}
void RenderInterface::ReleaseTexture(Rml::TextureHandle texture)
{
	this->releaseTextures.push_back(texture);
}

void RenderInterface::EnableScissorRegion(bool enable)
{
	auto& lastPass = this->passes.back();
	if(lastPass.queue.empty())
		lastPass.settings.enableScissor = enable;
	else
	{
		Pass newPass;
		newPass.settings = lastPass.settings;
		newPass.settings.enableScissor = enable;
		this->passes.push_back(std::move(newPass));
	}
}
void RenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
	auto& lastPass = this->passes.back();
	if(lastPass.queue.empty())
		lastPass.settings.scissorRegion = region;
	else
	{
		Pass newPass;
		newPass.settings = lastPass.settings;
		newPass.settings.scissorRegion = region;
		this->passes.push_back(std::move(newPass));
	}
}
