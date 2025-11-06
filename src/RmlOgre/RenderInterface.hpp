#ifndef NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
#define NIMBLE_RMLOGRE_RENDERINTERFACE_HPP

#include "FilterMaker.hpp"
#include "Material.hpp"
#include "ObjectIndex.hpp"
#include "ShaderMaker.hpp"
#include "Workspace.hpp"
#include "filters.hpp"

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

struct Layer
{
	int connectionId = -1;
	int copyPass = -1;

	Layer take()
	{
		Layer self = *this;
		this->connectionId = -1;
		return self;
	}
	bool isTaken() const
	{
		return this->connectionId == -1;
	}
};

class RenderInterface : public Rml::RenderInterface
{
	Ogre::HlmsUnlit* hlms = nullptr;
	Ogre::HlmsMacroblock macroblock;
	Ogre::HlmsBlendblock blendblock;
	Ogre::HlmsSamplerblock samplerblock;
	ObjectIndex<Material> materials;

	std::unordered_map<Rml::String, std::unique_ptr<FilterMaker>> filterMakers;
	MaskImageFilterMaker maskImageFilterMaker;
	ObjectIndex<std::unique_ptr<Filter>> filters;

	std::unordered_map<Rml::String, std::unique_ptr<ShaderMaker>> shaderMakers;
	ObjectIndex<Material> shaders;

	RenderPassSettings renderPassSettings;
	int connectionId = 0;
	std::vector<Layer> layerBuffers;
	int numActiveLayers = 0;
	Passes passes;

	int datablockId = 0;
	std::vector<Rml::CompiledGeometryHandle> releaseGeometries;
	std::vector<Rml::TextureHandle> releaseTextures;
	std::vector<Ogre::TextureGpu*> releaseRenderTextures;

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
	Layer getLayerBuffer(int index);
	void putLayerBuffer(int index, Layer id);
	Layer acquireLayerBuffer();
	void releaseLayerBuffer(Layer id);

	const RenderPassSettings& currentRenderPassSettings() const { return this->renderPassSettings; }
	void addPass(Pass&& pass);
	void releaseRenderTexture(Ogre::TextureGpu* texture);


	void AddFilterMaker(Rml::String name, std::unique_ptr<FilterMaker> filterMaker);
	void AddShaderMaker(Rml::String name, std::unique_ptr<ShaderMaker> shaderMaker);

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

	Rml::CompiledFilterHandle CompileFilter(
		const Rml::String& name,
		const Rml::Dictionary& parameters
	) override;
	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::TextureHandle SaveLayerAsTexture() override;
	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledShaderHandle CompileShader(
		const Rml::String& name,
		const Rml::Dictionary& parameters
	) override;
	void RenderShader(
		Rml::CompiledShaderHandle shader,
		Rml::CompiledGeometryHandle geometry,
		Rml::Vector2f translation,
		Rml::TextureHandle texture
	) override;
	void ReleaseShader(Rml::CompiledShaderHandle shader) override;
};

}

#endif // NIMBLE_RMLOGRE_RENDERINTERFACE_HPP
