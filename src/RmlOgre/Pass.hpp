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

struct StartLayerPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/StartLayer";

	int moveOut = -1;

	StartLayerPass(int moveOut) :
		moveOut{moveOut}
	{}

	void addExtraConnections(NodeConnectionMap& connections) const override
	{
		assert(this->moveOut >= 0);
		connections.setOut(this->moveOut, 3);
	}
};

struct SwapPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/Swap";

	int swapIn = -1;
	int swapOut = -1;

	SwapPass(int swapIn, int swapOut = -1) :
		swapIn{swapIn},
		swapOut{swapOut}
	{}

	void addExtraConnections(NodeConnectionMap& connections) const override
	{
		assert(this->swapIn >= 0);
		connections.setIn(this->swapIn, 3);
		connections.setOut(this->swapOut, 3);
	}
};

struct CopyPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/Copy";

	int copyOut = -1;

	CopyPass(int copyOut) :
		copyOut{copyOut}
	{}

	void addExtraConnections(NodeConnectionMap& connections) const override
	{
		assert(this->copyOut >= 0);
		connections.setOut(this->copyOut, 3);
	}
};

struct RenderQuadPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderQuad";

	Ogre::MaterialPtr material;

	bool enableScissor = false;
	Rml::Rectanglei scissorRegion;

	RenderQuadPass(
		Ogre::MaterialPtr material,
		const RenderPassSettings& renderPassSettings = RenderPassSettings{}
	) :
		material{material},
		enableScissor{renderPassSettings.enableScissor},
		scissorRegion{renderPassSettings.scissorRegion}
	{}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node,
		int passIndex
	) const;

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		this->writePass(workspace, node, 0);
	}
};

struct CompositePass : RenderQuadPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/Composite";

	int dst = -1;

	CompositePass(
		int dst,
		bool noBlending,
		const RenderPassSettings& renderPassSettings);

	void addExtraConnections(NodeConnectionMap& connections) const override
	{
		assert(this->dst >= 0);
		connections.setIn(this->dst, 3);
	}
};

struct CompositeWithStencilPass : CompositePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/CompositeWithStencil";

	using CompositePass::CompositePass;

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const override
	{
		RenderQuadPass::writePass(workspace, node, 1);
	}
};

struct ClearSecondaryPass : BasePass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/ClearSecondary";
};

struct RenderToTexturePass : RenderQuadPass
{
	static constexpr const char* BASE_NODE_NAME = "Rml/RenderToTexture";

	int renderTexture = -1;

	RenderToTexturePass(
		int renderTexture,
		const RenderPassSettings& renderPassSettings
	) :
		RenderQuadPass(nullptr, renderPassSettings),

		renderTexture{renderTexture}
	{}

	void addExtraConnections(NodeConnectionMap& connections) const override
	{
		assert(this->renderTexture >= 0);
		connections.setExternal(this->renderTexture, 3);
	}

	void writePass(
		Workspace& workspace,
		Ogre::CompositorNode* node
	) const;
};

using Pass = std::variant<
	NullPass,
	RenderPass,
	RenderWithStencilPass,
	RenderToStencilSetPass,
	RenderToStencilSetInversePass,
	RenderToStencilIntersectPass,
	StartLayerPass,
	SwapPass,
	CopyPass,
	CompositePass,
	CompositeWithStencilPass,
	RenderQuadPass,
	ClearSecondaryPass,
	RenderToTexturePass
>;

}

#endif // NIMBLE_RMLOGRE_PASS_HPP
