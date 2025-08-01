#ifndef NIMBLE_RMLOGRE_FILTERMAKER_HPP
#define NIMBLE_RMLOGRE_FILTERMAKER_HPP

#include <OgreMaterial.h>

#include <RmlUi/Core/Dictionary.h>

#include <memory>


namespace nimble::RmlOgre {

class RenderInterface;

class Filter
{
public:
	virtual ~Filter() {}
	virtual void apply(RenderInterface& renderInterface) = 0;
	virtual void release(RenderInterface& renderInterface) {}
};

class SingleMaterialFilter : public Filter
{
	Ogre::MaterialPtr material;

public:
	SingleMaterialFilter(Ogre::MaterialPtr material) :
		material{material}
	{}

	void apply(RenderInterface& renderInterface) override;
	void release(RenderInterface& renderInterface) override;
};

class FilterMaker
{
public:
	virtual ~FilterMaker() {}
	virtual std::unique_ptr<Filter> make(const Rml::Dictionary& parameters) = 0;
};

}

#endif // NIMBLE_RMLOGRE_FILTERMAKER_HPP
