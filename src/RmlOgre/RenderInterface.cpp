#include "RenderInterface.hpp"

#include "CompositorPassGeometry.hpp"
#include "CompositorPassGeometryDef.hpp"
#include "Geometry.hpp"
#include "RenderInterface.hpp"

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

	// Clear existing
	for(auto& node : this->geometryNodes)
		compositorManager->removeNodeDefinition(node.node->getName());

	this->workspaceDef = compositorManager->addWorkspaceDefinition(this->output->getNameStr());
	this->workspaceDef->connect("Start", 0, "End", 0);
	this->workspaceDef->connectExternal(0, "End", 1);

	std::vector<std::pair<Ogre::IdString, CompositorPassGeometryDef*>> workingNodes;
	for(std::size_t i = 0; i < numGeometryNodes; ++i)
	{
		Ogre::String name = this->workspaceDef->getNameStr();
		name.append("_UiRender_");
		name.append(std::to_string(i));
		Ogre::CompositorNodeDef* node = compositorManager->addNodeDefinition(name);
		node->setStartEnabled(false);
		node->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TextureSource::TEXTURE_INPUT);
		node->addTextureSourceName("rt1", 1, Ogre::TextureDefinitionBase::TextureSource::TEXTURE_INPUT);

		node->setNumOutputChannels(2);
		node->mapOutputChannel(0, "rt0");
		node->mapOutputChannel(1, "rt1");

		node->setNumTargetPass(1);
		auto* target = node->addTargetPass("rt0");
		target->setNumPasses(1);
		auto* geometryPass = static_cast<CompositorPassGeometryDef*>(target->addPass(
			Ogre::CompositorPassType::PASS_CUSTOM,
			"rml_geometry"));

		auto nodeName = node->getName();
		this->workspaceDef->addNodeAlias(nodeName, nodeName);

		workingNodes.push_back({nodeName, geometryPass});
	}

	this->workspace = compositorManager->addWorkspace(
		this->sceneManager,
		this->output,
		this->camera,
		this->workspaceDef->getName(),
		true);

	this->geometryNodes.reserve(workingNodes.size());
	for(auto& workingNode : workingNodes)
	{
		auto* node = this->workspace->findNode(workingNode.first);
		auto* passDef = workingNode.second;
		this->geometryNodes.push_back(GeometryNode{node, passDef});
	}
}

void RenderInterface::populateWorkspace()
{
	this->workspaceDef->clearAllInterNodeConnections();

	assert(this->passes.size() <= geometryNodes.size());

	for(auto& node : this->geometryNodes)
		if(node.passDef->renderQueue)
			node.passDef->renderQueue->clear();

	std::size_t totalRenderObjects = 0;
	for(auto& pass : passes)
		totalRenderObjects += pass.queue.size();

	this->sceneNodes.clear();
	this->sceneNodes.reserve(totalRenderObjects);
	this->renderObjects.clear();
	this->renderObjects.reserve(totalRenderObjects);

	this->releaseBufferedGeometry();

	auto geometryNode = this->geometryNodes.begin();
	Ogre::IdString lastNode = "Start";
	for(auto& pass : passes)
	{
		auto& node = *geometryNode++;
		node.node->setEnabled(true);

		if(!node.passDef->renderQueue)
		{
			node.passDef->renderQueue = std::make_unique<Ogre::RenderQueue>(
				Ogre::Root::getSingleton().getHlmsManager(),
				this->sceneManager,
				Ogre::Root::getSingleton().getRenderSystem()->getVaoManager());
			node.passDef->renderQueue->setSortRenderQueue(
				RenderObject::RENDER_QUEUE_ID,
				Ogre::RenderQueue::RqSortMode::DisableSort);
		}

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
			object.setDatablock(this->noTextureDatablock);

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
				node.passDef->renderQueue->addRenderableV2(
					0,
					RenderObject::RENDER_QUEUE_ID,
					false,
					renderable,
					&object);
			}
		}

		auto nodeName = node.node->getName();
		this->workspaceDef->connect(lastNode, nodeName);
		lastNode = nodeName;
	}

	if(!this->sceneNodes.empty())
	{
		Ogre::Transform t;
		std::size_t numNodes = this->nodeMemoryManager.getFirstNode(t, 0);
		Ogre::Node::updateAllTransforms(numNodes, t);
	}

	this->workspaceDef->connect(lastNode, 0, "End", 0);

	for(; geometryNode != this->geometryNodes.end(); ++geometryNode)
		geometryNode->node->setEnabled(false);

	this->workspace->reconnectAllNodes();
}

void RenderInterface::releaseBufferedGeometry()
{
	for(Rml::CompiledGeometryHandle geometry : this->releaseGeometry)
		std::unique_ptr<Geometry>(reinterpret_cast<Geometry*>(geometry));
	this->releaseGeometry.clear();
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
	return Rml::TextureHandle{};
}
Rml::TextureHandle RenderInterface::GenerateTexture(
	Rml::Span<const Rml::byte> source,
	Rml::Vector2i source_dimensions)
{
	return Rml::TextureHandle{};
}
void RenderInterface::ReleaseTexture(Rml::TextureHandle texture) {}

void RenderInterface::EnableScissorRegion(bool enable) {}
void RenderInterface::SetScissorRegion(Rml::Rectanglei region) {}
