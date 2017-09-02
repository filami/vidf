#include "pch.h"
#include "asset.h"

namespace vidf
{


Asset::Asset(AssetRef& _assetRef)
	: assetRef(_assetRef)
{
}



Asset::~Asset() {}



void Asset::serialize(yasli::Archive& ar)
{
}



void Asset::SerializeQuickEdit(yasli::Archive& ar)
{
	serialize(ar);
}



}
