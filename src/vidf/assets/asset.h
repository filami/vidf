#pragma once

namespace vidf
{



typedef uint64 AssetId;
class Asset;
struct AssetTraits;
typedef std::shared_ptr<Asset> AssetPtr;



struct AssetRef
{
	AssetId      id = -1;
	std::string  name;
	AssetPtr     asset;
	AssetTraits* traits;
};



struct AssetTraits
{
	virtual ~AssetTraits() {}
	virtual Asset*       Create(AssetRef& assetRef) = 0;
	virtual const char*  GetTypeName() const = 0;
	virtual AssetTraits* GetParent() const { return nullptr; }
	virtual bool         IsAbstract() const { return false; }
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
