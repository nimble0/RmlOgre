#include "CompositorPass.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreException.h>
#include <OgreLogManager.h>
#include <OgreRenderSystem.h>


using namespace nimble::RmlOgre;

// Copy of CompositorPass::setRenderPassDescToCurrent
// Edited to allowing setting scissor region on pass instead of pass definition
void CompositorPass::setRenderPassDescToCurrent()
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
