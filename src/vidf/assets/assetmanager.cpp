#include "pch.h"
#include "assetmanager.h"
#include "texture.h"

namespace vidf
{



AssetManager::AssetManager()
	: random(std::time(0))
{
	RegisterTextureTypes(this);
}



void AssetManager::RegisterType(AssetTraitsPtr type)
{
	types[type->GetTypeName()] = type;
}



AssetTraitsPtr AssetManager::FindTypeTraits(const char* typeName) const
{
	auto it = types.find(typeName);
	if (it != types.end())
		return it->second;
	return AssetTraitsPtr();
}



AssetPtr AssetManager::MakeAsset(const AssetTraits& traits, const char* name)
{
	assert(!traits.IsAbstract());
	AssetId id = MakeUniqueId();
	AssetRef& ref = assets.emplace(id, AssetRef()).first->second;
	ref.id = id;
	ref.name = name;
	ref.traits = &traits;
	ref.asset = traits.Create(ref);
	return ref.asset;
}



AssetId AssetManager::MakeUniqueId()
{
	AssetId id = random();
	assert(assets.find(id) == assets.end()); // oops, not so random, now is it?
	return id;
}



}
