#pragma once

#include "asset.h"

namespace vidf
{

class AssetManager
{
public:
	typedef std::unordered_map<std::string, AssetTraitsPtr> AssetTypeMap;
	typedef std::unordered_map<AssetId, AssetPtr> AssetMap;

public:
	AssetManager();

	void           RegisterType(AssetTraitsPtr type);
	AssetTraitsPtr FindTypeTraits(const char* typeName) const;
	AssetPtr       MakeAsset(const AssetTraits& traits, const char* name);
	AssetMap::const_iterator begin() const { return assets.begin(); }
	AssetMap::const_iterator end() const { return assets.end(); }

private:
	AssetId  MakeUniqueId();
	void     SaveAsset(AssetPtr asset);
	AssetPtr LoadAssetFile(const char* filePath);

private:
	AssetTypeMap    types;
	AssetMap        assets;
	std::mt19937_64 random;
};

}
