#include "Workspace.hpp"

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
	Ogre::CompositorWorkspaceDef& self,
	std::size_t maxChannels,
	Ogre::IdString outNode,
	Ogre::IdString inNode )
{
	using namespace Ogre;

	IdString aliasedOutNode = outNode;
	IdString aliasedInNode = inNode;

	auto& mAliasedNodes = self.getNodeAliasMap();
	auto* mCompositorManager = self.getCompositorManager();

	CompositorWorkspaceDef::NodeAliasMap::const_iterator aliasedOutNodeIt = mAliasedNodes.find( outNode );
	if( aliasedOutNodeIt != mAliasedNodes.end() )
		aliasedOutNode = aliasedOutNodeIt->second;

	CompositorWorkspaceDef::NodeAliasMap::const_iterator aliasedInNodeIt = mAliasedNodes.find( inNode );
	if( aliasedInNodeIt != mAliasedNodes.end() )
		aliasedInNode = aliasedInNodeIt->second;

	const CompositorNodeDef *outDef = mCompositorManager->getNodeDefinition( aliasedOutNode );
	const CompositorNodeDef *inDef = mCompositorManager->getNodeDefinition( aliasedInNode );

	size_t channels = std::min({inDef->getNumInputChannels(), outDef->getNumOutputChannels(), maxChannels});

	for( uint32 i = 0; i < channels; ++i )
		self.connect( outNode, i, inNode, i );
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

	this->workspace = compositorManager->addWorkspace(
		this->sceneManager,
		{ this->output, this->background },
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

	if(needRebuild)
	{
		std::array<std::size_t, NUM_NODE_TYPES> numNodes;
		// Start from 1 to skip null passes
		for(std::size_t i = 1; i < this->nodeTypes.size(); ++i)
			numNodes[i] = grow_capacity(this->nodeTypes[i].nodes.size(), minNodes[i]);

		this->buildWorkspace(numNodes);
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

	std::size_t totalRenderObjects = 0;
	for(auto& pass : passes)
		std::visit([&](auto& pass)
		{
			totalRenderObjects += pass.numRenderObjects();
		}, pass);
	this->reserveRenderObjects(totalRenderObjects);
	this->reserveSceneNodes(totalRenderObjects);

	// Connect required nodes in workspace, which creates passes
	using NodesIter = std::vector<Ogre::CompositorNode*>::iterator;
	std::vector<std::pair<NodesIter, NodesIter>> nodeTypeIters;
	nodeTypeIters.reserve(this->nodeTypes.size());
	for(auto& type : this->nodeTypes)
		nodeTypeIters.push_back({type.nodes.begin(), type.nodes.end()});
	std::vector<Ogre::CompositorNode*> activeNodes;
	activeNodes.reserve(passes.size());
	Ogre::IdString lastActiveNode = "Rml/Start";
	for(auto& pass : passes)
	{
		// Skip null passes
		if(pass.index() == 0)
			continue;

		auto& nodeTypeIterPair = nodeTypeIters.at(pass.index());
		auto nodeType = nodeTypeIterPair.first++;
		assert(nodeType != nodeTypeIterPair.second);

		(*nodeType)->setEnabled(true);
		auto nodeName = (*nodeType)->getName();

		connect_nodes(*this->workspaceDef, 2, lastActiveNode, nodeName);
		lastActiveNode = nodeName;

		activeNodes.push_back(*nodeType);
	}
	// Disable unused nodes
	for(auto& iters : nodeTypeIters)
		for(auto iter = iters.first; iter != iters.second; ++iter)
			(*iter)->setEnabled(false);

	this->workspaceDef->connect(lastActiveNode, 0, "Rml/End", 0);
	this->workspace->reconnectAllNodes();

	// Fill in passes with data
	auto nodeIter = activeNodes.begin();
	for(auto& pass : passes)
		std::visit([&](auto& pass)
		{
			pass.writePass(*this, *nodeIter++);
		}, pass);

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
