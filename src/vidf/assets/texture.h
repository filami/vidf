#include "asset.h"

namespace vidf
{



class AssetManager;



class Texture : public Asset
{
public:
	Texture(AssetRef& _assetRef);

	void serialize(yasli::Archive& ar) override;

private:
	int test = 0;
};



void RegisterTextureTypes(AssetManager* manager);



}
