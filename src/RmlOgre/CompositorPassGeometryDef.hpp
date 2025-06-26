#ifndef NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRYDEF_HPP
#define NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRYDEF_HPP

#include <Compositor/Pass/OgreCompositorPassDef.h>

#include <OgreRenderQueue.h>

#include <memory>


namespace nimble::RmlOgre {

class CompositorPassGeometryDef : public Ogre::CompositorPassDef
{
public:
	CompositorPassGeometryDef(Ogre::CompositorTargetDef* parentTargetDef) :
		Ogre::CompositorPassDef(Ogre::PASS_CUSTOM, parentTargetDef)
	{
		mProfilingId = "RmlOgre Rendering";
	}
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRYDEF_HPP
