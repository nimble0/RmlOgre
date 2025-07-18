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

struct RenderWithStencilPass : BaseRenderPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderWithStencil";

	static void clearNode(Ogre::CompositorNode* node)
	{
		RenderPass::clearNodePass(node, 1);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writeRenderPass(workspace, node, 1);
	}
};

struct RenderToStencilSetPass : BaseRenderPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderToStencilSet";

	static void clearNode(Ogre::CompositorNode* node)
	{
		RenderPass::clearNodePass(node, 2);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writeRenderPass(workspace, node, 2);
	}
};

struct RenderToStencilSetInversePass : BaseRenderPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderToStencilSetInverse";

	static void clearNode(Ogre::CompositorNode* node)
	{
		RenderPass::clearNodePass(node, 2);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writeRenderPass(workspace, node, 2);
	}
};

struct RenderToStencilIntersectPass : BaseRenderPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderToStencilIntersect";

	static void clearNode(Ogre::CompositorNode* node)
	{
		RenderPass::clearNodePass(node, 1);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writeRenderPass(workspace, node, 1);
	}
};

using Pass = std::variant<
	NullPass,
	RenderPass,
	RenderWithStencilPass,
	RenderToStencilSetPass,
	RenderToStencilSetInversePass,
	RenderToStencilIntersectPass
>;

}

#endif // NIMBLE_RMLOGRE_PASS_HPP
