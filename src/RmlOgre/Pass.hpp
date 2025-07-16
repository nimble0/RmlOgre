#ifndef NIMBLE_RMLOGRE_PASS_HPP
#define NIMBLE_RMLOGRE_PASS_HPP

#include "BaseRenderPass.hpp"
#include "RenderObject.hpp"

#include <OgreHlmsDatablock.h>

#include <RmlUi/Core/RenderInterface.h>

#include <variant>


namespace nimble::RmlOgre {

class Workspace;

struct NullPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "";
};

struct RenderPass : BaseRenderPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/Render";

	static void clearNode(Ogre::CompositorNode* node)
	{
		BaseRenderPass::clearNodePass(node, 0);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writeRenderPass(workspace, node, 0);
	}
};

using Pass = std::variant<
	NullPass,
	RenderPass
>;

}

#endif // NIMBLE_RMLOGRE_PASS_HPP
