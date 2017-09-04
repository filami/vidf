#include "pch.h"
#include "asset.h"
#include "yasli/Archive.h"
#include "yasli/STL.h"

namespace vidf
{



std::string AssetTraits::GetFullTypeName() const
{
	AssetTraits* parent = GetParent();
	if (parent)
		return parent->GetFullTypeName() + "." + GetTypeName();
	return GetTypeName();
}



Asset::Asset()
{
}



Asset::~Asset() {}



void Asset::serialize(yasli::Archive& ar)
{
	ar(id, "id");
	ar(name, "name");
}



void Asset::SerializeQuickEdit(yasli::Archive& ar)
{
	serialize(ar);
}



}
