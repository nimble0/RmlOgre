#ifndef NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUADDEF_HPP
#define NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUADDEF_HPP

#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h>


namespace nimble::RmlOgre {

class CompositorPassRenderQuadDef : public Ogre::CompositorPassDef
{
public:
	using QuadTextureSource = Ogre::CompositorPassQuadDef::QuadTextureSource;
	using TextureSources = Ogre::CompositorPassQuadDef::TextureSources;
	using FrustumCorners = Ogre::CompositorPassQuadDef::FrustumCorners;

protected:
	TextureSources           mTextureSources;
	Ogre::CompositorNodeDef* mParentNodeDef;

public:
	/** Whether to use a full screen quad or triangle. (default: false). Note that you may not
		always get the triangle (for example, if you ask for WORLD_SPACE_CORNERS)
	*/
	bool mUseQuad;

	/** When true, the user is telling Ogre this pass just performs a custom FSAA resolve filter.
		Hence we should skip this pass for those APIs that don't support explicit resolving
		TODO: Not really implemented yet!!!
	@remarks
		See TextureDefinitionBase::TextureDefinition::fsaaExplicitResolve
	*/
	bool mIsResolve;

	/** When true, the camera will be rotated 90°, -90° or 180° depending on the value of
		mRtIndex and then restored to its original rotation after we're done.
	*/
	bool mCameraCubemapReorient;

	/// When true, Ogre will check all bound textures in the material
	/// to see if they were properly transitioned to ResourceLayout::Texture,
	/// not just the textures referenced by the compositor
	bool mAnalyzeAllTextureLayouts;

	bool         mMaterialIsHlms;  ///< If true, mMaterialName is an Hlms material
	Ogre::String mMaterialName;

	/** Type of frustum corners to pass in the quad normals.
		mCameraName contains which camera's frustum to pass
	*/
	FrustumCorners mFrustumCorners;
	Ogre::IdString mCameraName;

	CompositorPassRenderQuadDef( Ogre::CompositorNodeDef *parentNodeDef, Ogre::CompositorTargetDef *parentTargetDef ) :
		CompositorPassDef( Ogre::PASS_CUSTOM, parentTargetDef ),
		mParentNodeDef( parentNodeDef ),
		mUseQuad( false ),
		mIsResolve( false ),
		mCameraCubemapReorient( false ),
		mAnalyzeAllTextureLayouts( false ),
		mMaterialIsHlms( false ),
		mFrustumCorners( FrustumCorners::NO_CORNERS )
	{
		mProfilingId = "rml/render_quad";

		// Unfortunately have to hard-code the texture source for custom pass
		this->addQuadTextureSource(0, "rt0");
	}

	/** Indicates the pass to change the texture units to use the specified texture sources.
		See CompositorPassQuadDef::QuadTextureSource for params
	*/
	void addQuadTextureSource( size_t texUnitIdx, const Ogre::String &textureName );

	const TextureSources &getTextureSources() const { return mTextureSources; }
};

}

#endif // NIMBLE_RMLOGRE_COMPOSITOR_COMPOSITORPASSRENDERQUADDEF_HPP
