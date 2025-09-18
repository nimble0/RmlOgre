#include "filters.hpp"

#include "Pass.hpp"
#include "RenderInterface.hpp"

#include <OgreGpuProgramParams.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/DecorationTypes.h>


using namespace nimble::RmlOgre;

namespace {

Ogre::Vector4 to_ogre(Rml::ColourbPremultiplied c)
{
	Ogre::Vector4 result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c[i]);
	return result;
}

}

void BlurFilter::apply(RenderInterface& renderInterface)
{
	RenderPassSettings passSettings = renderInterface.currentRenderPassSettings();
	if(this->halfSamples > 0)
	{
		passSettings.scissorRegion.p0 /= 2;
		renderInterface.addPass(RenderQuadPass(this->halfsample, passSettings));
		for(int i = 0; i < this->halfSamples - 1; ++i)
		{
			passSettings.scissorRegion.p0 /= 2;
			passSettings.scissorRegion.p1 = (passSettings.scissorRegion.p1 + Rml::Vector2i{1, 1}) / 2;
			renderInterface.addPass(RenderQuadPass(this->halfsample, passSettings));
		}
	}
	passSettings.scissorRegion.p0.x = renderInterface.currentRenderPassSettings().scissorRegion.p0.x;
	passSettings.scissorRegion.p1 = renderInterface.currentRenderPassSettings().scissorRegion.p1;
	renderInterface.addPass(RenderQuadPass(this->blurH, passSettings));
	renderInterface.addPass(RenderQuadPass(this->blurV, renderInterface.currentRenderPassSettings()));
}

BlurFilterMaker::BlurFilterMaker()
{
	this->halfsampleMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Halfsample");
	this->blurMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Blur");
}

BlurFilter BlurFilterMaker::make(float sigma)
{
	int halfsamples = 0;
	int step = 1;
	while(sigma > MAX_SCALED_SIGMA * step)
	{
		halfsamples += 1;
		step *= 2;
	}
	float scale = 1.0f / step;

	std::array<float, NUM_WEIGHTS> weights;
	if(sigma < 0.1f)
		weights[0] = 1.0f;
	else
	{
		float weightsTotal = 0.0f;
		for(int i = 0; i < NUM_WEIGHTS; ++i)
		{
			int j = i * step;
			weights[i] = Rml::Math::Exp(-float(j * j) / (2.0f * sigma * sigma))
				/ (Rml::Math::SquareRoot(2.0f * Rml::Math::RMLUI_PI) * sigma);
			weightsTotal += (i == 0 ? 1.0f : 2.0f) * weights[i];
		}

		for(float& weight : weights)
			weight /= weightsTotal;
	}

	Ogre::MaterialPtr blurH(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
	*blurH = *this->blurMaterial;
	blurH->load();
	{
		auto* pass = blurH
			->getBestTechnique()
			->getPass(0);
		auto vertexProgramParameters = pass->getVertexProgramParameters();
		vertexProgramParameters->setNamedConstant("scale", Ogre::Vector2{scale, 1.0f});
		auto fragmentProgramParameters = pass->getFragmentProgramParameters();
		fragmentProgramParameters->setNamedConstant("direction", Ogre::Vector2{1.0f, 0.0f});
		fragmentProgramParameters->setNamedConstant("weights", weights.data(), weights.size() / 4);
	}

	Ogre::MaterialPtr blurV(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
	*blurV = *this->blurMaterial;
	blurV->load();
	{
		auto* pass = blurV
			->getBestTechnique()
			->getPass(0);
		auto vertexProgramParameters = pass->getVertexProgramParameters();
		vertexProgramParameters->setNamedConstant("scale", Ogre::Vector2{1.0f, scale});
		auto fragmentProgramParameters = pass->getFragmentProgramParameters();
		fragmentProgramParameters->setNamedConstant("direction", Ogre::Vector2{0.0f, 1.0f});
		fragmentProgramParameters->setNamedConstant("weights", weights.data(), weights.size() / 4);
	}

	return BlurFilter(this->halfsampleMaterial, blurH, blurV, halfsamples);
}

std::unique_ptr<Filter> BlurFilterMaker::make(const Rml::Dictionary& parameters)
{
	return std::make_unique<BlurFilter>(this->make(Rml::Get(parameters, "sigma", 0.0f)));
}


void DropShadowFilter::apply(RenderInterface& renderInterface)
{
	RenderPassSettings passSettings = renderInterface.currentRenderPassSettings();

	int tempLayer = renderInterface.addConnection();
	renderInterface.addPass(CopyPass(renderInterface.acquireLayerBuffer(), tempLayer));

	renderInterface.addPass(RenderQuadPass(this->shadowMaterial, passSettings));
	this->blurFilter.apply(renderInterface);

	int shadowLayer = renderInterface.addConnection();
	renderInterface.addPass(SwapPass(tempLayer, shadowLayer));
	int discardCopy = renderInterface.addConnection();
	renderInterface.addPass(CompositePass(shadowLayer, discardCopy, false, passSettings));
	renderInterface.releaseLayerBuffer(discardCopy);
}

DropShadowFilterMaker::DropShadowFilterMaker()
{
	this->shadowMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Shadow");
	this->blurlessShadowMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/BlurlessShadow");
}
std::unique_ptr<Filter> DropShadowFilterMaker::make(const Rml::Dictionary& parameters)
{
	float sigma = Rml::Get(parameters, "sigma", 0.0f);
	auto colour = Rml::Get(parameters, "color", Rml::Colourb()).ToPremultiplied();
	auto offset = Rml::Get(parameters, "offset", Rml::Vector2f(0.f));

	if(sigma > 0.5f)
	{
		Ogre::MaterialPtr shadowMaterial(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
		*shadowMaterial = *this->shadowMaterial;
		shadowMaterial->load();
		{
			auto* pass = shadowMaterial
				->getBestTechnique()
				->getPass(0);
			auto vertexProgramParameters = pass->getVertexProgramParameters();
			vertexProgramParameters->setNamedConstant("offset", Ogre::Vector2{offset.x, offset.y});
			auto fragmentProgramParameters = pass->getFragmentProgramParameters();
			fragmentProgramParameters->setNamedConstant("colour", to_ogre(colour));
		}

		return std::make_unique<DropShadowFilter>(
			this->blurFilterMaker.make(sigma),
			shadowMaterial
		);
	}
	else
	{
		Ogre::MaterialPtr shadowMaterial(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
		*shadowMaterial = *this->blurlessShadowMaterial;
		shadowMaterial->load();
		{
			auto* pass = shadowMaterial
				->getBestTechnique()
				->getPass(0);
			auto fragmentProgramParameters = pass->getFragmentProgramParameters();
			fragmentProgramParameters->setNamedConstant("offset", Ogre::Vector2{offset.x, offset.y});
			fragmentProgramParameters->setNamedConstant("colour", to_ogre(colour));
		}

		return std::make_unique<SingleMaterialFilter>(shadowMaterial);
	}
}


OpacityFilterMaker::OpacityFilterMaker()
{
	this->baseMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Opacity");
}

std::unique_ptr<Filter> OpacityFilterMaker::make(const Rml::Dictionary& parameters)
{
	Ogre::MaterialPtr material(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
	*material = *this->baseMaterial;
	material->load();
	auto fragmentProgramParameters = material
		->getBestTechnique()
		->getPass(0)
		->getFragmentProgramParameters();
	fragmentProgramParameters->setNamedConstant("value", Rml::Get(parameters, "value", 1.0f));
	return std::make_unique<SingleMaterialFilter>(material);
}


ColourMatrixFilterMaker::ColourMatrixFilterMaker()
{
	this->baseMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/ColourMatrix");
}

std::unique_ptr<Filter> ColourMatrixFilterMaker::make(const Ogre::Matrix4& matrix)
{
	Ogre::MaterialPtr material(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
	*material = *this->baseMaterial;
	material->load();
	material
		->getBestTechnique()
		->getPass(0)
		->getFragmentProgramParameters()
		->setNamedConstant("colourMatrix", matrix);
	return std::make_unique<SingleMaterialFilter>(material);
}


std::unique_ptr<Filter> BrightnessFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	return this->make(Ogre::Matrix4::getScale(value, value, value));
}

std::unique_ptr<Filter> ContrastFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	float grayness = 0.5f - 0.5f * value;
	auto matrix = Ogre::Matrix4::getScale(value, value, value);
	matrix[0][3] = grayness;
	matrix[1][3] = grayness;
	matrix[2][3] = grayness;
	return this->make(matrix);
}

std::unique_ptr<Filter> InvertFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Math::Clamp(Rml::Get(parameters, "value", 1.0f), 0.0f, 1.0f);
	float inverted = 1.0f - 2.0f * value;
	auto matrix = Ogre::Matrix4::getScale(inverted, inverted, inverted);
	matrix[0][3] = value;
	matrix[1][3] = value;
	matrix[2][3] = value;
	return this->make(matrix);
}

std::unique_ptr<Filter> GrayscaleFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	float invValue = 1.0f - value;
	auto gray = value * Ogre::Vector3{0.2126f, 0.7152f, 0.0722f};
	Ogre::Matrix4 matrix{
		gray.x + invValue, gray.y,            gray.z,            0.0f,
		gray.x,            gray.y + invValue, gray.z,            0.0f,
		gray.x,            gray.y,            gray.z + invValue, 0.0f,
		0.0f,              0.0f,              0.0f,              1.0f
	};
	return this->make(matrix);
}

std::unique_ptr<Filter> SepiaFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	float invValue = 1.0f - value;
	auto rMix = value * Ogre::Vector3{0.393f, 0.769f, 0.189f};
	auto gMix = value * Ogre::Vector3{0.349f, 0.686f, 0.168f};
	auto bMix = value * Ogre::Vector3{0.272f, 0.534f, 0.131f};
	Ogre::Matrix4 matrix{
		rMix.x + invValue, rMix.y,            rMix.z,            0.0f,
		gMix.x,            gMix.y + invValue, gMix.z,            0.0f,
		bMix.x,            bMix.y,            bMix.z + invValue, 0.0f,
		0.0f,              0.0f,              0.0f,              1.0f
	};
	return this->make(matrix);
}

std::unique_ptr<Filter> HueRotateFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	float s = Ogre::Math::Sin(value);
	float c = Ogre::Math::Cos(value);
	Ogre::Matrix4 matrix{
		0.213f + 0.787f * c - 0.213f * s,  0.715f - 0.715f * c - 0.715f * s,  0.072f - 0.072f * c + 0.928f * s,  0.0f,
		0.213f - 0.213f * c + 0.143f * s,  0.715f + 0.285f * c + 0.140f * s,  0.072f - 0.072f * c - 0.283f * s,  0.0f,
		0.213f - 0.213f * c - 0.787f * s,  0.715f - 0.715f * c + 0.715f * s,  0.072f + 0.928f * c + 0.072f * s,  0.0f,
		0.0f,                              0.0f,                              0.0f,                              1.0f
	};
	return this->make(matrix);
}

std::unique_ptr<Filter> SaturateFilterMaker::make(const Rml::Dictionary& parameters)
{
	float value = Rml::Get(parameters, "value", 1.0f);
	Ogre::Matrix4 matrix{
		0.213f + 0.787f * value,  0.715f - 0.715f * value,  0.072f - 0.072f * value,  0.0f,
		0.213f - 0.213f * value,  0.715f + 0.285f * value,  0.072f - 0.072f * value,  0.0f,
		0.213f - 0.213f * value,  0.715f - 0.715f * value,  0.072f + 0.928f * value,  0.0f,
		0.0f,                     0.0f,                     0.0f,                     1.0f
	};
	return this->make(matrix);
}


MaskImageFilterMaker::MaskImageFilterMaker()
{
	this->baseMaterial = Ogre::MaterialManager::getSingleton().getByName("Rml/Mask");
}

SingleMaterialFilter MaskImageFilterMaker::make(Ogre::TextureGpu* image)
{
	Ogre::MaterialPtr material(OGRE_NEW Ogre::Material(nullptr, "", 0, "", false, nullptr));
	*material = *this->baseMaterial;
	material->load();
	auto textureUnit = material
		->getBestTechnique()
		->getPass(0)
		->getTextureUnitState("dstTex");
	textureUnit->setTexture(image);
	return SingleMaterialFilter(material);
}

std::unique_ptr<Filter> MaskImageFilterMaker::make(const Rml::Dictionary& parameters)
{
	void* texture = Rml::Get<void*>(parameters, "maskImage", nullptr);
	return std::make_unique<SingleMaterialFilter>(this->make(static_cast<Ogre::TextureGpu*>(texture)));
}
