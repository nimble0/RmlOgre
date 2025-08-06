#ifndef NIMBLE_RMLOGRE_SHADERMAKER_HPP
#define NIMBLE_RMLOGRE_SHADERMAKER_HPP

#include <OgreMaterial.h>

#include <RmlUi/Core/Dictionary.h>


namespace nimble::RmlOgre {

class ShaderMaker
{
public:
	virtual ~ShaderMaker() {}
	virtual Ogre::MaterialPtr make(const Rml::Dictionary& parameters) = 0;
};

}

#endif // NIMBLE_RMLOGRE_SHADERMAKER_HPP
