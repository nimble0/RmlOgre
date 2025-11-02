# A RmlUi render interface for OGRE-Next

## Dependencies

- https://github.com/mikke89/RmlUi
- https://github.com/OGRECave/ogre-next

## Supported features

- ✔️ Geometry
- ✔️ Textures
- ✔️ Scissor region
- ✔️ Clip mask
- ✔️ Transforms
- ✔️ Layers
- ✔️ Render textures
- ✔️ Mask images
- ✔️ Filters
- ✔️ Shaders

## Supported render systems

- OpenGL 3
- Vulkan
- DirectX 11

## Usage

See the example folder, these are the main steps:

- Add to your OGRE `resources.cfg`:
	- `media/scripts/materials/Rml`
	- `media/scripts/materials/Rml/GLSL`
	- `media/scripts/materials/Rml/HLSL`
	- `media/scripts/compositors`
- Add `FileSystem=.` as a resource path in order for relative path documents to load images correctly.
- Add `FileSystem=/` as a resource path in order for absolute path documents to load images correctly.
- Add a compositor pass provider that provides (see `MyCompositorPassProvider` in `example/src/main.cpp`):
	- `nimble::RmlOgre::CompositorPassGeometry`, id `rml/geometry`
	- `nimble::RmlOgre::CompositorPassRenderQuad`, id `rml/render_quad`

- Render your scene to a texture.
- Create an instance of `nimble::RmlOgre::RenderInterface` with your window texture (`output` parameter) and scene texture (`background` parameter).
- Pass your render interface to a `Rml::CreateContext` call as normal.
- Call `nimble::RmlOgre::RenderInterface::BeginFrame` before `Rml::Context::Render`.
- Call `nimble::RmlOgre::RenderInterface::EndFrame` after `Rml::Context::Render`.
