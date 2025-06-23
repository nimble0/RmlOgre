#include "CompositorPassGeometry.hpp"

#include "CompositorPassGeometryDef.hpp"
#include "RenderObject.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceListener.h>
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
	Ogre::CompositorPass(definition, parentNode),

	camera{camera}
{
	this->initialize(rtv);
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

	Ogre::SceneManager* sceneManager = this->camera->getSceneManager();
	sceneManager->_setCamerasInProgress(Ogre::CamerasInProgress(this->camera));
	sceneManager->_setCurrentCompositorPass(this);

	Ogre::RenderSystem* renderSystem = sceneManager->getDestinationRenderSystem();

	auto* def = static_cast<const CompositorPassGeometryDef*>(this->getDefinition());
	auto* renderQueue = def->renderQueue.get();

	renderQueue->renderPassPrepare(false, false);
	renderSystem->executeRenderPassDescriptorDelayedActions();
	renderQueue->render(
		renderSystem,
		RenderObject::RENDER_QUEUE_ID,
		RenderObject::RENDER_QUEUE_ID + 1,
		false,
		false);
	renderQueue->frameEnded();

	sceneManager->_setCurrentCompositorPass(nullptr);

	notifyPassPosExecuteListeners();

	profilingEnd();
}
