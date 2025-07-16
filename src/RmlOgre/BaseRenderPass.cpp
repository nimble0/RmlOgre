#include "BaseRenderPass.hpp"

#include "CompositorPassGeometry.hpp"
#include "CompositorPassGeometryDef.hpp"
#include "RenderObject.hpp"
#include "Workspace.hpp"
#include "geometry.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <Hlms/Unlit/OgreHlmsUnlitDatablock.h>
#include <OgreRenderQueue.h>


using namespace nimble::RmlOgre;

void BaseRenderPass::clearNodePass(Ogre::CompositorNode* node, std::size_t passIndex)
{
	auto& passes = node->_getPasses();
	if(passes.empty())
		return;

	assert(dynamic_cast<CompositorPassGeometry*>(passes.at(passIndex)));
	auto* nodePass = static_cast<CompositorPassGeometry*>(passes.at(passIndex));
	nodePass->renderQueue->clear();
}

void BaseRenderPass::writeRenderPass(
	Workspace& workspace,
	Ogre::CompositorNode* node,
	std::size_t passIndex
) const
{
	auto& passes = node->_getPasses();
	if(passes.empty())
		return;

	assert(dynamic_cast<CompositorPassGeometry*>(passes.at(passIndex)));
	auto* nodePass = static_cast<CompositorPassGeometry*>(passes.at(passIndex));

	for(auto& queueObject : this->queue)
	{
		auto* object = workspace.addRenderObject();
		object->setVao(queueObject.vao);
		object->setDatablock(queueObject.datablock);

		auto* sceneNode = workspace.addSceneNode();
		sceneNode->setPosition(Ogre::Vector3{queueObject.translation.x, queueObject.translation.y, 0.0f});
		sceneNode->attachObject(object);

		for(auto renderable : object->mRenderables)
		{
			nodePass->renderQueue->addRenderableV2(
				0,
				RenderObject::RENDER_QUEUE_ID,
				false,
				renderable,
				object);
		}
	}

	if(this->settings.enableScissor)
	{
		float width = workspace.width();
		float height = workspace.height();
		nodePass->scissorRegion = Ogre::Vector4{
			this->settings.scissorRegion.Left() / width,
			this->settings.scissorRegion.Top() / height,
			this->settings.scissorRegion.Width() / width,
			this->settings.scissorRegion.Height() / height
		};
	}
	else
		nodePass->scissorRegion = Ogre::Vector4{0.0f, 0.0f, 1.0f, 1.0f};
}
