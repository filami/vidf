#include "pch.h"
#include <complex>
#include <json11.hpp>
#include "Box2D/Box2D.h"
#include "vidf/common/matrix33.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/rendererdx11/wikidraw.h"
#include "magicavoxel/vox.h"
#include "worldrender/voxel.h"
#include "worldrender/voxelmodelrenderer.h"
#include "worldrender/worldrender.h"


using namespace std;
using namespace vidf;
using namespace json11;
using namespace vox;
using namespace vi::retro::render;
using namespace dx11;


#define VI_BUILD_VERSION "prealpha.1"
#define VI_BUILD_DATE    __DATE__ "-" __TIME__
#define VI_BUILD_TEXT    VI_BUILD_VERSION "-" VI_BUILD_DATE



namespace std
{

	template<> struct hash<vidf::Vector2i>
	{
		typedef vidf::Vector2i argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			result_type const h1(std::hash<int>{}(s.x));
			result_type const h2(std::hash<int>{}(s.y));
			return hash_combine(h1, h2);
		}
	};


}



namespace
{

vector<shared_ptr<VoxelModel>> LoadVoxFile(const char* path)
{
	vector<shared_ptr<VoxelModel>> output;

	ifstream ifs{ path, ifstream::binary };
	if (!ifs)
		return output;

	VoxData voxData;
	if (!ReadFox(ifs, voxData))
		return output;

	for (const auto& voxModel : voxData.models)
	{
		auto model = make_shared<VoxelModel>();
		output.push_back(model);
		for (uint i = 0; i < voxModel.voxels.size(); ++i)
		{
			const VoxVoxel voxVoxel = voxModel.voxels[i];
			const VoxColor voxColor = voxData.palette[voxModel.voxels[i].index - 1];
			Vector3i point = Vector3i(voxVoxel.x, voxVoxel.y, voxVoxel.z);
			Color color(voxColor.r / 255.0f, voxColor.g / 255.0f, voxColor.b / 255.0f, voxColor.a / 255.0f);
			model->InsertVoxel(point, color);
		}
	}

	return output;
}


}



namespace vi::retro::physics
{

	using namespace vidf;


	class DebugDraw : public b2Draw
	{
	public:
		DebugDraw(WikiDraw& _wikiDraw)
			: wikiDraw(_wikiDraw) {}

		virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
		virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
		virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override;
		virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override;
		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
		virtual void DrawTransform(const b2Transform& xf) override;
		virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override;

	private:
		WikiDraw& wikiDraw;
	};



	void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		wikiDraw.Begin(WikiDraw::Lines);
		wikiDraw.SetColor(
			uint8(color.r * 255.0f),
			uint8(color.g * 255.0f),
			uint8(color.b * 255.0f),
			uint8(color.a * 255.0f));
		for (uint i = 0; i < vertexCount - 1; ++i)
		{
			wikiDraw.PushVertex(Vector3f(vertices[i].x, 0.0f, vertices[i].y));
			wikiDraw.PushVertex(Vector3f(vertices[i + 1].x, 0.0f, vertices[i + 1].y));
		}
		wikiDraw.PushVertex(Vector3f(vertices[vertexCount - 1].x, 0.0f, vertices[vertexCount - 1].y));
		wikiDraw.PushVertex(Vector3f(vertices[0].x, 0.0f, vertices[0].y));
		wikiDraw.End();
	}



	void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		wikiDraw.Begin(WikiDraw::Triangles);
		wikiDraw.SetColor(
			uint8(color.r * 255.0f),
			uint8(color.g * 255.0f),
			uint8(color.b * 255.0f),
			uint8((color.a * 0.25f) * 255.0f));
		for (uint i = 1; i < vertexCount - 1; ++i)
		{
			wikiDraw.PushVertex(Vector3f(vertices[0].x, 0.0f, vertices[0].y));
			wikiDraw.PushVertex(Vector3f(vertices[i].x, 0.0f, vertices[i].y));
			wikiDraw.PushVertex(Vector3f(vertices[i + 1].x, 0.0f, vertices[i + 1].y));
		}
		wikiDraw.End();

	//	DrawPolygon(vertices, vertexCount, color);
	}



	void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
	{
		wikiDraw.Begin(WikiDraw::Triangles);
		wikiDraw.SetColor(
			uint8(color.r * 255.0f),
			uint8(color.g * 255.0f),
			uint8(color.b * 255.0f),
			uint8((color.a * 0.25f) * 255.0f));
		const uint numVertices = 12;
		Vector3f c = Vector3f(center.x, 0.0f, center.y);
		Vector3f v0 = Vector3f(center.x + radius, 0.0f, center.y);
		Vector3f v1;
		for (uint i = 1; i < numVertices; ++i)
		{
			const float t = 2 * PI * (i / float(numVertices));
			v1 = Vector3f(
				center.x + cos(t) * radius,
				0.0f,
				center.y + sin(t) * radius);
			wikiDraw.PushVertex(c);
			wikiDraw.PushVertex(v0);
			wikiDraw.PushVertex(v1);
			v0 = v1;
		}
		wikiDraw.End();
	}



	void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
	{
		DrawCircle(center, radius, color);
	}



	void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
	//	glLineWidth(2.0f);
	//	glColor4f(color.r, color.g, color.b, color.a);
	//	glBegin(GL_LINES);
	//	glVertex2f(p1.x, p1.y);
	//	glVertex2f(p2.x, p2.y);
	//	glEnd();
	}



	void DebugDraw::DrawTransform(const b2Transform& xf)
	{
	}



	void DebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
	{
	//	glLineWidth(2.0f);
	//	glColor4f(color.r, color.g, color.b, color.a);
	//	glBegin(GL_LINE_LOOP);
	//	glVertex2f(p.x - size * 0.5f, p.y);
	//	glVertex2f(p.x, p.y - size * 0.5f);
	//	glVertex2f(p.x + size * 0.5f, p.y);
	//	glVertex2f(p.x, p.y + size * 0.5f);
	//	glEnd();
	}



	class BodyReference
	{
	public:
		virtual void BeginContact(b2Contact* contact, uint thisId, uint otherId) { }
		virtual void EndContact(b2Contact* contact, uint thisId, uint otherId) { }
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold, uint thisId, uint otherId) { }
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse, uint thisId, uint otherId) { }
	};


	BodyReference* GetBodyReference(b2Contact* contact, uint index)
	{
		assert(index == 0 || index == 1);
		if (index == 0)
			return reinterpret_cast<BodyReference*>(contact->GetFixtureA()->GetBody()->GetUserData());
		return reinterpret_cast<BodyReference*>(contact->GetFixtureB()->GetBody()->GetUserData());
	}


	b2Fixture* GetFixture(b2Contact* contact, uint index)
	{
		assert(index == 0 || index == 1);
		if (index == 0)
			return contact->GetFixtureA();
		return contact->GetFixtureB();
	}


	b2Body* GetBody(b2Contact* contact, uint index)
	{
		assert(index == 0 || index == 1);
		if (index == 0)
			return contact->GetFixtureA()->GetBody();
		return contact->GetFixtureB()->GetBody();
	}


	class ContactListener : public b2ContactListener
	{
	public:
		virtual void BeginContact(b2Contact* contact) override
		{
			BodyReference* refA = GetBodyReference(contact, 0);
			BodyReference* refB = GetBodyReference(contact, 1);
			if (refA)
				refA->BeginContact(contact, 0, 1);
			if (refB)
				refB->BeginContact(contact, 1, 0);
		}
		virtual void EndContact(b2Contact* contact) override
		{
			BodyReference* refA = GetBodyReference(contact, 0);
			BodyReference* refB = GetBodyReference(contact, 1);
			if (refA)
				refA->EndContact(contact, 0, 1);
			if (refB)
				refB->EndContact(contact, 1, 0);
		}
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
		{
			BodyReference* refA = GetBodyReference(contact, 0);
			BodyReference* refB = GetBodyReference(contact, 1);
			if (refA)
				refA->PreSolve(contact, oldManifold, 0, 1);
			if (refB)
				refB->PreSolve(contact, oldManifold, 1, 0);
		}
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
		{
			BodyReference* refA = GetBodyReference(contact, 0);
			BodyReference* refB = GetBodyReference(contact, 1);
			if (refA)
				refA->PostSolve(contact, impulse, 0, 1);
			if (refB)
				refB->PostSolve(contact, impulse, 1, 0);
		}
	};



};



namespace vi::retro::core
{


	typedef vector<Vector2f> Polygon;



	struct Name
	{
		GUID   guid;
		string name;
	};

	typedef shared_ptr<Name> NamePtr;



	bool IsHex(char hex)
	{
		if (hex >= '0' && hex <= '9')
			return true;
		if (hex >= 'a' && hex <= 'f')
			return true;
		if (hex >= 'A' && hex <= 'F')
			return true;
		return false;
	}



	uint HexToInt(char hex)
	{
		if (hex >= '0' && hex <= '9')
			return hex - '0';
		if (hex >= 'a' && hex <= 'f')
			return hex - 'a' + 0xa;
		if (hex >= 'A' && hex <= 'F')
			return hex - 'A' + 0xa;
		return 0;
	}



	bool IsValidGUID(const GUID& guid)
	{
		GUID zero{};
		return guid != zero;
	}



	GUID ToGuid(const char* guidStr)
	{
		const GUID zero{};
		GUID out{};

		for (uint i = 0; i < 8; ++i)
		{
			if (!IsHex(*guidStr))
				return zero;
			out.Data1 = (out.Data1 << 4) | HexToInt(*guidStr);
			guidStr++;
		}
		if (*guidStr++ != '-')
			return zero;

		for (uint i = 0; i < 4; ++i)
		{
			if (!IsHex(*guidStr))
				return zero;
			out.Data2 = (out.Data2 << 4) | HexToInt(*guidStr);
			guidStr++;
		}
		if (*guidStr++ != '-')
			return zero;

		for (uint i = 0; i < 4; ++i)
		{
			if (!IsHex(*guidStr))
				return zero;
			out.Data3 = (out.Data3 << 4) | HexToInt(*guidStr);
			guidStr++;
		}
		if (*guidStr++ != '-')
			return zero;

		for (uint i = 0; i < 2; ++i)
		{
			for (uint j = 0; j < 2; ++j)
			{
				if (!IsHex(*guidStr))
					return zero;
				out.Data4[i] = (out.Data4[i] << 4) | HexToInt(*guidStr);
				guidStr++;
			}
		}
		if (*guidStr++ != '-')
			return zero;
		for (uint i = 2; i < 8; ++i)
		{
			for (uint j = 0; j < 2; ++j)
			{
				if (!IsHex(*guidStr))
					return zero;
				out.Data4[i] = (out.Data4[i] << 4) | HexToInt(*guidStr);
				guidStr++;
			}
		}

		return out;
	}



	Json JsonFromFile(const char* fileName)
	{
		ifstream ifs{ fileName };
		string text;
		while (!ifs.eof())
		{
			char buff[1024]{};
			ifs.read(buff, 1023);
			text += buff;
		}
		string err;
		Json json = Json::parse(text.c_str(), err);
		if (!err.empty())
			__debugbreak();
		return json;
	}



	void ReadJson(Vector2f& out, const Json& json)
	{
		out.x = atof(json["x"].string_value().c_str());
		out.y = atof(json["y"].string_value().c_str());
	}



	template<typename T>
	void ReadJson(vector<T>& out, const Json& json)
	{
		out.clear();
		out.reserve(json.array_items().size());
		for (const auto& item : json.array_items())
		{
			T data;
			ReadJson(data, item);
			out.push_back(data);
		}
	}


}



namespace vi::retro::world
{

	using namespace vi::retro::core;



	class AssetData;
	typedef shared_ptr<AssetData> AssetDataPtr;



	class AssetData
	{
	public:
		AssetData(NamePtr _partition, NamePtr _name)
			: partition(_partition)
			, name(_name) {}

		virtual void ReadJson(const Json& data) {}

	private:
		NamePtr partition;
		NamePtr name;
	};



	class AssetSetData : public AssetData
	{
	public:
	private:
		unordered_map<Name, AssetDataPtr> assets;
	};



	class BlockData : public AssetData
	{
	public:
		BlockData(NamePtr _partition, NamePtr _name)
			: AssetData(_partition, _name) {}

		const vector<Polygon>& GetPolygons() const { return polygons; }

		virtual void ReadJson(const Json& data) override
		{
			core::ReadJson(polygons, data["polygons"]);
		}

	private:
		vector<Polygon> polygons;
	};



	class CoumpoundData : public AssetData
	{
	public:
		struct BlockDataRef
		{
			Name     blockName;
			Vector2f position;
		};

	public:
	private:
		vector<BlockDataRef> blockRefs;
	};



	void LoadPartitionJson(const char* fileName)
	{
		Json data = JsonFromFile(fileName);

		NamePtr partition = make_shared<Name>();
		partition->name = fileName;
		partition->guid = ToGuid(data["guid"].string_value().c_str());

		const Json& assets = data["assets"];

		for (const auto& entry : assets.object_items())
		{
			const string& type = entry.first;

			NamePtr assetName = make_shared<Name>();

			const Json& entryData = entry.second;
			assetName->guid = ToGuid(entryData["guid"].string_value().c_str());
			if (!IsValidGUID(assetName->guid))
				continue;
			assetName->name = entryData["name"].string_value();

			AssetDataPtr objectData;
			if (type == "BlockData")
				objectData = make_shared<BlockData>(partition, assetName);
			if (objectData)
				objectData->ReadJson(entryData);
		}
	}



}



namespace vi::platform
{

	using namespace vidf;
	using namespace retro;
	using namespace physics;



	class Entity;


	class IInterface
	{
	public:
		template<typename Type>
		Type* _GetInterface()
		{
			return reinterpret_cast<Type*>(GetInterface(typeid(Type)));
		}

	protected:
		virtual IInterface* GetInterface(const type_info& typeInfo) { return nullptr; }
	};



	class EntityGrid
	{
	private:
		typedef unordered_multimap<Vector2i, Entity*> EntityMap;

	public:
		void AddEntity(Vector2i point, Entity* entity)
		{
			entityGrid.insert(EntityMap::value_type(point, entity));
		}

		template<typename Type>
		Type* FindEntity(Vector2i point) const
		{
			auto its = entityGrid.equal_range(point);
			for (auto it = its.first; it != its.second; ++it)
			{
				Entity* entity = it->second;
				Type* output = entity->_GetInterface<Type>();
				if (output)
					return output;
			}
			return nullptr;
		}

	private:
		EntityMap entityGrid;
	};



	class GameWorld;


	class Entity : public IInterface
	{
	public:
		Entity(GameWorld* gameWorld) {}
		virtual void Update(GameWorld* gameWorld, Time deltaTime) {}
	private:
	};



	class GameWorld
	{
	public:
		GameWorld(CanvasPtr canvas)
			: worldRender(canvas)
			, physics(b2Vec2(0.0f, 0.0f))
			, physicsDebug(worldRender.GetWikiDraw())
		{
			physics.SetContactListener(&contactListener);
			physicsDebug.SetFlags(b2Draw::e_shapeBit);
			physics.SetDebugDraw(&physicsDebug);
		}
		
		void Update()
		{
			Time deltaTime = timer.GetElapsed();
			accum += deltaTime;

			for (auto& entity : entitiesToAdd)
				entities.push_back(entity);
			entitiesToAdd.clear();

			while (accum > steptime)
			{
				physics.Step(steptime.AsFloat(), 4, 4);

				for (auto& entity : entities)
					entity->Update(this, steptime);

				accum -= steptime;
			}

			physics.DrawDebugData();
		}

		b2World&     GetPhysics()     { return physics; }
		WorldRender& GetWorldRender() { return worldRender; }
		EntityGrid&  GetEntityGrid()  { return entityGrid; }

		template<typename Type>
		Type* FindFirstEntity() const
		{
			for (const auto& entity : entities)
			{
				Type* type = entity->_GetInterface<Type>();
				if (type)
					return type;
			}
			return nullptr;
		}

		template<typename Type, typename ...Args>
		shared_ptr<Type> Spawn(Args... args)
		{
			auto entity = make_shared<Type>(this, args...);
			entitiesToAdd.push_back(entity);
			return entity;
		}

	private:
		Time            steptime = Time(1.0f / 60.0f);
		Time            accum;
		TimeCounter     timer;
		WorldRender     worldRender;
		b2World         physics;
		DebugDraw       physicsDebug;
		ContactListener contactListener;
		EntityGrid      entityGrid;
		vector<shared_ptr<Entity>> entities;
		deque<shared_ptr<Entity>>  entitiesToAdd;
	};



	class InputState
	{
	public:
		InputState(uint _key)
			: key(_key) {}

		void Update()
		{
			bool cur = (GetAsyncKeyState(key) & 0x8000) != 0;
			pressed = cur && !hold;
			released = !cur && hold;
			hold = cur;
		}

		bool Pressed()  const { return pressed; }
		bool Released() const { return released; }
		bool Hold()     const { return hold; }

	private:
		uint key;
		bool pressed = false;
		bool released = false;
		bool hold = false;
	};


	struct Camera;
	class  EntityGrid;



	class Camera : public Entity
	{
	public:
		struct PlayerState
		{
			Vector3f playerPos;
			bool     moving;
		};

	public:
		Camera(GameWorld* gameWorld)
			: Entity(gameWorld) {}

		virtual void Update(GameWorld* gameWorld, Time deltaTime) override
		{
			Entity::Update(gameWorld, deltaTime);
			snapTiming = Saturate(snapTiming + deltaTime.AsFloat());
			camPosition = Lerp(snapOrig, snapTarget, Hermite2(snapTiming));

			const float fov = Degrees2Radians(70.0f);
			const float nearPlane = 1.0f;
			const float farPlane = 1024.0f;
			const float aspect = 16.0f / 9.0f;
			const float playfieldHeight = 240 / 16.0f;
		//	const float distance = 8.5f;
		//	const float camHight = 0.75f;
			const float distance = 11.0f;
			const float camHight = 0.5f;

			Matrix44f perspective = PerspectiveFovRH(fov, aspect, nearPlane, farPlane);

			Vector3f up = Vector3f(0.0f, 0.0f, 1.0f);
			Vector3f target = camPosition;
			Vector3f position = target + Vector3f(0.0f, -distance, camHight);
			Matrix44f view = LookAtRH(position, target, up);

			gameWorld->GetWorldRender().SetView(perspective, view, position);
		}

		void SetPlayerState(const PlayerState& state)
		{
			if (snapped || (camPosition.x < state.playerPos.x - 4.0f || camPosition.x > state.playerPos.x + 4.0f) || camPosition.y != state.playerPos.z)
			{
				if (!snapped)
				{
					snapOrig = camPosition;
					snapTiming = 0.0f;
					snapped = true;
				}
				snapTarget = state.playerPos + Vector3f(0.0f, 0.0f, 3.0f);
			}
			if (!state.moving)
				snapped = false;
		}

	protected:
		virtual IInterface* GetInterface(const type_info& typeInfo) override
		{
			if (typeInfo == typeid(Camera))
				return this;
			else
				Entity::GetInterface(typeInfo);
		}
		
	private:
		Vector3f camPosition = Vector3f(0.0f, 0.0f, 0.0f);
		Vector3f snapOrig = Vector3f(0.0f, 0.0f, 0.0f);
		Vector3f snapTarget = Vector3f(0.0f, 0.0f, 0.0f);
		float    snapTiming = 0.0f;
		bool     snapped = false;
	};



	class Ladder : public Entity
	{
	public:
		Ladder(GameWorld* gameWorld, Vector2i _point, uint _height)
			: Entity(gameWorld)
			, point(_point)
			, height(_height)
		{
			for (uint i = 0; i < _height; ++i)
				gameWorld->GetEntityGrid().AddEntity(point + Vector2i(0, i), this);
		}

		Vector2i GetPoint()  const { return point; }
		uint     GetHeight() const { return height; }

	protected:
		virtual IInterface* GetInterface(const type_info& typeInfo) override
		{
			if (typeInfo == typeid(Ladder))
				return this;
			else
				Entity::GetInterface(typeInfo);
		}

	private:
		Vector2i point;
		uint     height;
	};




	const float gravityUp = 120.0f;
	const float gravityDown = gravityUp * 2.0f;
	const float moveSpeed = 8.0f;
	const float jumpSpeed = 16.0f;
	const float extraJumpTime = 0.1f;



	class Projectile : public Entity
	{
	public:
		Projectile(GameWorld* gameWorld, Vector2f position, Vector2f velocity, float radius, uint layer)
			: Entity(gameWorld)
		{
			b2BodyDef bd;
			bd.type = b2_dynamicBody;
			bd.fixedRotation = true;
			bd.gravityScale = 0.0f;

			bd.position.Set(position.x, position.y);
			object = gameWorld->GetPhysics().CreateBody(&bd);
			object->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));

			b2CircleShape circle;
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circle;
			fixtureDef.density = 0.5f;
			fixtureDef.friction = 0.0f;
			fixtureDef.restitution = 0.0f;

			const float width = 0.9f;
			const float height = 1.5f;
			const float stepHeight = 0.25f;

			b2Filter filter;
			filter.categoryBits = 1 << 4;
			filter.maskBits = 1;

			circle.m_radius = radius;
			b2Fixture* fixture = object->CreateFixture(&fixtureDef);
			fixture->SetFilterData(filter);

			models = LoadVoxFile("data/voxels/projectile.vox");
			modelHandle = gameWorld->GetWorldRender().GetVoxelModelRenderer().AddModel(models[0]);
		}

		virtual void Update(GameWorld* gameWorld, Time deltaTime) override
		{
			Entity::Update(gameWorld, deltaTime);

			const b2Vec2 position = object->GetPosition();

			gameWorld->GetWorldRender().GetVoxelModelRenderer().SetModel(
				modelHandle,
				models[0]);
			gameWorld->GetWorldRender().GetVoxelModelRenderer().SetModelWorldTM(
				modelHandle,
				Mul(Scale(1.0f / 16.0f), Translate(Vector3f(position.x, 0.0f, position.y))));
		}

	private:
		b2Body* object = nullptr;
		vector<shared_ptr<VoxelModel>> models;
		VoxelModelRenderer::Handle     modelHandle;
		float animTime = 0.0f;
		int   animFrame = 0;
		int   animFrameId = 0;
	};



	class Actor : public Entity
	{
	private:
		struct Physics : public BodyReference
		{
			virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold, uint thisId, uint otherId)
			{
				if (GetFixture(contact, thisId) == fixture)
				{
					b2Manifold* manifold = contact->GetManifold();
					if (manifold->localNormal.y > 1.0 / sqrt(2.0f))
					{
						onGround = true;
					}
					else
						contact->SetEnabled(false);
				}
			}

			b2Fixture* fixture   = nullptr;
			b2Body*    object    = nullptr;
			bool       onGround  = false;
			bool       prevOnGround = false;
		};

		enum class State
		{
			Moving,
			ChangingPlane,
			OnLadder,
		};

	public:
		Actor(GameWorld* gameWorld)
			: Entity(gameWorld)
		{
			b2BodyDef bd;
			bd.type = b2_dynamicBody;
			bd.fixedRotation = true;
			bd.gravityScale = 0.0f;
			bd.userData = &physics;

			bd.position.Set(0.0f, 4.0f);
			physics.object = gameWorld->GetPhysics().CreateBody(&bd);

			b2PolygonShape box;
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &box;
			fixtureDef.density = 0.5f;
			fixtureDef.friction = 0.0f;
			fixtureDef.restitution = 0.0f;

			const float width = 0.9f;
			const float height = 1.5f;
			const float stepHeight = 0.25f;

			b2Filter filter;
			filter.categoryBits = 1 << 4;
			filter.maskBits = 1;

			box.SetAsBox(
				width * 0.5f, (height - stepHeight) * 0.5f,
				b2Vec2(0.0f, stepHeight * 0.5f + height * 0.5f),
				0.0f);
			b2Fixture* fixture = physics.object->CreateFixture(&fixtureDef);
			fixture->SetFilterData(filter);

			box.SetAsBox(
				width * 0.5f, stepHeight * 0.5f,
				b2Vec2(0.0f, stepHeight * 0.5f),
				0.0f);
			physics.fixture = physics.object->CreateFixture(&fixtureDef);
			physics.fixture->SetFilterData(filter);

			models = LoadVoxFile("data/voxels/platformer_char_test02.vox");
			modelHandle = gameWorld->GetWorldRender().GetVoxelModelRenderer().AddModel(models[0]);
		}
		~Actor()
		{
		}

		virtual void Update(GameWorld* gameWorld, Time deltaTime) override
		{
			Entity::Update(gameWorld, deltaTime);

			inputMoveLeft.Update();
			inputMoveRight.Update();
			inputMoveUp.Update();
			inputMoveDown.Update();
			inputJump.Update();
			inputFire.Update();

			b2Vec2 position = physics.object->GetPosition();
			b2Vec2 curVelocity = physics.object->GetLinearVelocity();
			
			switch (state)
			{
			case State::Moving:
				Move(curVelocity, deltaTime.AsFloat());
				Jump(deltaTime.AsFloat(), false);
				Fire(deltaTime.AsFloat(), gameWorld);
				InteractDepth(gameWorld->GetEntityGrid());
				InteractLadder(gameWorld->GetEntityGrid());
				break;
			case State::ChangingPlane:
				ChangingPlane(deltaTime.AsFloat());
				break;
			case State::OnLadder:
				StepLadder(deltaTime.AsFloat());
				break;
			}
			
			physics.object->SetLinearVelocity(velocity);
			
			physics.prevOnGround = physics.onGround;
			physics.onGround = false;
			physics.object->SetAwake(true);

			//

			Camera::PlayerState camState;
			camState.playerPos = Vector3f(position.x, depth, position.y);
			camState.moving =
				inputMoveLeft.Hold() || inputMoveRight.Hold() ||
				inputMoveUp.Hold() || inputMoveDown.Hold();
			auto camera = gameWorld->FindFirstEntity<Camera>();
			if (camera)
				camera->SetPlayerState(camState);

			//

			uint animId = 0;

			struct Animation
			{
				vector<uint> frames;
				float frameTime;
				bool  loop;
			};
			static vector<Animation> animations =
			{
				{{0, 1, 2}, 1.0f / 10.0f, false},    // iddle
				{{3, 4, 5, 4}, 1.0f / 10.0f, true},	 // running
			};

			if (inputMoveLeft.Hold() || inputMoveRight.Hold())
				animId = 1;
			if ((inputMoveLeft.Pressed() && facingRight) || (inputMoveRight.Pressed() && !facingRight))
			{
				animTime = 0.0f;
				animFrame = 0;
			}
			if (inputMoveLeft.Pressed())
				facingRight = false;
			if (inputMoveRight.Pressed())
				facingRight = true;

			const auto& animation = animations[animId];
			animTime += deltaTime.AsFloat();
			if (animTime > animation.frameTime)
			{
				animTime -= animation.frameTime;
				animFrame++;
			}
			if (animFrame >= animation.frames.size())
				animFrame = animation.loop ? 0 : animation.frames.size() - 1;
			animFrameId = animation.frames[animFrame];
			gameWorld->GetWorldRender().GetVoxelModelRenderer().SetModel(
				modelHandle,
				models[animFrameId]);
			if (facingRight)
			{
				gameWorld->GetWorldRender().GetVoxelModelRenderer().SetModelWorldTM(
					modelHandle,
					Mul(Scale(1.0f / 16.0f), Translate(Vector3f(position.x - 1.0f, -1.0f + depth, position.y))));
			}
			else
			{
				gameWorld->GetWorldRender().GetVoxelModelRenderer().SetModelWorldTM(
					modelHandle,
					Mul(NonUniformScale(Vector3f(-1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f)), Translate(Vector3f(position.x + 1.0f, -1.0f + depth, position.y))));
			}
		}

	private:
		void Move(const b2Vec2 curVelocity, float timeStep)
		{
			if (physics.onGround)
			{
				if (inputMoveLeft.Hold())
					velocity.x = -moveSpeed;
				else if (inputMoveRight.Hold())
					velocity.x = moveSpeed;
				else
					velocity.x = 0.0f;
				if (!physics.prevOnGround)
					velocity.y = 0.0f;
			}
			else
			{
				const float gravity = curVelocity.y > 0.0f ? gravityUp : gravityDown;
				velocity.y = curVelocity.y - timeStep * gravity;
			}
		}

		void Jump(float timeStep, bool force)
		{
			if (inputJump.Pressed() && (physics.onGround || force))
			{
				jumping = true;
				jumpTime = 0.0f;
				velocity.y += jumpSpeed;
			}
			if (inputJump.Hold() && jumping)
			{
				if (jumpTime < extraJumpTime)
					velocity.y = jumpSpeed;
				else
					jumping = false;
				jumpTime += timeStep;
			}
			else
				jumping = false;
		}

		void Fire(float deltaTime, GameWorld* gameWorld)
		{
			fireColdDown = max(fireColdDown - deltaTime, 0.0f);
			if (fireColdDown != 0.0 || !inputFire.Pressed())
				return;
			const b2Vec2 playerPos = physics.object->GetPosition();
			fireColdDown = 1.0f / 5.0f;
			const float velocity = 25.0f;
			Vector2f projPosition{ playerPos.x, playerPos.y + 1.0f };
			Vector2f projVelocity{ facingRight ? velocity : -velocity, 0.0f };
			gameWorld->Spawn<Projectile>(
				projPosition, projVelocity,
				0.1f, depthLevel);
		}

		void ChangingPlane(float deltaTime)
		{
			const float target = depthLevel * 4.0f;
			const float dir = (target > depth) ? 1.0f : -1.0f;
			depth += moveSpeed * deltaTime * dir;
			if ((dir > 0.0 && depth > target) || (dir < 0.0 && depth < target))
			{
				depth = target;
				state = State::Moving;
			}
		}

		void StepLadder(float timeStep)
		{
			b2Vec2 position = physics.object->GetPosition();
			float xOffset = ladder->GetPoint().x - position.x + 0.5f;
			velocity.x = Clamp(xOffset / timeStep, -5.0f, 5.0f);

			if (inputMoveUp.Hold())
				velocity.y = 3.5f;
			else if (inputMoveDown.Hold())
				velocity.y = -3.5f;
			else
				velocity.y = 0.0f;
			if (inputJump.Pressed())
			{
				velocity.y = 5.0f;
				state = State::Moving;
			}
			if (inputMoveLeft.Pressed() || inputMoveRight.Pressed() || inputJump.Pressed())
				state = State::Moving;
		}

		void InteractLadder(const EntityGrid& entityGrid)
		{
			if (!inputMoveUp.Hold())
				return;
			b2Vec2 position = physics.object->GetPosition();
			Vector2i point = Vector2i(position.x, position.y + 1.5f);
			ladder = entityGrid.FindEntity<Ladder>(point);
			if (ladder != nullptr)
				state = State::OnLadder;
		}

		void InteractDepth(const EntityGrid& entityGrid)
		{
			if (velocity != b2Vec2(0, 0))
				return;

			if (inputMoveUp.Hold() && depthLevel < 3)
			{
				depthLevel++;
				state = State::ChangingPlane;
			}
			if (inputMoveDown.Hold() && depthLevel > 0)
			{
				depthLevel--;
				state = State::ChangingPlane;
			}

			b2Filter filter;
			filter.categoryBits = 1 << 4;
			filter.maskBits = 1 << depthLevel;
			for (b2Fixture* fixture = physics.object->GetFixtureList(); fixture; fixture = fixture->GetNext())
				fixture->SetFilterData(filter);
		}

	private:
		Physics physics;

		State  state = State::Moving;
		b2Vec2 velocity = b2Vec2(0.0f, 0.0f);
		float  jumpTime = 0.0f;
		bool   jumping = false;
		bool   facingRight = true;
		float  fireColdDown = 0.0f;

		float depth = 0.0f;
		int   depthLevel = 0;

		Ladder* ladder = nullptr;

		InputState inputMoveLeft{ VK_LEFT };
		InputState inputMoveRight{ VK_RIGHT };
		InputState inputMoveUp{ VK_UP };
		InputState inputMoveDown{ VK_DOWN };
		InputState inputJump{ 'S' };
		InputState inputFire{ 'A' };

		vector<shared_ptr<VoxelModel>> models;
		VoxelModelRenderer::Handle     modelHandle;
		float   animTime = 0.0f;
		int     animFrame = 0;
		int     animFrameId = 0;
	};



}



/////////////////////////////////////////////////////////



void PlatformGameplayTestDx11()
{
	using namespace vidf;
	using namespace vi::retro;
	using namespace vi::retro::core;
	using namespace vi::retro::physics;
	using namespace vi::retro::world;
	using namespace vi::platform;

	//

	CanvasDesc canvasDesc{};
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	GameWorld gameWorld{ canvas };
	
	//////

	ifstream ifs{ "data/voxels/platformer_test04.vox" , ios::binary };
	VoxData voxData;
	ReadFox(ifs, voxData);

#if 0

	vector<shared_ptr<VoxelModel>> models;
	for (const auto& voxModel : voxData.models)
	{
		auto model = make_shared<VoxelModel>();
		models.push_back(model);
		for (uint i = 0; i < voxModel.voxels.size(); ++i)
		{
			const VoxVoxel voxVoxel = voxModel.voxels[i];
			const VoxColor voxColor = voxData.palette[voxModel.voxels[i].index - 1];
			Vector3i point = Vector3i(voxVoxel.x, voxVoxel.y, voxVoxel.z);
			Color color(voxColor.r / 255.0f, voxColor.g / 255.0f, voxColor.b / 255.0f, voxColor.a / 255.0f);
			model->InsertVoxel(point, color);
		}
	}

	auto levelModel = make_shared<VoxelModel>();
	struct Transform
	{		
		Matrix33<int> rot{ zero };
		Vector3i      position{ zero };
		uint l = 0;
	};
	VoxTransverse(voxData, voxData.nodes[0].get(), Transform(),
		[&levelModel, &models](const VoxData& voxData, VoxNode* node, Transform& context)
	{
		if (auto transform = VoxToTransform(node))
		{
			assert(transform->numFrames == 1);
			auto it = transform->frameAttibutes[0].find("_t");
			if (it != transform->frameAttibutes[0].end())
			{
				int x, y, z;
				sscanf_s(it->second.c_str(), "%d %d %d", &x, &y, &z);
				context.position.x += x;
				context.position.y += y;
				context.position.z += z;
			}
			it = transform->frameAttibutes[0].find("_r");
			if (it != transform->frameAttibutes[0].end())
			{
				int v;
				sscanf_s(it->second.c_str(), "%d", &v);
				const uint idx0 = v & 0x3;
				const uint idx1 = (v >> 2) & 0x3;
				uint idx2 = 0;
				if (idx0 == 0 && idx1 == 1 || idx0 == 1 && idx1 == 0)
					idx2 = 2;
				else if (idx0 == 0 && idx1 == 2 || idx0 == 2 && idx1 == 0)
					idx2 = 1;
				const uint n0 = (v >> 4) & 0x1;
				const uint n1 = (v >> 5) & 0x1;
				const uint n2 = (v >> 6) & 0x1;
			
				memset(&context.rot, 0, sizeof(context.rot));
				context.rot[0 + idx0] = (n0 == 1) ? -1 : 1;
				context.rot[3 + idx1] = (n1 == 1) ? -1 : 1;
				context.rot[6 + idx2] = (n2 == 1) ? -1 : 1;

				if (n0 == 1)
					context.position[idx0]--;
				if (n1 == 1)
					context.position[idx1]--;
				if (n2 == 1)
					context.position[idx2]--;
			}
		}
		if (auto shape = VoxToShape(node))
		{
			for (uint i = 0; i < shape->models.size(); ++i)
			{
				VoxVec3i size = voxData.models[shape->models[i].modelId].size;
				Vector3i position =
					context.position -
					Mul(context.rot, Vector3i(size.x / 2, size.y / 2, size.z / 2));
				levelModel->AppendModel(
					*models[shape->models[i].modelId],
					position,
					context.rot);
			}
		}
		++context.l;
	});

	//////

	b2Body* levels[3]{ nullptr };

	for (uint level = 0; level < 3; ++level)
	{
		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0.0f, 0.0f);
		levels[level] = gameWorld.GetPhysics().CreateBody(&bd);

		vector<b2Vec2> points;
		b2PolygonShape polygon;

		for (const auto& it : levelModel->GetVoxels())
		{
			if (it.first.y != level * 32)
				continue;
			const float sz = 1.0f / 8.0f;
			Vector2f pt = Vector2f(it.first.x * sz, it.first.z * sz);
			points.emplace_back(pt.x + 0.0f, pt.y + 0.0f);
			points.emplace_back(pt.x + 0.0f, pt.y + sz  );
			points.emplace_back(pt.x + sz,   pt.y + sz  );
			points.emplace_back(pt.x + sz,   pt.y + 0.0f);
			polygon.Set(points.data(), points.size());
			b2Fixture* fixture = levels[level]->CreateFixture(&polygon, 0.0f);
			b2Filter filter;
			filter.categoryBits = 1 << level;
			filter.maskBits = 1 << 4;
			fixture->SetFilterData(filter);
			points.clear();
		}
	}

	auto& voxelRender = gameWorld.GetWorldRender().GetVoxelModelRenderer();
	auto handle = voxelRender.AddModel(levelModel);
	voxelRender.SetModelWorldTM(handle, Scale(1.0f / 8.0f));
#endif

	vector<shared_ptr<VoxelModel>> models;
	for (const auto& voxModel : voxData.models)
	{
		auto model = make_shared<VoxelModel>();
		models.push_back(model);
		for (uint i = 0; i < voxModel.voxels.size(); ++i)
		{
			const VoxVoxel voxVoxel = voxModel.voxels[i];
			const VoxColor voxColor = voxData.palette[voxModel.voxels[i].index - 1];
			Vector3i point = Vector3i(voxVoxel.x, voxVoxel.y, voxVoxel.z);
			Color color(voxColor.r / 255.0f, voxColor.g / 255.0f, voxColor.b / 255.0f, voxColor.a / 255.0f);
			model->InsertVoxel(point, color);
		}
	}

	//////

	auto& voxelRender = gameWorld.GetWorldRender().GetVoxelModelRenderer();

	auto levelModel = make_shared<VoxelModel>();

	struct Transform
	{
		Matrix33<int> rot{ zero };
		Vector3i      position{ zero };
	};
	struct Reference
	{
		Matrix44<int> worldTM;
		uint          modelIdx;
		VoxelModelRenderer::Handle handle;
	};
	deque<Reference> references;
	VoxTransverse(voxData, voxData.nodes[0].get(), Transform(),
		[&references, &levelModel, &models](const VoxData& voxData, VoxNode* node, Transform& context)
	{
		if (auto transform = VoxToTransform(node))
		{
			assert(transform->numFrames == 1);
			auto it = transform->frameAttibutes[0].find("_t");
			if (it != transform->frameAttibutes[0].end())
			{
				int x, y, z;
				sscanf_s(it->second.c_str(), "%d %d %d", &x, &y, &z);
				context.position.x += x;
				context.position.y += y;
				context.position.z += z;
			}
			it = transform->frameAttibutes[0].find("_r");
			if (it != transform->frameAttibutes[0].end())
			{
				int v;
				sscanf_s(it->second.c_str(), "%d", &v);
				const uint idx0 = v & 0x3;
				const uint idx1 = (v >> 2) & 0x3;
				uint idx2 = 0;
				if (idx0 == 0 && idx1 == 1 || idx0 == 1 && idx1 == 0)
					idx2 = 2;
				else if (idx0 == 0 && idx1 == 2 || idx0 == 2 && idx1 == 0)
					idx2 = 1;
				const uint n0 = (v >> 4) & 0x1;
				const uint n1 = (v >> 5) & 0x1;
				const uint n2 = (v >> 6) & 0x1;

				memset(&context.rot, 0, sizeof(context.rot));
				context.rot[0 + idx0] = (n0 == 1) ? -1 : 1;
				context.rot[3 + idx1] = (n1 == 1) ? -1 : 1;
				context.rot[6 + idx2] = (n2 == 1) ? -1 : 1;
			}
		}
		if (auto shape = VoxToShape(node))
		{
			for (uint i = 0; i < shape->models.size(); ++i)
			{
				VoxVec3i size = voxData.models[shape->models[i].modelId].size;
				Vector3i position =
					context.position -
					Mul(context.rot, Vector3i(size.x / 2, size.y / 2, size.z / 2));
				Reference reference;
				reference.modelIdx = shape->models[i].modelId;
				reference.worldTM = Mul(Matrix44<int>(context.rot), Translate(position));
				references.push_back(reference);

				levelModel->AppendModel(
					*models[shape->models[i].modelId],
					position,
					context.rot);
			}
		}
	});

	for (auto& reference : references)
	{
		reference.handle = voxelRender.AddModel(models[reference.modelIdx]);
		voxelRender.SetModelWorldTM(
			reference.handle,
			Mul(Matrix44f(reference.worldTM), Scale(1.0f / 8.0f)));
	}

	//////

	b2BodyDef bd;
	bd.type = b2_staticBody;
	bd.position.Set(0.0f, 0.0f);
	b2Body* levelBody = gameWorld.GetPhysics().CreateBody(&bd);
	vector<b2Vec2> points;
	b2PolygonShape polygon;

	for (uint level = 0; level < 3; ++level)
	{
		/*
		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0.0f, 0.0f);

		vector<b2Vec2> points;
		b2PolygonShape polygon;

		points.emplace_back(-16.0f,-1.0f);
		points.emplace_back(-16.0f, 0.0f);
		points.emplace_back( 16.0f, 0.0f);
		points.emplace_back( 16.0f,-1.0f);
		polygon.Set(points.data(), points.size());
		b2Fixture* fixture = levelBody->CreateFixture(&polygon, 0.0f);
		b2Filter filter;
		filter.categoryBits = 1 << level;
		filter.maskBits = 1 << 4;
		fixture->SetFilterData(filter);
		points.clear();
		*/

		for (const auto& it : levelModel->GetVoxels())
		{
			if (it.first.y != level * 32)
				continue;
			const float sz = 1.0f / 8.0f;
			Vector2f pt = Vector2f(it.first.x * sz, it.first.z * sz);
			points.emplace_back(pt.x + 0.0f, pt.y + 0.0f);
			points.emplace_back(pt.x + 0.0f, pt.y + sz);
			points.emplace_back(pt.x + sz, pt.y + sz);
			points.emplace_back(pt.x + sz, pt.y + 0.0f);
			polygon.Set(points.data(), points.size());
			b2Fixture* fixture = levelBody->CreateFixture(&polygon, 0.0f);
			b2Filter filter;
			filter.categoryBits = 1 << level;
			filter.maskBits = 1 << 4;
			fixture->SetFilterData(filter);
			points.clear();
		}
	}

	//////

	gameWorld.Spawn<Camera>();
	gameWorld.Spawn<Actor>();
	gameWorld.Spawn<Ladder>(Vector2i(4, 3), 4);
	
	WikiText wikiText{ gameWorld.GetWorldRender().GetRenderDevice() };
	InputState consoleBtn{ 'D' };
	bool consoleActive = false;

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		gameWorld.Update();

		consoleBtn.Update();
		if (consoleBtn.Pressed())
			consoleActive = !consoleActive;
		if (consoleActive)
		{
			auto& wikiDraw = gameWorld.GetWorldRender().GetWikiDraw();
			wikiDraw.PushProjViewTM(Matrix44f{ zero });
			wikiDraw.PushWorldTM(Matrix44f{ zero });
			wikiDraw.Begin(WikiDraw::Quads);
			wikiDraw.SetColor(0, 0, 0, 192);
			wikiDraw.PushVertex(Vector3f(-1.0f, 1.0f, 0.0f));
			wikiDraw.PushVertex(Vector3f(1.0f, 1.0f, 0.0f));
			wikiDraw.PushVertex(Vector3f(1.0f, 0.0f, 0.0f));
			wikiDraw.PushVertex(Vector3f(-1.0f, 0.0f, 0.0f));
			wikiDraw.End();
			wikiDraw.SetColor(192, 192, 192, 255);
			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.9f, 0.9f }, 0.035f,
				VI_BUILD_TEXT);
		}
				
		gameWorld.GetWorldRender().Draw();

		Time time = GetTime();
	}
}



/////////////////////////////////////////////////////////


namespace
{



void PlatformVoxelTestDx11()
{
	using namespace vidf;
	using namespace vi;
	using namespace vi::retro::render;

	vector<shared_ptr<VoxelModel>> character = LoadVoxFile("data/voxels/platformer_char_test02.vox");

	ifstream ifs{ "data/voxels/platformer_test03.vox" , ios::binary };
	VoxData voxData;
	ReadFox(ifs, voxData);

	vector<shared_ptr<VoxelModel>> models;
	for (const auto& voxModel : voxData.models)
	{
		auto model = make_shared<VoxelModel>();
		models.push_back(model);
		for (uint i = 0; i < voxModel.voxels.size(); ++i)
		{
			const VoxVoxel voxVoxel = voxModel.voxels[i];
			const VoxColor voxColor = voxData.palette[voxModel.voxels[i].index - 1];
			Vector3i point = Vector3i(voxVoxel.x, voxVoxel.y, voxVoxel.z);
			Color color(voxColor.r / 255.0f, voxColor.g / 255.0f, voxColor.b / 255.0f, voxColor.a / 255.0f);
			model->InsertVoxel(point, color);
		}
	}

	auto level = make_shared<VoxelModel>();
	struct Transform
	{
		Vector3i position{ zero };
		uint l = 0;
	};
	VoxTransverse(voxData, voxData.nodes[0].get(), Transform(),
		[&level, &models](const VoxData& voxData, VoxNode* node, Transform& context)
	{
		if (auto transform = VoxToTransform(node))
		{
			assert(transform->numFrames == 1);
			auto it = transform->frameAttibutes[0].find("_t");
			if (it != transform->frameAttibutes[0].end())
			{
				int x, y, z;
				sscanf_s(it->second.c_str(), "%d %d %d", &x, &y, &z);
				context.position.x += x;
				context.position.y += y;
				context.position.z += z;
			}
			it = transform->frameAttibutes[0].find("_r");
			if (it != transform->frameAttibutes[0].end())
			{
				it = it;
			}
		}
		if (auto shape = VoxToShape(node))
		{
			for (uint i = 0; i < shape->models.size(); ++i)
			{
				VoxVec3i size = voxData.models[shape->models[i].modelId].size;
				Vector3i position = context.position - Vector3i(size.x / 2, size.y / 2, size.z / 2);
				level->AppendModel(*models[shape->models[i].modelId], position);
			}
		}
		++context.l;
	});
		
	CanvasDesc canvasDesc{};
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	WorldRender worldRender{ canvas };

	proto::OrbitalCamera camera(canvas);
	camera.SetPerspective(Degrees2Radians(65.0f), 1.0f, 1000.0f);
	camera.SetCamera(Vector3f(zero), Quaternionf(zero), 5.0f);

	auto characterHandle = worldRender.GetVoxelModelRenderer().AddModel(character[0]);
	worldRender.GetVoxelModelRenderer().SetModelWorldTM(
		characterHandle,
		Mul(Scale(1 / 16.0f), Translate(Vector3f(0.0f, 0.0f, 1/8.0f))));

	auto levelHandle = worldRender.GetVoxelModelRenderer().AddModel(level);
	worldRender.GetVoxelModelRenderer().SetModelWorldTM(levelHandle, Scale(1 / 8.0f));

	TimeCounter counter;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();
		camera.Update(deltaTime);

		worldRender.SetView(camera.PerspectiveMatrix(), camera.ViewMatrix(), camera.Position());
		worldRender.Draw();

		Time time = GetTime();
	}
}



}

/////////////////////////////////////////////////////////

void Platform()
{
	PlatformGameplayTestDx11();
//	PlatformVoxelTestDx11();
}
