#ifndef NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP
#define NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP

#include <OgrePrerequisites.h>
#include <Compositor/Pass/OgreCompositorPass.h>


namespace nimble::RmlOgre {

class CompositorPassGeometryDef;

class CompositorPassGeometry : public Ogre::CompositorPass
{
protected:
	Ogre::Camera* camera = nullptr;

public:
	CompositorPassGeometry(
		const CompositorPassGeometryDef* definition,
		Ogre::Camera* camera,
		const Ogre::RenderTargetViewDef* rtv,
		Ogre::CompositorNode* parentNode);

	void execute(const Ogre::Camera* lodCamera) override;
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP
