#include "pch.h"
#include "texture.h"
#include "assetmanager.h"

namespace vidf
{



Texture::Texture(AssetRef& _assetRef)
	: Asset(_assetRef)
{
}



void Texture::serialize(yasli::Archive& ar)
{
	ar(test, "test", "Test");
}




void RegisterTextureTypes(AssetManager* manager)
{
	class TextureType : public AssetTraits
	{
		AssetPtr    Create(AssetRef& assetRef) const override { return std::make_shared<Texture>(assetRef); }
		const char* GetTypeName() const override { return "Texture"; }
	};
	manager->RegisterType(std::make_shared<TextureType>());
}



}
