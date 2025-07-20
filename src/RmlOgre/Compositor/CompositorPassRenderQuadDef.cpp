#include "CompositorPassRenderQuadDef.hpp"

#include <Compositor/OgreCompositorNodeDef.h>


using namespace nimble::RmlOgre;

void CompositorPassRenderQuadDef::addQuadTextureSource( size_t texUnitIdx, const Ogre::String &textureName )
{
	if( textureName.find( "global_" ) == 0 )
	{
		mParentNodeDef->addTextureSourceName(
			textureName,
			0,
			Ogre::TextureDefinitionBase::TEXTURE_GLOBAL
		);
	}
	mTextureSources.push_back( QuadTextureSource( texUnitIdx, textureName ) );
}
