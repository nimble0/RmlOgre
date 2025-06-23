#ifndef NIMBLE_RMLOGRE_GEOMETRY_HPP
#define NIMBLE_RMLOGRE_GEOMETRY_HPP

#include <RmlUi/Core/Vertex.h>


namespace Ogre {

class VertexArrayObject;

}

namespace nimble::RmlOgre {

struct Geometry
{
	Ogre::VertexArrayObject* vao = nullptr;

	Geometry() = default;
	Geometry(const Geometry&) = delete;
	Geometry& operator=(const Geometry&) = delete;
	Geometry(Geometry&& b) :
		vao{b.vao}
	{
		b.vao = nullptr;
	}
	Geometry& operator=(Geometry&& b)
	{
		this->vao = b.vao;
		b.vao = nullptr;
		return *this;
	}
	Geometry(
		Rml::Span<const Rml::Vertex> vertices,
		Rml::Span<const int> indices
	);
	~Geometry();
};

}

#endif // NIMBLE_RMLOGRE_GEOMETRY_HPP
