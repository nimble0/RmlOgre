#ifndef NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
#define NIMBLE_RMLOGRE_RENDERINTERFACE_HPP

#include "RenderObject.hpp"

#include <Math/Array/OgreObjectMemoryManager.h>
#include <Math/Array/OgreNodeMemoryManager.h>
#include <OgreHlmsDatablock.h>
#include <OgreMemoryStdAlloc.h>

#include <RmlUi/Core/RenderInterface.h>


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

class CompositorPassGeometryDef;
struct Geometry;

struct QueuedGeometry
{
	Geometry* geometry = nullptr;
	Rml::Vector2f translation;
	Ogre::HlmsUnlitDatablock* datablock = nullptr;
};

struct PassSettings
{
	bool enableScissor = false;
	Rml::Rectanglei scissorRegion;
};

struct Pass
{
	PassSettings settings;
	std::vector<QueuedGeometry> queue;
};

struct GeometryNode
{
	Ogre::CompositorNode* node = nullptr;
	CompositorPassGeometryDef* passDef = nullptr;
};

class RenderInterface : public Rml::RenderInterface
{
	Ogre::HlmsUnlit* hlms = nullptr;
	Ogre::HlmsMacroblock macroblock;
	Ogre::HlmsBlendblock blendBlock;
	Ogre::HlmsUnlitDatablock* noTextureDatablock;

	Ogre::CompositorWorkspace* workspace = nullptr;
	Ogre::CompositorWorkspaceDef* workspaceDef = nullptr;
	Ogre::TextureGpu* output = nullptr;
	std::vector<GeometryNode> geometryNodes;

	std::vector<Pass> passes;

	int datablockId = 0;
	std::vector<Rml::CompiledGeometryHandle> releaseGeometry;
	std::vector<Rml::TextureHandle> releaseTextures;

	Ogre::SceneManager* sceneManager = nullptr;
	Ogre::Camera* camera = nullptr;
	Ogre::ObjectMemoryManager objectMemoryManager;
	Ogre::NodeMemoryManager nodeMemoryManager;
	// Storing Ogre::SceneNode and RmlUiRenderObject in vectors only possible
	// because no SceneManager and manually controlling Ogre::RenderQueue.
	// No parent or child nodes allowed because of storing Ogre::SceneNode in vector
	std::vector<Ogre::SceneNode> sceneNodes;
	std::vector<RenderObject, Ogre::STLAllocator<RenderObject, Ogre::AlignAllocPolicy<>>> renderObjects;

	void buildWorkspace(std::size_t numGeometryNodes);
	void populateWorkspace();

	void releaseBufferedGeometry();
	void releaseBufferedTextures();

public:
	RenderInterface(
		const Ogre::String& name,
		Ogre::SceneManager* sceneManager,
		Rml::Vector2i resolution);

	// Added for consistency with other render interfaces
	void BeginFrame() {}
	void EndFrame();

	void SetViewport(Rml::Vector2i dimensions);
	void SetViewport(int width, int height);

	Ogre::TextureGpu* GetOutput() { return this->output; }
	const Ogre::TextureGpu* GetOutput() const { return this->output; }

	Rml::CompiledGeometryHandle CompileGeometry(
		Rml::Span<const Rml::Vertex> vertices,
		Rml::Span<const int> indices
	) override;
	void RenderGeometry(
		Rml::CompiledGeometryHandle geometry,
		Rml::Vector2f translation,
		Rml::TextureHandle texture
	) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(
		Rml::Vector2i& texture_dimensions,
		const Rml::String& source
	) override;
	Rml::TextureHandle GenerateTexture(
		Rml::Span<const Rml::byte> source,
		Rml::Vector2i source_dimensions
	) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;
};

}

#endif // NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
