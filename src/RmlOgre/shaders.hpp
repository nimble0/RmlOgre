#ifndef NIMBLE_RMLOGRE_SHADERS_HPP
#define NIMBLE_RMLOGRE_SHADERS_HPP

#include "ShaderMaker.hpp"


namespace nimble::RmlOgre {

class GradientMaker : public ShaderMaker
{
public:
	enum class Type
	{
		LINEAR,
		RADIAL,
		CONIC
	};

	// Must be same as:
	// MAX_STOP_COLOURS in media/scripts/material/Rml/GLSL/Gradient_ps.glsl
	// MAX_STOP_COLOURS in media/scripts/material/Rml/HLSL/Gradient_ps.hlsl
	static const int MAX_STOP_COLOURS = 16;

private:
	Type type;
	Ogre::MaterialPtr baseMaterial;

public:
	GradientMaker(Type type);
	Ogre::MaterialPtr makeGradient(
		const Rml::Dictionary& parameters,
		Ogre::Vector2& origin,
		Ogre::Vector2& scale);
};


class LinearGradientMaker : public GradientMaker
{
public:
	LinearGradientMaker() :
		GradientMaker(GradientMaker::Type::LINEAR)
	{}
	Ogre::MaterialPtr make(const Rml::Dictionary& parameters) override;
};

class RadialGradientMaker : public GradientMaker
{
public:
	RadialGradientMaker() :
		GradientMaker(GradientMaker::Type::RADIAL)
	{}
	Ogre::MaterialPtr make(const Rml::Dictionary& parameters) override;
};

class ConicGradientMaker : public GradientMaker
{
public:
	ConicGradientMaker() :
		GradientMaker(GradientMaker::Type::CONIC)
	{}
	Ogre::MaterialPtr make(const Rml::Dictionary& parameters) override;
};

}

#endif // NIMBLE_RMLOGRE_SHADERS_HPP
