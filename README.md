# A RmlUi render interface for OGRE-Next

## Dependencies

- https://github.com/mikke89/RmlUi
- https://github.com/OGRECave/ogre-next

## Supported features

- ✔️ Geometry
- ✔️ Textures
- ✔️ Scissor region
- ❌ Clip mask
- ❌ Transforms
- ❌ Layers
- ❌ Render textures
- ❌ Mask images
- ❌ Filters
- ❌ Shaders

## Usage

See the example folder, these are the main steps:

- Add `media/scripts/compositors/Ui.compositor` to your OGRE `resources.cfg`.
- Add `media/materials/AlphaBlend` to your OGRE `resources.cfg`.
- Add `FileSystem=.` as a resource path in order for relative path documents to load images correctly.
- Add `FileSystem=/` as a resource path in order for absolute path documents to load images correctly.
- Register a compositor pass provider for `nimble::RmlOgre::CompositorPassGeometry` with custom id `rml_geometry`, see `MyCompositorPassProvider` in `example/src/main.cpp`.
- Connect the render interface output by connecting `nimble::RmlOgre::RenderInterface::GetOutput` to an extra external node in your main workspace, see `media/scripts/compositors/Example.compositor`.
- Call `nimble::RmlOgre::RenderInterface::EndFrame` after `Rml::Context::Render`.
