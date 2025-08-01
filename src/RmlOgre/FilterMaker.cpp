#include "FilterMaker.hpp"

#include "RenderInterface.hpp"

#include <OgreTechnique.h>


using namespace nimble::RmlOgre;

void SingleMaterialFilter::apply(RenderInterface& renderInterface)
{
	renderInterface.addPass(RenderQuadPass(this->material, renderInterface.currentRenderPassSettings()));
}

void SingleMaterialFilter::release(RenderInterface& renderInterface)
{
	auto* pass = this->material
		->getBestTechnique()
		->getPass(0);
	for(auto& textureUnit : pass->getTextureUnitStateIterator())
	{
		auto* texture = textureUnit->_getTexturePtr();
		renderInterface.releaseRenderTexture(texture);
	}
}
