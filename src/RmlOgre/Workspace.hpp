#ifndef NIMBLE_RMLOGRE_WORKSPACE_HPP
#define NIMBLE_RMLOGRE_WORKSPACE_HPP

#include "Pass.hpp"
#include "RenderObject.hpp"

#include <Math/Array/OgreObjectMemoryManager.h>
#include <Math/Array/OgreNodeMemoryManager.h>
#include <OgreHlmsDatablock.h>
#include <OgreMemoryStdAlloc.h>
#include <OgreString.h>

#include <RmlUi/Core/RenderInterface.h>

#include <array>
#include <variant>


namespace Ogre {

class Camera;
class CompositorNode;
class CompositorNodeDef;
class CompositorWorkspace;
class CompositorWorkspaceDef;
class HlmsUnlit;
class HlmsUnlitDatablock;
class SceneManager;
class TextureGpu;

}

namespace nimble::RmlOgre {

class CompositorPassGeometry;
class WorkspacePopulator;
struct Geometry;

struct NodeType
{
	using ClearFunction = void (*)(Ogre::CompositorNode*);

	Ogre::IdString baseName;
	ClearFunction clear;
	std::vector<Ogre::CompositorNode*> nodes;

	void clearAll()
	{
		for(auto* node : this->nodes)
			this->clear(node);
	}
};

using Passes = std::vector<Pass>;

class Workspace
{
	static const std::size_t NUM_NODE_TYPES = std::variant_size_v<Pass>;

	std::array<NodeType, NUM_NODE_TYPES> nodeTypes;

	Ogre::SceneManager* sceneManager = nullptr;
	Ogre::Camera* camera = nullptr;
	Ogre::ObjectMemoryManager objectMemoryManager;
	Ogre::NodeMemoryManager nodeMemoryManager;
	// Storing Ogre::SceneNode and RenderObject in vectors only possible
	// because no SceneManager and manually controlling Ogre::RenderQueue.
	// No parent or child nodes allowed because of storing Ogre::SceneNode in vector
	std::vector<Ogre::SceneNode> sceneNodes;
	std::vector<RenderObject, Ogre::STLAllocator<RenderObject, Ogre::AlignAllocPolicy<>>> renderObjects;

	Ogre::TextureGpu* output = nullptr;
	Ogre::TextureGpu* background = nullptr;
	Ogre::CompositorWorkspace* workspace = nullptr;
	Ogre::CompositorWorkspaceDef* workspaceDef = nullptr;

	void buildWorkspace(const std::array<std::size_t, NUM_NODE_TYPES>& reservedNodes);
	void ensureWorkspaceNodes(const std::array<std::size_t, NUM_NODE_TYPES>& minNodes);

	void reserveRenderObjects(std::size_t capacity);
	void reserveSceneNodes(std::size_t capacity);
	void updateSceneNodes();

public:
	Workspace(
		Ogre::String name,
		Ogre::SceneManager* sceneManager,
		Ogre::TextureGpu* output,
		Ogre::TextureGpu* background);

	void updateProjectionMatrix();
	void clearAll();
	void populateWorkspace(const Passes& passes);

	RenderObject* addRenderObject();
	Ogre::SceneNode* addSceneNode();

	Ogre::String getNameStr() const;
	Ogre::IdString getName() const;
	Ogre::uint32 width() const;
	Ogre::uint32 height() const;
};

}

#endif // NIMBLE_RMLOGRE_WORKSPACE_HPP
