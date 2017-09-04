#include "pch.h"
#include "vidf/platform/filesystemutils.h"
#include "vidf/platform/system.h"
#include "assetmanager.h"
#include "texture.h"
#include "yasli/JSONIArchive.h"
#include "yasli/JSONOArchive.h"
#include "yasli/STL.h"
#include "yasli/Pointers.h"



//////////////////////////////////////////////////////////////////////////

namespace yasli
{

	template<class T>
	class StdSharedPtrSerializer : public yasli::PointerInterface
	{
	public:
		StdSharedPtrSerializer(std::shared_ptr<T>& ptr)
			: ptr_(ptr)
		{}

		const char* registeredTypeName() const {
			if (ptr_)
				return ClassFactory<T>::the().getRegisteredTypeName(ptr_.get());
			else
				return "";
		}
		void create(const char* typeName) const {
			YASLI_ASSERT(!ptr_ || ptr_.use_count() == 1);
			if (typeName && typeName[0] != '\0')
				ptr_.reset(factory()->create(typeName));
			else
				ptr_.reset((T*)0);
		}
		TypeID baseType() const { return TypeID::get<T>(); }
		virtual Serializer serializer() const {
			return Serializer(*ptr_);
		}
		void* get() const {
			return reinterpret_cast<void*>(ptr_.get());
		}
		const void* handle() const {
			return &ptr_;
		}
		TypeID pointerType() const { return TypeID::get<SharedPtr<T> >(); }
		virtual ClassFactory<T>* factory() const { return &ClassFactory<T>::the(); }
	protected:
		std::shared_ptr<T>& ptr_;
	};


	template<class T>
	bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, std::shared_ptr<T>& ptr, const char* name, const char* label)
	{
		StdSharedPtrSerializer<T> serializer(ptr);
		return ar(static_cast<yasli::PointerInterface&>(serializer), name, label);
	}


}


//////////////////////////////////////////////////////////////////////////


namespace vidf
{



AssetManager::AssetManager()
	: random(std::time(0))
{
	RegisterTextureTypes(this);
	LoadAssetFile("assets/at the root.vi.asset");
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
	AssetPtr assetPtr = traits.Create();
	assets.emplace(id, assetPtr);
	assetPtr->id = id;
	assetPtr->name = name;
	assetPtr->traits = &traits;
	SaveAsset(assetPtr);
	return assetPtr;
}



void AssetManager::SaveAsset(AssetPtr asset)
{
	std::string fileName = std::string("assets/") + asset->name + ".vi.asset";

	std::string filePath = GetPathFromFilePath(fileName);
	MakeFolder(filePath.c_str());

	struct Adapter
	{
		AssetPtr asset;
		void serialize(yasli::Archive& ar)
		{
			ar(asset, "Asset");
		}
	} adapter;
	adapter.asset = asset;

	yasli::JSONOArchive ar;
	ar(adapter);
	ar.save(fileName.c_str());
}



AssetPtr AssetManager::LoadAssetFile(const char* filePath)
{
	struct Adapter
	{
		AssetPtr asset;
		void serialize(yasli::Archive& ar)
		{
			ar(asset, "Asset");
		}
	} adapter;
	adapter.asset;

	yasli::JSONIArchive ar;
	ar.load(filePath);
	ar(adapter);

	return AssetPtr();
}



AssetId AssetManager::MakeUniqueId()
{
	AssetId id = random();
	assert(assets.find(id) == assets.end()); // oops, not so random, now is it?
	return id;
}



}
