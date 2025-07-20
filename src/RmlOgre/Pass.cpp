#include "Pass.hpp"

#include "Compositor/CompositorPassRenderQuad.hpp"
#include "RenderInterface.hpp"
#include "Workspace.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>


using namespace nimble::RmlOgre;

void RenderQuadPass::writePass(
	Workspace& workspace,
	Ogre::CompositorNode* node,
	int passIndex
) const
{
	auto& passes = node->_getPasses();
	if(passes.empty())
		return;

	assert(dynamic_cast<CompositorPassRenderQuad*>(passes.at(passIndex)));
	auto* nodePass = static_cast<CompositorPassRenderQuad*>(passes.at(passIndex));

	if(this->material)
		nodePass->setMaterial(this->material);
	if(this->enableScissor)
	{
		float width = workspace.width();
		float height = workspace.height();
		nodePass->scissorRegion = Ogre::Vector4{
			this->scissorRegion.Left() / width,
			this->scissorRegion.Top() / height,
			this->scissorRegion.Width() / width,
			this->scissorRegion.Height() / height
		};
	}
	else
		nodePass->scissorRegion = Ogre::Vector4{0.0f, 0.0f, 1.0f, 1.0f};
}

CompositePass::CompositePass(
	int dst,
	bool noBlending,
	const RenderPassSettings& renderPassSettings
) :
	RenderQuadPass(nullptr, renderPassSettings),

	dst{dst}
{
	if(noBlending)
		this->material = Ogre::MaterialManager::getSingleton().getByName("Ogre/Copy/4xFP32");
}
