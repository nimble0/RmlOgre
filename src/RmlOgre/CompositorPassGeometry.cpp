#include "CompositorPassGeometry.hpp"

#include "CompositorPassGeometryDef.hpp"
#include "RenderObject.hpp"

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
	Ogre::CompositorPass(definition, parentNode),

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

// Copied from CompositorPass
// Edited to use pass specific scissor region
void CompositorPassGeometry::setRenderPassDescToCurrent()
{
	using namespace Ogre;

	const auto& mDefinition = this->getDefinition();

	RenderSystem *renderSystem = mParentNode->getRenderSystem();
	if( mDefinition->mSkipLoadStoreSemantics )
	{
		OGRE_ASSERT_MEDIUM(
			( mDefinition->getType() == PASS_QUAD || mDefinition->getType() == PASS_SCENE ||
				mDefinition->getType() == PASS_CUSTOM ) &&
			"mSkipLoadStoreSemantics is only intended for use with pass quad, scene & custom" );

		const RenderPassDescriptor *currRenderPassDesc = renderSystem->getCurrentPassDescriptor();
		if( !currRenderPassDesc )
		{
			LogManager::getSingleton().logMessage(
				"mSkipLoadStoreSemantics was requested but there is no active render pass "
				"descriptor. Disable this setting for pass " +
					mDefinition->mProfilingId + " with RT " +
					mDefinition->getParentTargetDef()->getRenderTargetNameStr() +
					"\n - Check the contents of CompositorPass::mResourceTransitions to see if you "
					"can "
					"move them to a barrier pass",
				LML_CRITICAL );
			OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
							"mSkipLoadStoreSemantics was requested but there is no active render pass "
							"descriptor.\nSee Ogre.log for details",
							"CompositorPass::setRenderPassDescToCurrent" );
		}

		return;
	}

	CompositorWorkspace *workspace = mParentNode->getWorkspace();
	uint8 workspaceVpMask = workspace->getViewportModifierMask();

	bool applyModifier = ( workspaceVpMask & mDefinition->mViewportModifierMask ) != 0;
	Vector4 vpModifier = applyModifier ? workspace->getViewportModifier() : Vector4( 0, 0, 1, 1 );

	const uint32 numViewports = mDefinition->mNumViewports;
	Vector4 vpSize[16];
	Vector4 scissors[16];

	Vector4 scissorRegion_ = this->scissorRegion;
	scissorRegion_.x += vpModifier.x;
	scissorRegion_.y += vpModifier.y;
	scissorRegion_.z *= vpModifier.z;
	scissorRegion_.w *= vpModifier.w;

	scissorRegion_.x = std::clamp(scissorRegion_.x, 0.0f, 1.0f);
	scissorRegion_.y = std::clamp(scissorRegion_.y, 0.0f, 1.0f);
	scissorRegion_.z = std::clamp(scissorRegion_.z, 0.0f, 1.0f);
	scissorRegion_.w = std::clamp(scissorRegion_.w, 0.0f, 1.0f);

	for( size_t i = 0; i < numViewports; ++i )
	{
		Real left = mDefinition->mVpRect[i].mVpLeft + vpModifier.x;
		Real top = mDefinition->mVpRect[i].mVpTop + vpModifier.y;
		Real width = mDefinition->mVpRect[i].mVpWidth * vpModifier.z;
		Real height = mDefinition->mVpRect[i].mVpHeight * vpModifier.w;

		vpSize[i] = Vector4( left, top, width, height );
		scissors[i] = scissorRegion_;
	}

	renderSystem->beginRenderPassDescriptor(
		mRenderPassDesc, mAnyTargetTexture, mAnyMipLevel, vpSize, scissors, numViewports,
		mDefinition->mIncludeOverlays, mDefinition->mWarnIfRtvWasFlushed );
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
