#pragma once

namespace vidf
{



typedef uint64 AssetId;
class Asset;
class AssetTraits;
typedef std::shared_ptr<Asset> AssetPtr;
typedef std::shared_ptr<AssetTraits> AssetTraitsPtr;



struct AssetRef
{
	AssetId      id = -1;
	std::string  name;
	AssetPtr     asset;
	const AssetTraits* traits;
};



class AssetTraits
{
public:
	virtual ~AssetTraits() {}
	virtual AssetPtr     Create(AssetRef& assetRef) const { return AssetPtr(); }
	virtual const char*  GetTypeName() const = 0;
	virtual AssetTraits* GetParent() const { return nullptr; }
	virtual bool         IsAbstract() const { return false; }
	std::string          GetFullTypeName() const;
};



class Asset
{
public:
	Asset(AssetRef& _assetRef);
	virtual ~Asset();

	virtual void serialize(yasli::Archive& ar);
	virtual void SerializeQuickEdit(yasli::Archive& ar);

	const AssetRef& assetRef;
};



}
