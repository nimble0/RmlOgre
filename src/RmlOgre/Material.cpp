#include "Material.hpp"

#include "RenderObject.hpp"
#include "geometry.hpp"

#include <OgreHlms.h>
#include <OgreHlmsDatablock.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>

#include <array>


using namespace nimble::RmlOgre;

namespace {

RenderObject make_empty_render_object()
{
	RenderObject emptyRenderObject{};
	static std::array<const Rml::Vertex, 1> vertices{Rml::Vertex{}};
	static std::array<const int, 1> indices{0};
	emptyRenderObject.setVao(create_vao(
		Rml::Span<const Rml::Vertex>{vertices.data(), vertices.size()},
		Rml::Span<const int>{indices.data(), indices.size()}));
	return emptyRenderObject;
}

}

void Material::calculateHlmsHash()
{
	static RenderObject emptyRenderObject = make_empty_render_object();

	if(this->material)
	{
		this->material->load();
		this->datablock = this->material->getTechnique(0)->getPass(0)->_getDatablock();
	}
	emptyRenderObject.mMaterial = this->material;
	emptyRenderObject.mLodMaterial = this->material->_getLodValues();
	emptyRenderObject.mHlmsDatablock = this->datablock;
	this->datablock->getCreator()->calculateHashFor(&emptyRenderObject, this->hash, this->casterHash);
	emptyRenderObject.mMaterial = nullptr;
	emptyRenderObject.mLodMaterial = &Ogre::MovableObject::c_DefaultLodMesh;
	emptyRenderObject.mHlmsDatablock = nullptr;
}
