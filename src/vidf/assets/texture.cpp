#include "pch.h"
#include "texture.h"
#include "assetmanager.h"
#include "yasli/Enum.h"
#include "yasli/STL.h"

namespace vidf
{



YASLI_ENUM_BEGIN(TextureSizeMode, "TextureSizeMode")
	YASLI_ENUM(TextureSizeMode_Resize, "TextureSizeMode_Resize", "Resize")
	YASLI_ENUM(TextureSizeMode_OriginalSize, "TextureSizeMode_OriginalSize", "OriginalSize")
YASLI_ENUM_END()



YASLI_ENUM_BEGIN(ColorSpace, "ColorSpace")
	YASLI_ENUM(ColorSpace_Linear, "TextureSizeMode_Resize", "Linear")
	YASLI_ENUM(ColorSpace_SRGB, "ColorSpace_SRGB", "SRGB")
YASLI_ENUM_END()



YASLI_ENUM_BEGIN(TextureFormat, "TextureFormat")
	YASLI_ENUM(TextureFormat_R8_UINT,            "TextureFormat_R8_UINT", "R8_UINT")
	YASLI_ENUM(TextureFormat_R8_SINT,            "TextureFormat_R8_SINT", "R8_SINT")
	YASLI_ENUM(TextureFormat_R8_UNORM,           "TextureFormat_R8_UNORM", "R8_UNORM")
	YASLI_ENUM(TextureFormat_R8_SNORM,           "TextureFormat_R8_SNORM", "R8_SNORM")
	YASLI_ENUM(TextureFormat_R8_SRGB,            "TextureFormat_R8_SRGB", "R8_SRGB")
	YASLI_ENUM(TextureFormat_R8G8_UINT,          "TextureFormat_R8G8_UINT", "R8G8_UINT")
	YASLI_ENUM(TextureFormat_R8G8_SINT,          "TextureFormat_R8G8_SINT", "R8G8_SINT")
	YASLI_ENUM(TextureFormat_R8G8_UNORM,         "TextureFormat_R8G8_UNORM", "R8G8_UNORM")
	YASLI_ENUM(TextureFormat_R8G8_SNORM,         "TextureFormat_R8G8_SNORM", "R8G8_SNORM")
	YASLI_ENUM(TextureFormat_R8G8_SRGB,          "TextureFormat_R8G8_SRGB", "R8G8_SRGB")
	YASLI_ENUM(TextureFormat_R8G8B8A8_UINT,      "TextureFormat_R8G8B8A8_UINT", "R8G8B8A8_UINT")
	YASLI_ENUM(TextureFormat_R8G8B8A8_SINT,      "TextureFormat_R8G8B8A8_SINT", "R8G8B8A8_SINT")
	YASLI_ENUM(TextureFormat_R8G8B8A8_UNORM,     "TextureFormat_R8G8B8A8_UNORM", "R8G8B8A8_UNORM")
	YASLI_ENUM(TextureFormat_R8G8B8A8_SNORM,     "TextureFormat_R8G8B8A8_SNORM", "R8G8B8A8_SNORM")
	YASLI_ENUM(TextureFormat_R8G8B8A8_SRGB,      "TextureFormat_R8G8B8A8_SRGB", "R8G8B8A8_SRGB")
	YASLI_ENUM(TextureFormat_R16_UINT,           "TextureFormat_R16_UINT", "R16_UINT")
	YASLI_ENUM(TextureFormat_R16_SINT,           "TextureFormat_R16_SINT", "R16_SINT")
	YASLI_ENUM(TextureFormat_R16_UNORM,          "TextureFormat_R16_UNORM", "R16_UNORM")
	YASLI_ENUM(TextureFormat_R16_SNORM,          "TextureFormat_R16_SNORM", "R16_SNORM")
	YASLI_ENUM(TextureFormat_R16_SRGB,           "TextureFormat_R16_SRGB", "R16_SRGB")
	YASLI_ENUM(TextureFormat_R16_FLOAT,          "TextureFormat_R16_FLOAT", "R16_FLOAT")
	YASLI_ENUM(TextureFormat_R16G16_UINT,        "TextureFormat_R16G16_UINT", "R16G16_UINT")
	YASLI_ENUM(TextureFormat_R16G16_SINT,        "TextureFormat_R16G16_SINT", "R16G16_SINT")
	YASLI_ENUM(TextureFormat_R16G16_UNORM,       "TextureFormat_R16G16_UNORM", "R16G16_UNORM")
	YASLI_ENUM(TextureFormat_R16G16_SNORM,       "TextureFormat_R16G16_SNORM", "R16G16_SNORM")
	YASLI_ENUM(TextureFormat_R16G16_SRGB,        "TextureFormat_R16G16_SRGB", "R16G16_SRGB")
	YASLI_ENUM(TextureFormat_R16G16_FLOAT,       "TextureFormat_R16G16_FLOAT", "R16G16_FLOAT")
	YASLI_ENUM(TextureFormat_R16G16B16A16_UINT,  "TextureFormat_R16G16B16A16_UINT", "R16G16B16A16_UINT")
	YASLI_ENUM(TextureFormat_R16G16B16A16_SINT,  "TextureFormat_R16G16B16A16_SINT", "R16G16B16A16_SINT")
	YASLI_ENUM(TextureFormat_R16G16B16A16_UNORM, "TextureFormat_R16G16B16A16_UNORM", "R16G16B16A16_UNORM")
	YASLI_ENUM(TextureFormat_R16G16B16A16_SNORM, "TextureFormat_R16G16B16A16_SNORM", "R16G16B16A16_SNORM")
	YASLI_ENUM(TextureFormat_R16G16B16A16_SRGB,  "TextureFormat_R16G16B16A16_SRGB", "R16G16B16A16_SRGB")
	YASLI_ENUM(TextureFormat_R16G16B16A16_FLOAT, "TextureFormat_R16G16B16A16_FLOAT", "R16G16B16A16_FLOAT")
	YASLI_ENUM(TextureFormat_R32_UINT,           "TextureFormat_R32_UINT", "R32_UINT")
	YASLI_ENUM(TextureFormat_R32_SINT,           "TextureFormat_R32_SINT", "R32_SINT")
	YASLI_ENUM(TextureFormat_R32_UNORM,          "TextureFormat_R32_UNORM", "R32_UNORM")
	YASLI_ENUM(TextureFormat_R32_SNORM,          "TextureFormat_R32_SNORM", "R32_SNORM")
	YASLI_ENUM(TextureFormat_R32_SRGB,           "TextureFormat_R32_SRGB", "R32_SRGB")
	YASLI_ENUM(TextureFormat_R32_FLOAT,          "TextureFormat_R32_FLOAT", "R32_FLOAT")
	YASLI_ENUM(TextureFormat_R32G32_UINT,        "TextureFormat_R32G32_UINT", "R32G32_UINT")
	YASLI_ENUM(TextureFormat_R32G32_SINT,        "TextureFormat_R32G32_SINT", "R32G32_SINT")
	YASLI_ENUM(TextureFormat_R32G32_UNORM,       "TextureFormat_R32G32_UNORM", "R32G32_UNORM")
	YASLI_ENUM(TextureFormat_R32G32_SNORM,       "TextureFormat_R32G32_SNORM", "R32G32_SNORM")
	YASLI_ENUM(TextureFormat_R32G32_SRGB,        "TextureFormat_R32G32_SRGB", "R32G32_SRGB")
	YASLI_ENUM(TextureFormat_R32G32_FLOAT,       "TextureFormat_R32G32_FLOAT", "R32G32_FLOAT")
	YASLI_ENUM(TextureFormat_R32G32B32A32_UINT,  "TextureFormat_R32G32B32A32_UINT", "R32G32B32A32_UINT")
	YASLI_ENUM(TextureFormat_R32G32B32A32_SINT,  "TextureFormat_R32G32B32A32_SINT", "R32G32B32A32_SINT")
	YASLI_ENUM(TextureFormat_R32G32B32A32_UNORM, "TextureFormat_R32G32B32A32_UNORM", "R32G32B32A32_UNORM")
	YASLI_ENUM(TextureFormat_R32G32B32A32_SNORM, "TextureFormat_R32G32B32A32_SNORM", "R32G32B32A32_SNORM")
	YASLI_ENUM(TextureFormat_R32G32B32A32_SRGB,  "TextureFormat_R32G32B32A32_SRGB", "R32G32B32A32_SRGB")
	YASLI_ENUM(TextureFormat_R32G32B32A32_FLOAT, "TextureFormat_R32G32B32A32_FLOAT", "R32G32B32A32_FLOAT")
	YASLI_ENUM(TextureFormat_BC6H,               "TextureFormat_BC6H", "BC6H")
	YASLI_ENUM(TextureFormat_BC7,                "TextureFormat_BC7", "BC7")
	YASLI_ENUM(TextureFormat_BC7_SRGB,           "TextureFormat_BC7_SRGB", "BC7_SRGB")
YASLI_ENUM_END()



void TextureSettings::serialize(yasli::Archive& ar)
{
	ar(resizeMode, "resizeMode", "Resize");
	if (resizeMode != TextureSizeMode_OriginalSize)
		ar(resizeMaxSize, "resizeMaxSize", "Maximum Size");
	ar(sourceColorSpace, "sourceColorSpace", "Source Color Space");
	ar(textureFormat, "textureFormat", "Output Format");
}



TextureSettingsAsset::TextureSettingsAsset(AssetRef& _assetRef)
	: Asset(_assetRef)
{
}



void TextureSettingsAsset::serialize(yasli::Archive& ar)
{
	settings.serialize(ar);
}



Texture::Texture(AssetRef& _assetRef)
	: Asset(_assetRef)
{
}



void Texture::serialize(yasli::Archive& ar)
{
	ar(sourceTexturePath, "sourceTexturePath", "Source");
}




void RegisterTextureTypes(AssetManager* manager)
{
	class TextureSettingsAssetType : public AssetTraits
	{
		AssetPtr    Create(AssetRef& assetRef) const override { return std::make_shared<TextureSettingsAsset>(assetRef); }
		const char* GetTypeName() const override { return "TextureSettings"; }
	};
	manager->RegisterType(std::make_shared<TextureSettingsAssetType>());

	class TextureType : public AssetTraits
	{
		AssetPtr    Create(AssetRef& assetRef) const override { return std::make_shared<Texture>(assetRef); }
		const char* GetTypeName() const override { return "Texture"; }
	};
	manager->RegisterType(std::make_shared<TextureType>());
}



}
