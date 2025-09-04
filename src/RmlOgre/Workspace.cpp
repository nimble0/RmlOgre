#include "Workspace.hpp"

#include "Compositor/CompositorPassRenderQuadDef.hpp"

#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <OgreCamera.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreRoot.h>
#include <OgreTextureGpuManager.h>

#include <RmlUi/Core/Core.h>

#include <tuple>


using namespace nimble::RmlOgre;

namespace {

template <template <class...> class Variant, class... Ts>
std::tuple<Ts...> variant_tuple_helper(Variant<Ts...>)
{
    return std::tuple<Ts...>{};
}

template <class T>
using variant_tuple = decltype(variant_tuple_helper(T{}));

template <std::size_t I, class T>
struct pass_node;

template <std::size_t I, class T>
struct pass_node
{
	static NodeType get_node_type(std::size_t i)
	{
		if(i == I - 1)
		{
			using Type = std::tuple_element_t<I - 1, T>;
			NodeType nodeType;
			nodeType.baseName = Type::BASE_NODE_NAME;
			nodeType.clear = &Type::clearNode;
			return nodeType;
		}
		else
			return pass_node<I - 1, T>::get_node_type(i);
	}
};
template <class T>
struct pass_node<0, T>
{
	static NodeType get_node_type(std::size_t i)
	{
		throw std::out_of_range("Variant index out of range");
	}
};

template <class T>
NodeType get_pass_node_type(std::size_t i)
{
	constexpr std::size_t SIZE = std::tuple_size_v<variant_tuple<T>>;

	if(i >= SIZE)
		throw std::out_of_range("Variant index out of range");

	return pass_node<SIZE, variant_tuple<T>>::get_node_type(i);
}

void connect_nodes(
	std::size_t maxChannels,
	Ogre::CompositorNode* outNode,
	Ogre::CompositorNode* inNode)
{
	using namespace Ogre;

	const CompositorNodeDef* outDef = outNode->getDefinition();
	const CompositorNodeDef* inDef = inNode->getDefinition();

	size_t channels = std::min({inDef->getNumInputChannels(), outDef->getNumOutputChannels(), maxChannels});

	for(uint32 i = 0; i < channels; ++i)
		outNode->connectTo(i, inNode, i);
}

std::size_t grow_capacity(std::size_t current, std::size_t min)
{
	if(min > 0)
		current = std::max(current, std::size_t(1));
	while(current < min)
		current *= 2;
	return current;
}

}


Workspace::Workspace(
	Ogre::String name,
	Ogre::SceneManager* sceneManager,
	Ogre::TextureGpu* output,
	Ogre::TextureGpu* background
) :
	sceneManager{sceneManager},
	output{output},
	background{background}
{
	this->camera = this->sceneManager->createCamera(name + "_Camera");

	Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingleton().getCompositorManager2();
	this->workspaceDef = compositorManager->addWorkspaceDefinition(name);
	// Start from 1 to skip null pass
	for(std::size_t i = 1; i < this->nodeTypes.size(); ++i)
	{
		auto& nodeType = this->nodeTypes[i];

		nodeType = get_pass_node_type<Pass>(i);

		// Prevent warnings
		// Can't disable nodes in scripts
		compositorManager->getNodeDefinitionNonConst(nodeType.baseName)->setStartEnabled(false);
	}

	static_cast<CompositorPassRenderQuadDef*>(compositorManager
		->getNodeDefinitionNonConst("Rml/Composite")
		->getTargetPass(0)
		->getCompositorPassesNonConst()[0])
		->mMaterialName = "Rml/AlphaBlend";
	static_cast<CompositorPassRenderQuadDef*>(compositorManager
		->getNodeDefinitionNonConst("Rml/CompositeWithStencil")
		->getTargetPass(0)
		->getCompositorPassesNonConst()[1])
		->mMaterialName = "Rml/AlphaBlend";
	static_cast<CompositorPassRenderQuadDef*>(compositorManager
		->getNodeDefinitionNonConst("Rml/RenderToTexture")
		->getTargetPass(0)
		->getCompositorPassesNonConst()[0])
		->mMaterialName = "Rml/ScissorCopy";

	this->reserveRenderTextures(4);
	this->buildWorkspace({});
}

void Workspace::buildWorkspace(const std::array<std::size_t, NUM_NODE_TYPES>& reservedNodes)
{
	Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingleton().getCompositorManager2();

	// Clear existing
	if(this->workspace)
		compositorManager->removeWorkspace(this->workspace);
	for(auto& nodeType : this->nodeTypes)
		nodeType.nodes.clear();
	this->workspaceDef->clearAll();

	// This is just to prevent warnings about unconnected channels,
	// populateWorkspace will overwrite this connection
	this->workspaceDef->connect("Rml/Start", 0, "Rml/End", 0);
	this->workspaceDef->connectExternal(0, "Rml/End", 1);
	this->workspaceDef->connectExternal(1, "Rml/Start", 0);

	std::array<std::vector<Ogre::IdString>, NUM_NODE_TYPES> nodeTypeNames;
	// Start from 1 to skip null passes
	for(std::size_t i = 1; i < reservedNodes.size(); ++i)
	{
		auto& nodeType = this->nodeTypes[i];
		auto n = reservedNodes[i];
		std::vector<Ogre::IdString> nodeNames;
		nodeNames.reserve(n);
		for(std::size_t j = 0; j < n; ++j)
		{
			auto name = nodeType.baseName + Ogre::IdString(j);
			this->workspaceDef->addNodeAlias(name, nodeType.baseName);
			nodeNames.push_back(name);
		}
		nodeTypeNames[i] = std::move(nodeNames);
	}

	Ogre::CompositorChannelVec externalTextures;
	externalTextures.reserve(this->renderTextures.size() + 2);
	externalTextures.push_back(this->output);
	externalTextures.push_back(this->background);
	for(auto& texture : this->renderTextures)
		externalTextures.push_back(texture);

	this->workspace = compositorManager->addWorkspace(
		this->sceneManager,
		externalTextures,
		this->camera,
		this->workspaceDef->getName(),
		true);

	for(std::size_t i = 0; i < nodeTypeNames.size(); ++i)
	{
		auto& nodeType = this->nodeTypes[i];
		auto& names = nodeTypeNames[i];

		std::vector<Ogre::CompositorNode*> nodes;
		nodes.reserve(names.size());
		for(auto& name : names)
		{
			auto* node = this->workspace->findNode(name);
			node->setEnabled(false);
			nodes.push_back(node);
		}
		nodeType.nodes = std::move(nodes);
	}

	// This is just to prevent warnings about multiple connections
	this->workspaceDef->clearAllInterNodeConnections();
}

void Workspace::ensureWorkspaceNodes(const std::array<std::size_t, NUM_NODE_TYPES>& minNodes)
{
	bool needRebuild = false;
	// Start from 1 to skip null passes
	for(std::size_t i = 1; i < this->nodeTypes.size(); ++i)
		needRebuild = needRebuild || this->nodeTypes[i].nodes.size() < minNodes[i];

	needRebuild = needRebuild
		|| this->workspace->getExternalRenderTargets().size() < (this->renderTextures.size() + 2);

	if(needRebuild)
	{
		std::array<std::size_t, NUM_NODE_TYPES> numNodes;
		// Start from 1 to skip null passes
		for(std::size_t i = 1; i < this->nodeTypes.size(); ++i)
			numNodes[i] = grow_capacity(this->nodeTypes[i].nodes.size(), minNodes[i]);

		this->buildWorkspace(numNodes);
	}
}

void Workspace::reserveRenderTextures(std::size_t capacity)
{
	while(this->renderTextures.size() < capacity)
	{
		Ogre::String id = this->workspaceDef->getNameStr();
		id.append("_RenderTexture_");
		id.append(std::to_string(this->renderTextures.size()));
		auto* texture = Ogre::Root::getSingleton()
			.getRenderSystem()
			->getTextureGpuManager()
			->createTexture(
				id,
				Ogre::GpuPageOutStrategy::SaveToSystemRam,
				Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps,
				Ogre::TextureTypes::Type2D);
		texture->setPixelFormat(Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB);
		texture->setResolution(1, 1);
		texture->setNumMipmaps(8);
		if(texture->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
		{
			texture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
			texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
		}
		this->renderTextures.add(texture);
	}
}
void Workspace::reserveRenderObjects(std::size_t capacity)
{
	this->renderObjects.reserve(capacity);
}
void Workspace::reserveSceneNodes(std::size_t capacity)
{
	this->sceneNodes.reserve(capacity);
}
void Workspace::updateSceneNodes()
{
	if(!this->sceneNodes.empty())
	{
		Ogre::Transform t;
		std::size_t numNodes = this->nodeMemoryManager.getFirstNode(t, 0);
		Ogre::Node::updateAllTransforms(numNodes, t);
	}
}

void Workspace::updateProjectionMatrix()
{
	float scaleX = 2.0f / this->output->getWidth();
	float scaleY = -2.0f / this->output->getHeight();
	this->projectionMatrix_ = Ogre::Matrix4
	{
		scaleX, 0.0,    0.0, -1.0,
		0.0,    scaleY, 0.0, 1.0,
		0.0,    0.0,    1.0, 0.0,
		0.0,    0.0,    0.0, 1.0
	};
}

void Workspace::clearAll()
{
	for(auto& nodeType : this->nodeTypes)
		nodeType.clearAll();

	this->sceneNodes.clear();
	this->renderObjects.clear();
}

void Workspace::populateWorkspace(const Passes& passes)
{
	this->updateProjectionMatrix();

	std::array<std::size_t, Workspace::NUM_NODE_TYPES> nodeTypeCounts;
	nodeTypeCounts.fill(0);
	for(auto& pass : passes)
		++nodeTypeCounts[pass.index()];
	this->ensureWorkspaceNodes(nodeTypeCounts);

	this->workspaceDef->clearAllInterNodeConnections();
	this->workspaceDef->clearOutputConnections();

	std::size_t totalRenderObjects = 0;
	for(auto& pass : passes)
		std::visit([&](auto& pass)
		{
			totalRenderObjects += pass.numRenderObjects();
		}, pass);
	this->reserveRenderObjects(totalRenderObjects);
	this->reserveSceneNodes(totalRenderObjects);


	// Connect nodes
	auto& nodeSequence = const_cast<Ogre::CompositorNodeVec&>(this->workspace->getNodeSequence());
	for(auto* node : nodeSequence)
		node->_notifyCleared();

	Ogre::CompositorNode* startNode = this->workspace->findNode("Rml/Start");
	Ogre::CompositorNode* endNode = this->workspace->findNode("Rml/End");

	nodeSequence.clear();
	nodeSequence.push_back(startNode);

	startNode->connectExternalRT(
		this->workspace->getExternalRenderTargets()[1],
		0);
	endNode->connectExternalRT(
		this->workspace->getExternalRenderTargets()[0],
		1);

	using NodesIter = std::vector<Ogre::CompositorNode*>::iterator;
	std::vector<std::pair<NodesIter, NodesIter>> nodeTypeIters;
	nodeTypeIters.reserve(this->nodeTypes.size());
	for(auto& type : this->nodeTypes)
		nodeTypeIters.push_back({type.nodes.begin(), type.nodes.end()});

	NodeConnectionMap extraConnections(this->workspace);

	Ogre::CompositorNode* lastActiveNode = startNode;
	for(auto& pass : passes)
	{
		// Skip null passes
		if(pass.index() == 0)
			continue;

		auto& nodeTypeIterPair = nodeTypeIters.at(pass.index());
		auto nodeIter = nodeTypeIterPair.first++;
		assert(nodeIter != nodeTypeIterPair.second);

		Ogre::CompositorNode* node = *nodeIter;
		node->setEnabled(true);
		connect_nodes(3, lastActiveNode, node);

		extraConnections.setCurrentNode(node);
		std::visit([&](auto& pass)
		{
			pass.addExtraConnections(extraConnections);
		}, pass);
		extraConnections.setCurrentNode(nullptr);

		if(node->_getPasses().empty())
			node->createPasses();
		std::visit([&](auto& pass)
		{
			pass.writePass(*this, node);
		}, pass);

		nodeSequence.push_back(node);
		lastActiveNode = node;
	}

	lastActiveNode->connectTo(0, endNode, 0);
	nodeSequence.push_back(endNode);

	// Disable unused nodes
	for(auto& iters : nodeTypeIters)
		for(auto iter = iters.first; iter != iters.second; ++iter)
		{
			(*iter)->setEnabled(false);
			nodeSequence.push_back(*iter);
		}

	startNode->createPasses();
	endNode->createPasses();
	this->workspace->_notifyBarriersDirty();

	this->updateSceneNodes();
}

RenderObject* Workspace::addRenderObject()
{
	assert(this->renderObjects.capacity() > this->renderObjects.size());

	this->renderObjects.emplace_back(
		Ogre::Id::generateNewId<Ogre::MovableObject>(),
		&this->objectMemoryManager,
		nullptr,
		RenderObject::RENDER_QUEUE_ID
	);
	return &this->renderObjects.back();
}
Ogre::SceneNode* Workspace::addSceneNode()
{
	assert(this->sceneNodes.capacity() > this->sceneNodes.size());

	this->sceneNodes.emplace_back(
		Ogre::Id::generateNewId<Ogre::Node>(),
		nullptr,
		&this->nodeMemoryManager,
		nullptr);
	return &this->sceneNodes.back();
}

Ogre::String Workspace::getNameStr() const
{
	return this->workspaceDef->getNameStr();
}
Ogre::IdString Workspace::getName() const
{
	return this->workspaceDef->getName();
}
Ogre::uint32 Workspace::width() const
{
	return this->output->getWidth();
}
Ogre::uint32 Workspace::height() const
{
	return this->output->getHeight();
}

std::pair<Ogre::TextureGpu*, std::size_t> Workspace::getRenderTexture()
{
	if(this->renderTextures.full())
		this->reserveRenderTextures(2 * this->renderTextures.size());

	auto claimed = this->renderTextures.claim();
	// First 2 render textures are reserved
	// Other render textures start at 2
	return {claimed.first, claimed.second + 2};
}
bool Workspace::freeRenderTexture(Ogre::TextureGpu* texture)
{
	return this->renderTextures.free(texture);
}
