#ifndef NIMBLE_RMLOGRE_BASERENDERPASS_HPP
#define NIMBLE_RMLOGRE_BASERENDERPASS_HPP

#include "BasePass.hpp"
#include "Material.hpp"
#include "RenderObject.hpp"

#include <RmlUi/Core/RenderInterface.h>


namespace Ogre {

class CompositorNode;
class HlmsUnlitDatablock;
struct VertexArrayObject;

}

namespace nimble::RmlOgre {

class CompositorPassGeometry;
class Workspace;
struct Geometry;

struct QueuedGeometry
{
	Ogre::VertexArrayObject* vao = nullptr;
	Rml::Vector2f translation;
	Material material;
};

struct RenderPassSettings
{
	bool enableScissor = false;
	Rml::Rectanglei scissorRegion;
	Ogre::Matrix4 transform = Ogre::Matrix4::IDENTITY;
	bool enableStencil = false;
	Ogre::uint32 stencilRefValue = 0;

	bool operator==(const RenderPassSettings& b) const
	{
		return this->enableScissor == b.enableScissor
			&& this->scissorRegion == b.scissorRegion
			&& this->transform == b.transform
			&& this->enableStencil == b.enableStencil
			&& this->stencilRefValue == b.stencilRefValue;
	}
	bool operator!=(const RenderPassSettings& b) const
	{
		return !(*this == b);
	}
};

struct BaseRenderPass : BasePass
{
	RenderPassSettings settings;
	std::vector<QueuedGeometry> queue;

	int numRenderObjects() const override
	{
		return this->queue.size();
	}

	static void clearNodePass(Ogre::CompositorNode* node, std::size_t passIndex);

	virtual void writeRenderPass(
		Workspace& workspace,
		Ogre::CompositorNode* node,
		std::size_t passIndex
	) const;
};

}

#endif // NIMBLE_RMLOGRE_BASERENDERPASS_HPP
