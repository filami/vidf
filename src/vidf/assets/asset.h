#pragma once

namespace vidf
{



typedef uint64 AssetId;
class Asset;
class AssetTraits;
typedef std::shared_ptr<Asset> AssetPtr;
typedef std::shared_ptr<AssetTraits> AssetTraitsPtr;



class AssetTraits
{
public:
	virtual ~AssetTraits() {}
	virtual AssetPtr     Create() const { return AssetPtr(); }
	virtual const char*  GetTypeName() const = 0;
	virtual AssetTraits* GetParent() const { return nullptr; }
	virtual bool         IsAbstract() const { return false; }
	std::string          GetFullTypeName() const;
};



class Asset
{
public:
	Asset();
	virtual ~Asset();

	virtual void serialize(yasli::Archive& ar);
	virtual void SerializeQuickEdit(yasli::Archive& ar);
	AssetId      GetAssetId() const { return id; }
	const AssetTraits* GetAssetTraits() const { return traits; }
	const std::string& GetName() const { return name; }

private:
	friend class AssetManager;
	AssetId            id = -1;
	std::string        name;
	const AssetTraits* traits;
};



}
