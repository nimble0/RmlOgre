#ifndef NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP
#define NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP

#include <Compositor/Pass/OgreCompositorPass.h>
#include <OgreMatrix4.h>
#include <OgrePrerequisites.h>

#include <memory>


namespace nimble::RmlOgre {

class CompositorPassGeometryDef;

class CompositorPassGeometry : public Ogre::CompositorPass
{
protected:
	Ogre::Camera* camera = nullptr;

public:
	Ogre::Vector4 scissorRegion = Ogre::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Ogre::Matrix4 projectionMatrix = Ogre::Matrix4::IDENTITY;
	Ogre::uint32  stencilRefValue = 0;

	std::unique_ptr<Ogre::RenderQueue> renderQueue;

	CompositorPassGeometry(
		const CompositorPassGeometryDef* definition,
		Ogre::Camera* camera,
		const Ogre::RenderTargetViewDef* rtv,
		Ogre::CompositorNode* parentNode);

	void setRenderPassDescToCurrent();
	void execute(const Ogre::Camera* lodCamera) override;
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITORPASSGEOMETRY_HPP
