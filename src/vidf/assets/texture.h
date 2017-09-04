#include "asset.h"

namespace vidf
{



class AssetManager;



enum TextureSizeMode
{
	TextureSizeMode_Resize,
	TextureSizeMode_OriginalSize,
};



enum ColorSpace
{
	ColorSpace_Linear,
	ColorSpace_SRGB,
};



enum TextureFormat
{
	TextureFormat_R8_UINT,
	TextureFormat_R8_SINT,
	TextureFormat_R8_UNORM,
	TextureFormat_R8_SNORM,
	TextureFormat_R8_SRGB,

	TextureFormat_R8G8_UINT,
	TextureFormat_R8G8_SINT,
	TextureFormat_R8G8_UNORM,
	TextureFormat_R8G8_SNORM,
	TextureFormat_R8G8_SRGB,

	TextureFormat_R8G8B8A8_UINT,
	TextureFormat_R8G8B8A8_SINT,
	TextureFormat_R8G8B8A8_UNORM,
	TextureFormat_R8G8B8A8_SNORM,
	TextureFormat_R8G8B8A8_SRGB,

	TextureFormat_R16_UINT,
	TextureFormat_R16_SINT,
	TextureFormat_R16_UNORM,
	TextureFormat_R16_SNORM,
	TextureFormat_R16_SRGB,
	TextureFormat_R16_FLOAT,

	TextureFormat_R16G16_UINT,
	TextureFormat_R16G16_SINT,
	TextureFormat_R16G16_UNORM,
	TextureFormat_R16G16_SNORM,
	TextureFormat_R16G16_SRGB,
	TextureFormat_R16G16_FLOAT,

	TextureFormat_R16G16B16A16_UINT,
	TextureFormat_R16G16B16A16_SINT,
	TextureFormat_R16G16B16A16_UNORM,
	TextureFormat_R16G16B16A16_SNORM,
	TextureFormat_R16G16B16A16_SRGB,
	TextureFormat_R16G16B16A16_FLOAT,

	TextureFormat_R32_UINT,
	TextureFormat_R32_SINT,
	TextureFormat_R32_UNORM,
	TextureFormat_R32_SNORM,
	TextureFormat_R32_SRGB,
	TextureFormat_R32_FLOAT,

	TextureFormat_R32G32_UINT,
	TextureFormat_R32G32_SINT,
	TextureFormat_R32G32_UNORM,
	TextureFormat_R32G32_SNORM,
	TextureFormat_R32G32_SRGB,
	TextureFormat_R32G32_FLOAT,

	TextureFormat_R32G32B32A32_UINT,
	TextureFormat_R32G32B32A32_SINT,
	TextureFormat_R32G32B32A32_UNORM,
	TextureFormat_R32G32B32A32_SNORM,
	TextureFormat_R32G32B32A32_SRGB,
	TextureFormat_R32G32B32A32_FLOAT,

	TextureFormat_BC6H,
	TextureFormat_BC7,
	TextureFormat_BC7_SRGB,
};



struct TextureSettings
{
	TextureSizeMode resizeMode = TextureSizeMode_OriginalSize;
	ColorSpace      sourceColorSpace = ColorSpace_SRGB;
	TextureFormat   textureFormat = TextureFormat_R8G8B8A8_SRGB;
	uint            resizeMaxSize = 1024;

	void serialize(yasli::Archive& ar);
};



class TextureSettingsAsset : public Asset
{
public:
	TextureSettingsAsset(AssetRef& _assetRef);

	void serialize(yasli::Archive& ar) override;

private:
	TextureSettings settings;
};



class Texture : public Asset
{
public:
	Texture(AssetRef& _assetRef);

	void serialize(yasli::Archive& ar) override;

private:
	std::string          sourceTexturePath;
	// TextureSettingsAsset textureSettings;
};



void RegisterTextureTypes(AssetManager* manager);



}
