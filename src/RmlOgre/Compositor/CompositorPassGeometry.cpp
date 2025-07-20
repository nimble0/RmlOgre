#include "CompositorPassGeometry.hpp"

#include "CompositorPassGeometryDef.hpp"
#include "RmlOgre/RenderObject.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceListener.h>
#include <OgreLogManager.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreRenderSystem.h>
#include <OgreRenderQueue.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>

#include <OgreCamera.h>


using namespace nimble::RmlOgre;

CompositorPassGeometry::CompositorPassGeometry(
	const CompositorPassGeometryDef* definition,
	Ogre::Camera* camera,
	const Ogre::RenderTargetViewDef* rtv,
	Ogre::CompositorNode* parentNode
) :
	CompositorPass(definition, parentNode),

	camera{camera}
{
	this->initialize(rtv);

	this->renderQueue = std::make_unique<Ogre::RenderQueue>(
		Ogre::Root::getSingleton().getHlmsManager(),
		this->camera->getSceneManager(),
		Ogre::Root::getSingleton().getRenderSystem()->getVaoManager());
	this->renderQueue->setSortRenderQueue(
		RenderObject::RENDER_QUEUE_ID,
		Ogre::RenderQueue::RqSortMode::DisableSort);
}

void CompositorPassGeometry::execute(const Ogre::Camera* lodCamera)
{
	//Execute a limited number of times?
	if( mNumPassesLeft != std::numeric_limits<Ogre::uint32>::max() )
	{
		if( !mNumPassesLeft )
			return;
		--mNumPassesLeft;
	}

	profilingBegin();

	notifyPassEarlyPreExecuteListeners();

	executeResourceTransitions();

	setRenderPassDescToCurrent();

	//Fire the listener in case it wants to change anything
	notifyPassPreExecuteListeners();

	this->camera->setCustomProjectionMatrix(true, this->projectionMatrix);
	Ogre::SceneManager* sceneManager = this->camera->getSceneManager();
	sceneManager->_setCamerasInProgress(Ogre::CamerasInProgress(this->camera));
	sceneManager->_setCurrentCompositorPass(this);

	Ogre::RenderSystem* renderSystem = sceneManager->getDestinationRenderSystem();

	renderSystem->setStencilBufferParams(this->stencilRefValue, renderSystem->getStencilBufferParams());
	this->renderQueue->renderPassPrepare(false, false);
	renderSystem->executeRenderPassDescriptorDelayedActions();
	this->renderQueue->render(
		renderSystem,
		RenderObject::RENDER_QUEUE_ID,
		RenderObject::RENDER_QUEUE_ID + 1,
		false,
		false);
	this->renderQueue->frameEnded();

	sceneManager->_setCurrentCompositorPass(nullptr);

	notifyPassPosExecuteListeners();

	profilingEnd();
}
