#include "shaders.hpp"

#include <OgreGpuProgramParams.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/DecorationTypes.h>


using namespace nimble::RmlOgre;

namespace {

Ogre::Vector4 to_ogre(Rml::ColourbPremultiplied c)
{
	Ogre::Vector4 result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c[i]);
	return result;
}

Ogre::Vector2 to_ogre(Rml::Vector2f v)
{
	return Ogre::Vector2{v.x, v.y};
}

}


GradientMaker::GradientMaker(GradientMaker::Type type) :
	type{type}
{
	this->baseMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Gradient");
}

Ogre::MaterialPtr GradientMaker::makeGradient(
	const Rml::Dictionary& parameters,
	Ogre::Vector2& origin,
	Ogre::Vector2& scale)
{
	auto material = this->baseMaterial->clone("");
	material->load();
	auto fragmentProgramParameters = material
		->getBestTechnique()
		->getPass(0)
		->getFragmentProgramParameters();

	fragmentProgramParameters->setNamedConstant("gradientType", int(this->type));
	fragmentProgramParameters->setNamedConstant("repeating", int(Rml::Get(parameters, "repeating", false)));
	fragmentProgramParameters->setNamedConstant("origin", origin);
	fragmentProgramParameters->setNamedConstant("scale", scale);

	auto stopListIter = parameters.find("color_stop_list");
	assert(stopListIter  != parameters.end()
		&& stopListIter ->second.GetType() == Rml::Variant::COLORSTOPLIST);
	const Rml::ColorStopList& stopList = stopListIter->second.GetReference<Rml::ColorStopList>();
	int stopListSize = stopList.size();

	std::array<float, MAX_STOP_COLOURS> stopPositions;
	std::array<float, MAX_STOP_COLOURS * 4> stopColours;
	for(int i = 0, j = 0; i < stopListSize; ++i)
	{
		stopPositions[i] = stopList[i].position.number;
		auto colour = to_ogre(stopList[i].color);
		for(int k = 0; k < 4; ++k)
			stopColours[j++] = colour[k];
	}

	fragmentProgramParameters->setNamedConstant("numStops", stopListSize);
	fragmentProgramParameters->setNamedConstant(
		"stopPositions",
		stopPositions.data(),
		4);
	fragmentProgramParameters->setNamedConstant(
		"stopColours",
		stopColours.data(),
		stopColours.size() / 4);

	return material;
}


Ogre::MaterialPtr LinearGradientMaker::make(const Rml::Dictionary& parameters)
{
	auto p0 = Rml::Get(parameters, "p0", Rml::Vector2f(0.0f));
	auto p1 = Rml::Get(parameters, "p1", Rml::Vector2f(0.0f));
	auto origin = to_ogre(p0);
	auto scale = to_ogre(p1) - origin;
	return this->makeGradient(parameters, origin, scale);
}

Ogre::MaterialPtr RadialGradientMaker::make(const Rml::Dictionary& parameters)
{
	auto center = Rml::Get(parameters, "center", Rml::Vector2f(0.0f));
	auto radius = Rml::Get(parameters, "radius", Rml::Vector2f(1.0f));
	auto origin = to_ogre(center);
	auto scale = Ogre::Vector2(1.0f) / to_ogre(radius);
	return this->makeGradient(parameters, origin, scale);
}

Ogre::MaterialPtr ConicGradientMaker::make(const Rml::Dictionary& parameters)
{
	auto center = Rml::Get(parameters, "center", Rml::Vector2f(0.0f));
	auto angle = Rml::Get(parameters, "angle", 0.0f);
	auto origin = to_ogre(center);
	auto scale = Ogre::Vector2{Ogre::Math::Cos(angle), Ogre::Math::Sin(angle)};
	return this->makeGradient(parameters, origin, scale);
}
