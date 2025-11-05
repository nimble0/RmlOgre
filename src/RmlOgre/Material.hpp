#ifndef NIMBLE_RMLOGRE_MATERIAL_HPP
#define NIMBLE_RMLOGRE_MATERIAL_HPP

#include "RenderObject.hpp"

#include <OgreMaterial.h>


namespace Ogre {

class HlmsDatablock;

}

namespace nimble::RmlOgre {

struct Material
{
	Ogre::MaterialPtr    material;
	Ogre::HlmsDatablock* datablock = nullptr;
	Ogre::TextureGpu*    textureDependency = nullptr;
	Ogre::uint32         hash = 0;
	Ogre::uint32         casterHash = 0;

	bool needsHashing() const
	{
		return this->hash == 0 || this->casterHash == 0;
	}
	void calculateHlmsHash();
};

}

#endif // NIMBLE_RMLOGRE_MATERIAL_HPP
