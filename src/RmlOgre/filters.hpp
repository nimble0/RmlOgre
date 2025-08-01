#ifndef NIMBLE_RMLOGRE_FILTERS_HPP
#define NIMBLE_RMLOGRE_FILTERS_HPP

#include "FilterMaker.hpp"


namespace nimble::RmlOgre {

class BlurFilter : public Filter
{
	Ogre::MaterialPtr halfsample;
	Ogre::MaterialPtr blurH;
	Ogre::MaterialPtr blurV;
	int halfSamples;

public:
	BlurFilter(
		Ogre::MaterialPtr halfsample,
		Ogre::MaterialPtr blurH,
		Ogre::MaterialPtr blurV,
		int halfSamples
	) :
		halfsample{halfsample},
		blurH{blurH},
		blurV{blurV},
		halfSamples{halfSamples}
	{}

	void apply(RenderInterface& renderInterface) override;
};

class BlurFilterMaker : public FilterMaker
{
	Ogre::MaterialPtr halfsampleMaterial;
	Ogre::MaterialPtr blurMaterial;

public:
	// Must be same as:
	// NUM_WEIGHTS in media/scripts/material/Rml/GLSL/Blur_ps.glsl
	// NUM_WEIGHTS in media/scripts/material/Rml/HLSL/Blur_ps.hlsl
	static constexpr int NUM_WEIGHTS = 8;
	static constexpr float MAX_SCALED_SIGMA = 3.0f;

	BlurFilterMaker();
	BlurFilter make(float sigma);
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class DropShadowFilter : public Filter
{
	BlurFilter blurFilter;
	Ogre::MaterialPtr shadowMaterial;

public:
	DropShadowFilter(
		BlurFilter blurFilter,
		Ogre::MaterialPtr shadowMaterial
	) :
		blurFilter{blurFilter},
		shadowMaterial{shadowMaterial}
	{}

	void apply(RenderInterface& renderInterface) override;
};

class DropShadowFilterMaker : public FilterMaker
{
	BlurFilterMaker blurFilterMaker;
	Ogre::MaterialPtr shadowMaterial;
	Ogre::MaterialPtr blurlessShadowMaterial;

public:
	DropShadowFilterMaker();
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class OpacityFilterMaker : public FilterMaker
{
	Ogre::MaterialPtr baseMaterial;

public:
	OpacityFilterMaker();
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class ColourMatrixFilterMaker : public FilterMaker
{
	Ogre::MaterialPtr baseMaterial;

public:
	ColourMatrixFilterMaker();
	using FilterMaker::make;
	std::unique_ptr<Filter> make(const Ogre::Matrix4& matrix);
};

class BrightnessFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class ContrastFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class InvertFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class GrayscaleFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class SepiaFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class HueRotateFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class SaturateFilterMaker : public ColourMatrixFilterMaker
{
public:
	using ColourMatrixFilterMaker::make;
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

class MaskImageFilterMaker : public FilterMaker
{
	Ogre::MaterialPtr baseMaterial;

public:
	MaskImageFilterMaker();

	SingleMaterialFilter make(Ogre::TextureGpu* image);
	std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) override;
};

}

#endif // NIMBLE_RMLOGRE_FILTERS_HPP
