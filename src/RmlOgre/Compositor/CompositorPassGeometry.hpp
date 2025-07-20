#ifndef NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRY_HPP
#define NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRY_HPP

#include "CompositorPass.hpp"

#include <OgreMatrix4.h>
#include <OgrePrerequisites.h>

#include <memory>


namespace nimble::RmlOgre {

class CompositorPassGeometryDef;

class CompositorPassGeometry : public CompositorPass
{
protected:
	Ogre::Camera* camera = nullptr;

public:
	Ogre::Matrix4 projectionMatrix = Ogre::Matrix4::IDENTITY;

	std::unique_ptr<Ogre::RenderQueue> renderQueue;

	CompositorPassGeometry(
		const CompositorPassGeometryDef* definition,
		Ogre::Camera* camera,
		const Ogre::RenderTargetViewDef* rtv,
		Ogre::CompositorNode* parentNode);

	void execute(const Ogre::Camera* lodCamera) override;
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSGEOMETRY_HPP
