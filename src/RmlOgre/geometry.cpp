#include "geometry.hpp"

#include <OgreRoot.h>
#include <OgrePixelFormatGpuUtils.h>
#include <Vao/OgreVaoManager.h>
#include <Vao/OgreVertexArrayObject.h>


using namespace nimble::RmlOgre;

struct GuiVertex
{
	Ogre::Vector2 position;
	Ogre::ColourValue colour;
	Ogre::uint32 colour2;
	Ogre::Vector2 uv;

	GuiVertex(const Rml::Vertex& v) :
		position{v.position.x, v.position.y},
		colour{
			v.colour.red / 255.0f,
			v.colour.green / 255.0f,
			v.colour.blue / 255.0f,
			v.colour.alpha / 255.0f},
		uv{v.tex_coord.x, v.tex_coord.y}
	{}

	template <class Iterator>
	void write(Iterator& iterator) const
	{
		*(iterator++) = this->position.x;
		*(iterator++) = this->position.y;

		*(iterator++) = this->colour.r;
		*(iterator++) = this->colour.g;
		*(iterator++) = this->colour.b;
		*(iterator++) = this->colour.a;

		*(iterator++) = this->uv.x;
		*(iterator++) = this->uv.y;
	}

	static Ogre::VertexElement2Vec format()
	{
		Ogre::VertexElement2Vec format;
		format.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_POSITION));
		format.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_DIFFUSE));
		format.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));
		return format;
	}

	static const Ogre::VertexElement2Vec FORMAT;
};

const Ogre::VertexElement2Vec GuiVertex::FORMAT = GuiVertex::format();

Ogre::VertexArrayObject* nimble::RmlOgre::create_vao(
	Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices)
{
	Ogre::Root& root = Ogre::Root::getSingleton();
	Ogre::RenderSystem* renderSystem = root.getRenderSystem();
	Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

	auto vertexSize = vaoManager->calculateVertexSize(GuiVertex::FORMAT);
	auto* ogreVertices = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
		vertices.size() * vertexSize,
		Ogre::MEMCATEGORY_GEOMETRY));
	auto verticesIter = ogreVertices;
	for(auto& v : vertices)
		GuiVertex{v}.write(verticesIter);
	Ogre::VertexBufferPacked* vertexBuffer = vaoManager->createVertexBuffer(
		GuiVertex::FORMAT,
		vertices.size(),
		Ogre::BT_DEFAULT,
		ogreVertices,
		true);

	Ogre::VertexBufferPackedVec vertexBuffers;
	vertexBuffers.push_back(vertexBuffer);

	auto* ogreIndices = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(
		indices.size() * sizeof(Ogre::uint16),
		Ogre::MEMCATEGORY_GEOMETRY));
	for(std::size_t i = 0; i < indices.size(); ++i)
		ogreIndices[i] = indices[i];
	Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(
		Ogre::IndexBufferPacked::IT_16BIT,
		indices.size(),
		Ogre::BT_IMMUTABLE,
		ogreIndices,
		true);

	return vaoManager->createVertexArrayObject(
		vertexBuffers,
		indexBuffer,
		Ogre::OT_TRIANGLE_LIST);
}
