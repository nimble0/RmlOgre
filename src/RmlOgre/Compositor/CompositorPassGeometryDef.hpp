#ifndef NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRYDEF_HPP
#define NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRYDEF_HPP

#include <Compositor/Pass/OgreCompositorPassDef.h>


namespace nimble::RmlOgre {

class CompositorPassGeometryDef : public Ogre::CompositorPassDef
{
public:
	CompositorPassGeometryDef(Ogre::CompositorTargetDef* parentTargetDef) :
		Ogre::CompositorPassDef(Ogre::PASS_CUSTOM, parentTargetDef)
	{
		mProfilingId = "rml/geometry";
	}
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRYDEF_HPP
