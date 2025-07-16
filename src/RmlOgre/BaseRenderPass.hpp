#ifndef NIMBLE_RMLOGRE_BASERENDERPASS_HPP
#define NIMBLE_RMLOGRE_BASERENDERPASS_HPP

#include "BasePass.hpp"

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
	Ogre::HlmsUnlitDatablock* datablock = nullptr;
};

struct RenderPassSettings
{
	bool enableScissor = false;
	Rml::Rectanglei scissorRegion;

	bool operator==(const RenderPassSettings& b) const
	{
		return this->enableScissor == b.enableScissor
			&& this->scissorRegion == b.scissorRegion;
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
