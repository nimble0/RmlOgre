#ifndef NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUAD_HPP
#define NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUAD_HPP

#include "CompositorPass.hpp"

#include <Compositor/OgreCompositorChannel.h>
#include <Compositor/OgreCompositorCommon.h>
#include <OgreMaterial.h>


namespace Ogre::v1 {

class Rectangle2D;

}

namespace nimble::RmlOgre {

class CompositorPassRenderQuadDef;

// Essentially a copy of Ogre::CompositorPassQuad but holds material instead of node definition
class CompositorPassRenderQuad : public CompositorPass
{
	CompositorPassRenderQuadDef const *mDefinition;

protected:
	Ogre::v1::Rectangle2D *mFsRect;
	Ogre::HlmsDatablock   *mDatablock;
	Ogre::MaterialPtr      mMaterial;
	Ogre::Pass            *mPass;
	Ogre::Camera          *mCamera;

	void analyzeBarriers( const bool bClearBarriers = true ) override;

public:
	CompositorPassRenderQuad(
		const CompositorPassRenderQuadDef *definition,
		Ogre::Camera *defaultCamera,
		Ogre::CompositorNode *parentNode,
		const Ogre::RenderTargetViewDef *rtv
	);
	~CompositorPassRenderQuad() override;

	void setMaterial(Ogre::MaterialPtr material);
	void setMaterial(Ogre::String material);
	void setHlmsMaterial(Ogre::String material);

	void execute( const Ogre::Camera *lodCamera ) override;

	/// Don't make this const (useful for compile-time multithreading errors)
	/// Pointer can be null if using HLMS
	Ogre::Pass   *getPass() { return mPass; }
	Ogre::Camera *getCamera() { return mCamera; }
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUAD_HPP
