#ifndef NIMBLE_RMLOGRE_GEOMETRY_HPP
#define NIMBLE_RMLOGRE_GEOMETRY_HPP

#include <RmlUi/Core/Vertex.h>


namespace Ogre {

struct VertexArrayObject;

}

namespace nimble::RmlOgre {

Ogre::VertexArrayObject* create_vao(
	Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices);

}

#endif // NIMBLE_RMLOGRE_GEOMETRY_HPP
