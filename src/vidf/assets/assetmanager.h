#pragma once

#include "asset.h"

namespace vidf
{

class AssetManager
{
public:
	typedef std::unordered_map<AssetId, AssetRef> AssetMap;

public:
	AssetManager();

	AssetPtr MakeAsset(AssetTraits* traits, const char* name);
	AssetMap::const_iterator begin() const { return assets.begin(); }
	AssetMap::const_iterator end() const { return assets.end(); }

private:
	AssetId MakeUniqueId();

private:
	AssetMap        assets;
	std::mt19937_64 random;
};

}
