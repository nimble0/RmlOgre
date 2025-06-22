#include "RenderInterface.hpp"


using namespace nimble::RmlOgre;

Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(
	Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices)
{
	return Rml::CompiledGeometryHandle{};
}
void RenderInterface::RenderGeometry(
	Rml::CompiledGeometryHandle geometry,
	Rml::Vector2f translation,
	Rml::TextureHandle texture)
{}
void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{}

Rml::TextureHandle RenderInterface::LoadTexture(
	Rml::Vector2i& texture_dimensions,
	const Rml::String& source)
{
	return Rml::TextureHandle{};
}
Rml::TextureHandle RenderInterface::GenerateTexture(
	Rml::Span<const Rml::byte> source,
	Rml::Vector2i source_dimensions)
{
	return Rml::TextureHandle{};
}
void RenderInterface::ReleaseTexture(Rml::TextureHandle texture) {}

void RenderInterface::EnableScissorRegion(bool enable) {}
void RenderInterface::SetScissorRegion(Rml::Rectanglei region) {}
