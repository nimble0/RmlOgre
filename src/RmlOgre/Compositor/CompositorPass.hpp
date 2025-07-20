#ifndef NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASS_HPP
#define NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASS_HPP

#include <Compositor/Pass/OgreCompositorPass.h>


namespace nimble::RmlOgre {

class CompositorPass : public Ogre::CompositorPass
{
public:
	Ogre::Vector4 scissorRegion = Ogre::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Ogre::uint32  stencilRefValue = 0;

	using Ogre::CompositorPass::CompositorPass;

	void setRenderPassDescToCurrent();
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASS_HPP
