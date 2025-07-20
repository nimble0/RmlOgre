#ifndef NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
#define NIMBLE_RMLOGRE_RENDERINTERFACE_HPP

#include "RenderObject.hpp"
#include "Workspace.hpp"

#include <OgreHlmsDatablock.h>
#include <OgreHlmsSamplerblock.h>

#include <RmlUi/Core/RenderInterface.h>


namespace Ogre {

class HlmsUnlit;
class HlmsUnlitDatablock;
class SceneManager;
class TextureGpu;

}

namespace nimble::RmlOgre {

class RenderInterface : public Rml::RenderInterface
{
	Ogre::HlmsUnlit* hlms = nullptr;
	Ogre::HlmsMacroblock macroblock;
	Ogre::HlmsBlendblock blendblock;
	Ogre::HlmsSamplerblock samplerblock;
	Ogre::HlmsUnlitDatablock* noTextureDatablock;

	RenderPassSettings renderPassSettings;
	int connectionId = 0;
	std::vector<int> layerBuffers;
	Passes passes;

	int datablockId = 0;
	std::vector<Rml::CompiledGeometryHandle> releaseGeometries;
	std::vector<Rml::TextureHandle> releaseTextures;

	Workspace workspace;

	void releaseBufferedGeometries();
	void releaseBufferedTextures();

	template <class TRenderPass>
	TRenderPass& getRenderPass()
	{
		TRenderPass* lastPass = nullptr;
		if(!this->passes.empty())
			lastPass = std::get_if<TRenderPass>(&this->passes.back());
		if(!lastPass || lastPass->settings != this->renderPassSettings)
		{
			TRenderPass newPass;
			newPass.settings = this->renderPassSettings;
			this->passes.push_back(std::move(newPass));
			lastPass = &std::get<TRenderPass>(this->passes.back());
		}

		return *lastPass;
	}

public:
	RenderInterface(
		const Ogre::String& name,
		Ogre::SceneManager* sceneManager,
		Ogre::TextureGpu* output,
		Ogre::TextureGpu* background);


	int addConnection() { return this->connectionId++; }


	void BeginFrame();
	void EndFrame();

	Rml::CompiledGeometryHandle CompileGeometry(
		Rml::Span<const Rml::Vertex> vertices,
		Rml::Span<const int> indices
	) override;
	void RenderGeometry(
		Rml::CompiledGeometryHandle geometry,
		Rml::Vector2f translation,
		Rml::TextureHandle texture
	) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(
		Rml::Vector2i& texture_dimensions,
		const Rml::String& source
	) override;
	Rml::TextureHandle GenerateTexture(
		Rml::Span<const Rml::byte> source,
		Rml::Vector2i source_dimensions
	) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(
		Rml::ClipMaskOperation operation,
		Rml::CompiledGeometryHandle geometry,
		Rml::Vector2f translation
	) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(
		Rml::LayerHandle source,
		Rml::LayerHandle destination,
		Rml::BlendMode blend_mode,
		Rml::Span<const Rml::CompiledFilterHandle> filters
	) override;
	void PopLayer() override;
};

}

#endif // NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
