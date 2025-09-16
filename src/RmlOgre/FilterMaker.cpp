#include "FilterMaker.hpp"

#include "RenderInterface.hpp"

#include <OgreTechnique.h>


using namespace nimble::RmlOgre;

void SingleMaterialFilter::apply(RenderInterface& renderInterface)
{
	renderInterface.addPass(RenderQuadPass(this->material, renderInterface.currentRenderPassSettings()));
}
