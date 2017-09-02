#include "pch.h"
#include "asset.h"

namespace vidf
{



std::string AssetTraits::GetFullTypeName() const
{
	AssetTraits* parent = GetParent();
	if (parent)
		return parent->GetFullTypeName() + "." + GetTypeName();
	return GetTypeName();
}



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
