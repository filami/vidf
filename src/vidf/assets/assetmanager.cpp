#include "pch.h"
#include "assetmanager.h"

namespace vidf
{



AssetManager::AssetManager()
	: random(std::time(0))
{
}



AssetPtr AssetManager::MakeAsset(AssetTraits* traits, const char* name)
{
	AssetId id = MakeUniqueId();
	AssetRef& ref = assets.emplace(id, AssetRef()).first->second;
	ref.id = id;
	ref.name = name;
	ref.traits = traits;
	ref.asset = std::shared_ptr<Asset>(traits->Create(ref));
	return ref.asset;
}



AssetId AssetManager::MakeUniqueId()
{
	AssetId id = random();
	assert(assets.find(id) == assets.end()); // oops, not so random, now is it?
	return id;
}



}
