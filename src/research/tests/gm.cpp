#include "pch.h"
#include <vidf/common/random.h>
#include "gmMachine.h"
#include "gmArrayLib.h"
#include "gmCall.h"
#include "gmGCRoot.h"
#include "gmGCRootUtil.h"
#include "gmHelpers.h"
#include "gmMathLib.h"
#include "gmStringLib.h"
#include "gmSystemLib.h"
#include "gmVector3Lib.h"

using namespace vidf;
using namespace proto;



class DebugDraw
{
private:
	struct Vertex
	{
		Vector3f vertex = Vector3f(zero);
		Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	};
	struct Dispatch
	{
		GLenum type = 0;
		uint begin = 0;
		uint end = 0;
	};

public:
	void Begin(GLenum type)
	{
		currentDispatch.type = type;
		currentDispatch.begin = currentDispatch.end = vertices.size();
	}

	void End()
	{
		currentDispatch.end = vertices.size();
		dispatches.push_back(currentDispatch);
	}

	void AddVertex(Vector3f v)
	{
		currentVertex.vertex = v;
		vertices.push_back(currentVertex);
	}

	void SetColor(Color c)
	{
		currentVertex.color = c;
	}

	void Flush()
	{
		for (const auto& dispatch : dispatches)
		{
			glBegin(dispatch.type);
			for (uint i = dispatch.begin; i < dispatch.end; ++i)
			{
				glColor4fv(&vertices[i].color.r);
				glVertex3fv(&vertices[i].vertex.x);
			}
			glEnd();
		}
		dispatches.clear();
		vertices.clear();
	}

	void SetOrtho2D(Vector2f center, float size)
	{
		float aspect = 1.0f;
		float sizeX = size * aspect * 0.5f;
		float sizeY = size * 0.5f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(
			center.x - sizeX, center.x + sizeX,
			center.y - sizeY, center.y + sizeY);
		glMatrixMode(GL_MODELVIEW);
	}

	void SetPerspective(float _radFoV, float _nearPlane, float _farPlane)
	{
		glMatrixMode(GL_PROJECTION);
		Matrix44f perspective = PerspectiveFovRH(_radFoV, 1.0f, _nearPlane, _farPlane);
		glLoadMatrixf(&perspective.m00);
		glMatrixMode(GL_MODELVIEW);
	}

	void SetCamera(const Vector3f _position, const Vector3f _target, const Vector3f _up)
	{
		Matrix44f view = LookAtRH(_position, _target, _up);
		glLoadMatrixf(&view.m00);
	}

private:
	std::vector<Vertex> vertices;
	std::vector<Dispatch> dispatches;
	Vertex currentVertex;
	Dispatch currentDispatch;
};



//////////////////////////////////////////////////////////////////////////


DebugDraw* gDebugDraw;


int BindDebugDrawBegin(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(type, 0);
	gDebugDraw->Begin(type);
	return GM_OK;
}

int BindDebugDrawEnd(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(0);
	gDebugDraw->End();
	return GM_OK;
}

int BindDebugDrawAddVertex(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, vec, 0);
	gDebugDraw->AddVertex(Vector3f(vec[0], vec[1], vec[2]));
	return GM_OK;
}

int BindDebugSetColor(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, color, 0);
	const float a = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	gDebugDraw->SetColor(Color(color[0], color[1], color[2], a));
	return GM_OK;
}

int BindDebugSetOrtho2D(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, center, 0);
	const float size = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	gDebugDraw->SetOrtho2D(Vector2f(center[0], center[1]), size);
	return GM_OK;
}

int BindDebugSetPerspective(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	const float fov = gmGetFloatOrIntParamAsFloat(a_thread, 0);
	const float nearPlane = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	const float farPlane = gmGetFloatOrIntParamAsFloat(a_thread, 2);
	gDebugDraw->SetPerspective(fov, nearPlane, farPlane);
	return GM_OK;
}

int BindDebugSetCamera(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, position, 0);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, target, 1);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, up, 2);
	gDebugDraw->SetCamera(
		Vector3f(position[0], position[1], position[2]),
		Vector3f(target[0], target[1], target[2]),
		Vector3f(up[0], up[1], up[2]));
	return GM_OK;
}

void GmBindDebugDraw(gmMachine* gmVM)
{
	gmTableObject* table = gmVM->AllocTableObject();
	gmVM->GetGlobals()->Set(gmVM, "PrimitiveType", gmVariable(GM_TABLE, table->GetRef()));
	table->Set(gmVM, "Points",        gmVariable(GM_INT, GL_POINTS));
	table->Set(gmVM, "Lines",         gmVariable(GM_INT, GL_LINES));
	table->Set(gmVM, "LineLoop",      gmVariable(GM_INT, GL_LINE_LOOP));
	table->Set(gmVM, "LineStrip",     gmVariable(GM_INT, GL_LINE_STRIP));
	table->Set(gmVM, "Triangles",     gmVariable(GM_INT, GL_TRIANGLES));
	table->Set(gmVM, "TriangleStrip", gmVariable(GM_INT, GL_TRIANGLE_STRIP));
	table->Set(gmVM, "TriangleFan",   gmVariable(GM_INT, GL_TRIANGLE_FAN));
	table->Set(gmVM, "Quads",         gmVariable(GM_INT, GL_QUADS));
	table->Set(gmVM, "QuadStrip",     gmVariable(GM_INT, GL_QUAD_STRIP));
	table->Set(gmVM, "Polygon",       gmVariable(GM_INT, GL_POLYGON));

	static gmFunctionEntry library[] =
	{
		{ "Begin",          BindDebugDrawBegin },
		{ "End",            BindDebugDrawEnd },
		{ "AddVertex",      BindDebugDrawAddVertex },
		{ "SetColor",       BindDebugSetColor },
		{ "SetOrtho2D",     BindDebugSetOrtho2D },
		{ "SetPerspective", BindDebugSetPerspective },
		{ "SetCamera",      BindDebugSetCamera },
	};

	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"DebugDraw");
}


//////////////////////////////////////////////////////////////////////////


int BindInputGetState(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(key, 0);
	a_thread->PushInt(GetAsyncKeyState(key) & 0x8000 ? 1 : 0);
	return GM_OK;
}

void GmBindInput(gmMachine* gmVM)
{
	gmTableObject* table = gmVM->AllocTableObject();
	gmVM->GetGlobals()->Set(gmVM, "InputKeys", gmVariable(GM_TABLE, table->GetRef()));
	table->Set(gmVM, "Space", gmVariable(GM_INT, VK_SPACE));
	table->Set(gmVM, "Escape", gmVariable(GM_INT, VK_ESCAPE));
	table->Set(gmVM, "Return", gmVariable(GM_INT, VK_RETURN));
	table->Set(gmVM, "Up",     gmVariable(GM_INT, VK_UP));
	table->Set(gmVM, "Down",   gmVariable(GM_INT, VK_DOWN));
	table->Set(gmVM, "Right",  gmVariable(GM_INT, VK_RIGHT));
	table->Set(gmVM, "Left",   gmVariable(GM_INT, VK_LEFT));

	static gmFunctionEntry library[] =
	{
		{ "GetState",     BindInputGetState },
	};
	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"Input");
}


//////////////////////////////////////////////////////////////////////////

float frameTime = 0.0f;

int BindGameFrameTime(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(0);
	a_thread->PushFloat(frameTime);
	return GM_OK;
}

void GmBindGame(gmMachine* gmVM)
{
	static gmFunctionEntry library[] =
	{
		{ "FrameTime",     BindGameFrameTime },
	};
	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"Game");
}


//////////////////////////////////////////////////////////////////////////

class ScriptReload : public FileChangeNotification
{
public:
	ScriptReload(const char* _mainSource)
		: mainSource(_mainSource) {}

	void ReloadScript()
	{
		gmVM = std::make_unique<gmMachine>();
		gmVM->SetDebugMode(true);

		gmBindArrayLib(gmVM.get());
		gmBindMathLib(gmVM.get());
		gmBindStringLib(gmVM.get());
		gmBindSystemLib(gmVM.get());
		gmBindVector3Lib(gmVM.get());

		GmBindDebugDraw(gmVM.get());
		GmBindInput(gmVM.get());
		GmBindGame(gmVM.get());

		std::cout << "Compiling " << mainSource << std::endl;

		std::string code;
		std::ifstream ifs(mainSource);
		while (ifs)
		{
			const uint sz = 256;
			char buff[sz] = {};
			ifs.read(buff, sz-1);
			code += buff;
		}
		gmVM->ExecuteString(code.c_str(), nullptr, true, mainSource);
	}

	virtual void Notify(const wchar_t* fileName) override
	{
		ReloadScript();
	}

	gmMachine* GetMachine() { return gmVM.get(); }

private:
	std::unique_ptr<gmMachine> gmVM;
	const char* mainSource;
};

//////////////////////////////////////////////////////////////////////////

void TestGM()
{
#if 0
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc());

	DebugDraw debugDraw;
	gDebugDraw = &debugDraw;

	ScriptReload scriptReload("data/scripts/main.gm");
	fileChangeNotificationSystem.AddFileNotification(L"data\\scripts\\main.gm", &scriptReload);
	fileChangeNotificationSystem.StartNotificationListener();
	scriptReload.ReloadScript();

	TimeCounter counter;
	while (protoGL.Update())
	{
		fileChangeNotificationSystem.DispatchNotifications();

		Time deltaTime = counter.GetElapsed();
		frameTime = deltaTime.AsFloat();
		scriptReload.GetMachine()->Execute(deltaTime.AsMiliseconds());
		bool first = true;
		while (const char* log = scriptReload.GetMachine()->GetLog().GetEntry(first))
			std::cout << log;
		scriptReload.GetMachine()->GetLog().Reset();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		debugDraw.Flush();

		protoGL.Swap();
	}
#endif

	const uint worldSize = 1024;

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc());

	CameraOrtho2D camera(protoGL.GetCanvas());
	camera.SetCamera(Vector2f(worldSize*0.5f, worldSize*0.5f), float(worldSize)*1.25f);

	const auto DrawGrid = [](uint width, uint height)
	{
		glBegin(GL_LINES);
		for (uint x = 0; x <= width; ++x)
		{
			glVertex2f(float(x), 0.0f);
			glVertex2f(float(x), float(height));
		}
		for (uint y = 0; y <= width; ++y)
		{
			glVertex2f(0.0f, float(y));
			glVertex2f(float(width), float(y));
		}
		glEnd();
	};

	enum class State : uint8
	{
		Dead,
		Alive,
	};
	typedef std::array<State, worldSize*worldSize> World;
	std::unique_ptr<World> world[2] =
	{
		std::make_unique<World>(),
		std::make_unique<World>(),
	};
	uint from = 0;
	uint to = 1;

	Rand48 rand;
	UniformInt<uint> stateRand(0, 1);
	for (State& state : *world[from])
		state = stateRand(rand) == 0 ? State::Alive : State::Dead;
	*world[to] = *world[from];

	const auto DrawWorld = [](const World& world, const uint worldSize)
	{
		glBegin(GL_QUADS);
		for (uint idx = 0, y = 0; y < worldSize; ++y)
		{
			for (uint x = 0; x < worldSize; ++x, ++idx)
			{
				if (world[idx] == State::Dead)
					continue;
				glVertex2f(float(x) + 0.0f, float(y) + 0.0f);
				glVertex2f(float(x) + 1.0f, float(y) + 0.0f);
				glVertex2f(float(x) + 1.0f, float(y) + 1.0f);
				glVertex2f(float(x) + 0.0f, float(y) + 1.0f);
			}
		}
		glEnd();
	};

	const auto UpdateWorld = [](World* toWorld, const World& fromWorld, const uint worldSize)
	{
		auto NeightborCount = [&fromWorld, worldSize](uint x, uint y)
		{
			const uint mask = worldSize - 1;
			const int minx = x - 1;
			const int miny = y - 1;
			const int maxx = x + 1;
			const int maxy = y + 1;
			uint count = 0;
			for (int ny = miny; ny <= maxy; ++ny)
			{
				for (int nx = minx; nx <= maxx; ++nx)
				{
					const uint mx = nx & mask;
					const uint my = ny & mask;
					count += uint(fromWorld[mx + my*worldSize]);
				}
			}
			count -= uint(fromWorld[x + y*worldSize]);
			return count;
		};
		for (uint y = 0; y < worldSize; ++y)
		{
			for (uint x = 0; x < worldSize; ++x)
			{
				const uint idx = x + y*worldSize;
				const uint count = NeightborCount(x, y);
				if (fromWorld[idx] == State::Dead && count == 3)
					(*toWorld)[idx] = State::Alive;
				else if (fromWorld[idx] == State::Alive && (count < 2 || count > 3))
					(*toWorld)[idx] = State::Dead;
				else
					(*toWorld)[idx] = fromWorld[idx];
			}
		}
	};

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		// for (uint i = 0; i < 2; ++i)
		{
			UpdateWorld(world[to].get(), *world[from], worldSize);
			from = !from;
			to = !to;
		}

		glColor4ub(0, 0, 0, 255);
		DrawWorld(*world[to], worldSize);

		// glColor4ub(210, 210, 210, 255);
		// DrawGrid(worldSize, worldSize);

		protoGL.Swap();
		// ::Sleep(200);
	}
}


#if 0
#include "pch.h"
#include "gmMachine.h"
#include "gmArrayLib.h"
#include "gmCall.h"
#include "gmGCRoot.h"
#include "gmGCRootUtil.h"
#include "gmHelpers.h"
#include "gmMathLib.h"
#include "gmStringLib.h"
#include "gmSystemLib.h"
#include "gmVector3Lib.h"
#include "proto/mesh.h"
#include "formats/image.h"
#include "formats/imagegl.h"
#include "common/plane.h"

using namespace vidf;
using namespace proto;



class DebugDraw
{
private:
	struct Vertex
	{
		Vector3f vertex = Vector3f(zero);
		Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	};
	struct Dispatch
	{
		GLenum type = 0;
		uint begin = 0;
		uint end = 0;
	};

public:
	void Begin(GLenum type)
	{
		currentDispatch.type = type;
		currentDispatch.begin = currentDispatch.end = vertices.size();
	}

	void End()
	{
		currentDispatch.end = vertices.size();
		dispatches.push_back(currentDispatch);
	}

	void AddVertex(Vector3f v)
	{
		currentVertex.vertex = v;
		vertices.push_back(currentVertex);
	}

	void SetColor(Color c)
	{
		currentVertex.color = c;
	}

	void Flush()
	{
		for (const auto& dispatch : dispatches)
		{
			glBegin(dispatch.type);
			for (uint i = dispatch.begin; i < dispatch.end; ++i)
			{
				glColor4fv(&vertices[i].color.r);
				glVertex3fv(&vertices[i].vertex.x);
			}
			glEnd();
		}
		dispatches.clear();
		vertices.clear();
	}

	void SetOrtho2D(Vector2f center, float size)
	{
		float aspect = 1.0f;
		float sizeX = size * aspect * 0.5f;
		float sizeY = size * 0.5f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(
			center.x - sizeX, center.x + sizeX,
			center.y - sizeY, center.y + sizeY);
		glMatrixMode(GL_MODELVIEW);
	}

	void SetPerspective(float _radFoV, float _nearPlane, float _farPlane)
	{
		glMatrixMode(GL_PROJECTION);
		Matrix44f perspective = PerspectiveFovRH(_radFoV, 1.0f, _nearPlane, _farPlane);
		glLoadMatrixf(&perspective.m00);
		glMatrixMode(GL_MODELVIEW);
	}

	void SetCamera(const Vector3f _position, const Vector3f _target, const Vector3f _up)
	{
		Matrix44f view = LookAtRH(_position, _target, _up);
		glLoadMatrixf(&view.m00);
	}

private:
	std::vector<Vertex> vertices;
	std::vector<Dispatch> dispatches;
	Vertex currentVertex;
	Dispatch currentDispatch;
};



//////////////////////////////////////////////////////////////////////////


DebugDraw* gDebugDraw;


int BindDebugDrawBegin(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(type, 0);
	gDebugDraw->Begin(type);
	return GM_OK;
}

int BindDebugDrawEnd(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(0);
	gDebugDraw->End();
	return GM_OK;
}

int BindDebugDrawAddVertex(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, vec, 0);
	gDebugDraw->AddVertex(Vector3f(vec[0], vec[1], vec[2]));
	return GM_OK;
}

int BindDebugSetColor(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, color, 0);
	const float a = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	gDebugDraw->SetColor(Color(color[0], color[1], color[2], a));
	return GM_OK;
}

int BindDebugSetOrtho2D(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, center, 0);
	const float size = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	gDebugDraw->SetOrtho2D(Vector2f(center[0], center[1]), size);
	return GM_OK;
}

int BindDebugSetPerspective(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	const float fov = gmGetFloatOrIntParamAsFloat(a_thread, 0);
	const float nearPlane = gmGetFloatOrIntParamAsFloat(a_thread, 1);
	const float farPlane = gmGetFloatOrIntParamAsFloat(a_thread, 2);
	gDebugDraw->SetPerspective(fov, nearPlane, farPlane);
	return GM_OK;
}

int BindDebugSetCamera(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, position, 0);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, target, 1);
	GM_CHECK_USER_PARAM(float*, GM_VECTOR3, up, 2);
	gDebugDraw->SetCamera(
		Vector3f(position[0], position[1], position[2]),
		Vector3f(target[0], target[1], target[2]),
		Vector3f(up[0], up[1], up[2]));
	return GM_OK;
}

void GmBindDebugDraw(gmMachine* gmVM)
{
	gmTableObject* table = gmVM->AllocTableObject();
	gmVM->GetGlobals()->Set(gmVM, "PrimitiveType", gmVariable(GM_TABLE, table->GetRef()));
	table->Set(gmVM, "Points", gmVariable(GM_INT, GL_POINTS));
	table->Set(gmVM, "Lines", gmVariable(GM_INT, GL_LINES));
	table->Set(gmVM, "LineLoop", gmVariable(GM_INT, GL_LINE_LOOP));
	table->Set(gmVM, "LineStrip", gmVariable(GM_INT, GL_LINE_STRIP));
	table->Set(gmVM, "Triangles", gmVariable(GM_INT, GL_TRIANGLES));
	table->Set(gmVM, "TriangleStrip", gmVariable(GM_INT, GL_TRIANGLE_STRIP));
	table->Set(gmVM, "TriangleFan", gmVariable(GM_INT, GL_TRIANGLE_FAN));
	table->Set(gmVM, "Quads", gmVariable(GM_INT, GL_QUADS));
	table->Set(gmVM, "QuadStrip", gmVariable(GM_INT, GL_QUAD_STRIP));
	table->Set(gmVM, "Polygon", gmVariable(GM_INT, GL_POLYGON));

	static gmFunctionEntry library[] =
	{
		{ "Begin",          BindDebugDrawBegin },
		{ "End",            BindDebugDrawEnd },
		{ "AddVertex",      BindDebugDrawAddVertex },
		{ "SetColor",       BindDebugSetColor },
		{ "SetOrtho2D",     BindDebugSetOrtho2D },
		{ "SetPerspective", BindDebugSetPerspective },
		{ "SetCamera",      BindDebugSetCamera },
	};

	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"DebugDraw");
}


//////////////////////////////////////////////////////////////////////////


int BindInputGetState(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(key, 0);
	a_thread->PushInt(GetAsyncKeyState(key) & 0x8000 ? 1 : 0);
	return GM_OK;
}

void GmBindInput(gmMachine* gmVM)
{
	gmTableObject* table = gmVM->AllocTableObject();
	gmVM->GetGlobals()->Set(gmVM, "InputKeys", gmVariable(GM_TABLE, table->GetRef()));
	table->Set(gmVM, "Space", gmVariable(GM_INT, VK_SPACE));
	table->Set(gmVM, "Escape", gmVariable(GM_INT, VK_ESCAPE));
	table->Set(gmVM, "Return", gmVariable(GM_INT, VK_RETURN));
	table->Set(gmVM, "Up", gmVariable(GM_INT, VK_UP));
	table->Set(gmVM, "Down", gmVariable(GM_INT, VK_DOWN));
	table->Set(gmVM, "Right", gmVariable(GM_INT, VK_RIGHT));
	table->Set(gmVM, "Left", gmVariable(GM_INT, VK_LEFT));

	static gmFunctionEntry library[] =
	{
		{ "GetState",     BindInputGetState },
	};
	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"Input");
}


//////////////////////////////////////////////////////////////////////////

float frameTime = 0.0f;

int BindGameFrameTime(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(0);
	a_thread->PushFloat(frameTime);
	return GM_OK;
}

void GmBindGame(gmMachine* gmVM)
{
	static gmFunctionEntry library[] =
	{
		{ "FrameTime",     BindGameFrameTime },
	};
	gmVM->RegisterLibrary(
		library, sizeof(library) / sizeof(library[0]),
		"Game");
}


//////////////////////////////////////////////////////////////////////////

class ScriptReload : public FileChangeNotification
{
public:
	ScriptReload(const char* _mainSource)
		: mainSource(_mainSource) {}

	void ReloadScript()
	{
		gmVM = std::make_unique<gmMachine>();
		gmVM->SetDebugMode(true);

		gmBindArrayLib(gmVM.get());
		gmBindMathLib(gmVM.get());
		gmBindStringLib(gmVM.get());
		gmBindSystemLib(gmVM.get());
		gmBindVector3Lib(gmVM.get());

		GmBindDebugDraw(gmVM.get());
		GmBindInput(gmVM.get());
		GmBindGame(gmVM.get());

		std::cout << "Compiling " << mainSource << std::endl;

		std::string code;
		std::ifstream ifs(mainSource);
		while (ifs)
		{
			const uint sz = 256;
			char buff[sz] = {};
			ifs.read(buff, sz - 1);
			code += buff;
		}
		gmVM->ExecuteString(code.c_str(), nullptr, true, mainSource);
	}

	virtual void Notify(const wchar_t* fileName) override
	{
		ReloadScript();
	}

	gmMachine* GetMachine() { return gmVM.get(); }

private:
	std::unique_ptr<gmMachine> gmVM;
	const char* mainSource;
};

//////////////////////////////////////////////////////////////////////////

class InputListener : public CanvasListener
{
public:
	InputListener(ScriptReload& _script)
		: script(_script) {}

	virtual void KeyUp(KeyCode keyCode)
	{
		if (keyCode == VK_F5)
			script.ReloadScript();
	}

private:
	ScriptReload& script;
};

//////////////////////////////////////////////////////////////////////////
#if 0
/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#include <math.h>
#include <stdio.h>
/*
#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
dest[2] = v1[0] * v2[1] - v1[1] * v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])



#define SUB(dest,v1,v2) \

dest[0] = v1[0] - v2[0]; \

dest[1] = v1[1] - v2[1]; \

dest[2] = v1[2] - v2[2];
*/

/*
#define FINDMINMAX(x0,x1,x2,min,max) \

min = max = x0;   \

if (x1<min) min = x1; \

if (x1>max) max = x1; \

if (x2<min) min = x2; \

if (x2>max) max = x2;*/


template<typename T>
T FindMin(T x0, T x1, T x2)
{
	return Min(Min(x0, x1), x2);
}



template<typename T>
T FindMax(T x0, T x1, T x2)
{
	return Max(Max(x0, x1), x2);
}



bool PlaneBoxOverlap(Vector3f normal, Vector3f vert, Vector3f maxbox)
{
	Vector3f vmin, vmax;

	for (uint i = 0; i < 3; ++i)
	{
		if (normal[i] > 0.0f)
		{
			vmin[i] = -maxbox[i] - vert[i];
			vmax[i] = maxbox[i] - vert[i];
		}
		else
		{
			vmin[i] = maxbox[i] - vert[i];
			vmax[i] = -maxbox[i] - vert[i];
		}
	}

	if (Dot(normal, vmin) > 0.0f)
		return false;

	if (Dot(normal, vmax) >= 0.0f)
		return true;

	return false;
}



#if 0

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)			   \

p0 = a*v0[Y] - b*v0[Z];			       	   \

p2 = a*v2[Y] - b*v2[Z];			       	   \

if (p0 < p2) { min = p0; max = p2; }
else { min = p2; max = p0; } \

rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_X2(a, b, fa, fb)			   \

p0 = a*v0[Y] - b*v0[Z];			           \

p1 = a*v1[Y] - b*v1[Z];			       	   \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)			   \

p0 = -a*v0[X] + b*v0[Z];		      	   \

p2 = -a*v2[X] + b*v2[Z];	       	       	   \

if (p0 < p2) { min = p0; max = p2; }
else { min = p2; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_Y1(a, b, fa, fb)			   \

p0 = -a*v0[X] + b*v0[Z];		      	   \

p1 = -a*v1[X] + b*v1[Z];	     	       	   \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



/*======================== Z-tests ========================*/



#define AXISTEST_Z12(a, b, fa, fb)			   \

p1 = a*v1[X] - b*v1[Y];			           \

p2 = a*v2[X] - b*v2[Y];			       	   \

if (p2 < p1) { min = p2; max = p1; }
else { min = p1; max = p2; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_Z0(a, b, fa, fb)			   \

p0 = a*v0[X] - b*v0[Y];				   \

p1 = a*v1[X] - b*v1[Y];			           \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \

if (min > rad || max < -rad) return 0;

#endif


template<uint axis0, uint axis1>
bool AxisTest(Vector3f v0, Vector3f v1)
{
	const float p0 = a*v0[axis0] - b*v0[axis1];
	const float p1 = a*v1[axis0] - b*v1[axis1];

	if (p0 < p1)
	{
		min = p0;
		max = p1;
	}
	else
	{
		min = p1;
		max = p0;
	}

	const float rad = fa * boxhalfsize[axis0] + fb * boxhalfsize[axis1];

	return min <= rad && max >= -rad;
}



#if 0

int triBoxOverlap(float boxcenter[3], float boxhalfsize[3], float triverts[3][3])

{



	/*    use separating axis theorem to test overlap between triangle and box */

	/*    need to test for overlap in these directions: */

	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */

	/*       we do not even need to test these) */

	/*    2) normal of the triangle */

	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */

	/*       this gives 3x3=9 more tests */

	float v0[3], v1[3], v2[3];

	//   float axis[3];

	float min, max, p0, p1, p2, rad, fex, fey, fez;		// -NJMP- "d" local variable removed

	float normal[3], e0[3], e1[3], e2[3];



	/* This is the fastest branch on Sun */

	/* move everything so that the boxcenter is in (0,0,0) */

	SUB(v0, triverts[0], boxcenter);

	SUB(v1, triverts[1], boxcenter);

	SUB(v2, triverts[2], boxcenter);



	/* compute triangle edges */

	SUB(e0, v1, v0);      /* tri edge 0 */

	SUB(e1, v2, v1);      /* tri edge 1 */

	SUB(e2, v0, v2);      /* tri edge 2 */



						  /* Bullet 3:  */

						  /*  test the 9 tests first (this was faster) */

	fex = fabsf(e0[X]);

	fey = fabsf(e0[Y]);

	fez = fabsf(e0[Z]);

	AXISTEST_X01(e0[Z], e0[Y], fez, fey);

	AXISTEST_Y02(e0[Z], e0[X], fez, fex);

	AXISTEST_Z12(e0[Y], e0[X], fey, fex);



	fex = fabsf(e1[X]);

	fey = fabsf(e1[Y]);

	fez = fabsf(e1[Z]);

	AXISTEST_X01(e1[Z], e1[Y], fez, fey);

	AXISTEST_Y02(e1[Z], e1[X], fez, fex);

	AXISTEST_Z0(e1[Y], e1[X], fey, fex);



	fex = fabsf(e2[X]);

	fey = fabsf(e2[Y]);

	fez = fabsf(e2[Z]);

	AXISTEST_X2(e2[Z], e2[Y], fez, fey);

	AXISTEST_Y1(e2[Z], e2[X], fez, fex);

	AXISTEST_Z12(e2[Y], e2[X], fey, fex);



	/* Bullet 1: */

	/*  first test overlap in the {x,y,z}-directions */

	/*  find min, max of the triangle each direction, and test for overlap in */

	/*  that direction -- this is equivalent to testing a minimal AABB around */

	/*  the triangle against the AABB */



	/* test in X-direction */

	FINDMINMAX(v0[X], v1[X], v2[X], min, max);

	if (min > boxhalfsize[X] || max < -boxhalfsize[X]) return 0;



	/* test in Y-direction */

	FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);

	if (min > boxhalfsize[Y] || max < -boxhalfsize[Y]) return 0;



	/* test in Z-direction */

	FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);

	if (min > boxhalfsize[Z] || max < -boxhalfsize[Z]) return 0;



	/* Bullet 2: */

	/*  test if the box intersects the plane of the triangle */

	/*  compute plane equation of triangle: normal*x+d=0 */

	CROSS(normal, e0, e1);

	// -NJMP- (line removed here)

	if (!planeBoxOverlap(normal, v0, boxhalfsize)) return 0;	// -NJMP-



	return 1;   /* box and triangle overlaps */

}

#endif

#endif

struct Triangle
{
	Vector3f v0, v1, v2;
	Vector3f operator[] (uint idx) const { return static_cast<const Vector3f*>(&v0)[idx]; }
	Vector3f& operator[] (uint idx) { return static_cast<Vector3f*>(&v0)[idx]; }
};

bool BoxTriangleIntersect(const Boxf& box, const Triangle& triangle)
{
	const Planef planes[6] =
	{
		Planef(Vector3f(1.0f, 0.0f, 0.0f),  box.min.x),
		Planef(Vector3f(-1.0f, 0.0f, 0.0f), -box.max.x),
		Planef(Vector3f(0.0f, 1.0f, 0.0f),  box.min.y),
		Planef(Vector3f(0.0f,-1.0f, 0.0f), -box.max.y),
		Planef(Vector3f(0.0f, 0.0f, 1.0f),  box.min.z),
		Planef(Vector3f(0.0f, 0.0f,-1.0f), -box.max.z),
	};
	uint flags[3] = {};
	for (uint i = 0; i < 6; ++i)
	{
		const Planef plane = planes[i];
		uint planeFlags = 1 << i;
		for (uint j = 0; j < 3; ++j)
		{
			const Vector3f vertex = triangle[j];
			if (Distance(plane, vertex) >= 0.0f)
				flags[j] |= planeFlags;
		}
	}
	return (flags[0] | flags[1] | flags[2]) == 0x3f;
}

//////////////////////////////////////////////////////////////////////////

void DrawModelGeometry(const Module& model, uint geomIdx)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	glBegin(GL_LINES);
	for (uint polyIdx = geometry.firstPolygon; polyIdx != geometry.lastPolygon; ++polyIdx)
	{
		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		const Vector3f first = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		Vector3f last = first;
		for (uint vertIdx = 1; vertIdx < numVertices; ++vertIdx)
		{
			const Vector3f cur = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			glVertex3fv(&last.x);
			glVertex3fv(&cur.x);
			last = cur;
		}
		glVertex3fv(&last.x);
		glVertex3fv(&first.x);
	}
	glEnd();
}

void DrawModel(const Module& model)
{
	glColor4ub(0, 0, 0, 255);
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		DrawModelGeometry(model, geomIdx);
}

struct Fragment
{
	Vector3i position;
};

struct RasterData
{
	std::vector<Fragment> fragments;
	float voxelSize;
};

void RasterizeTriangle(RasterData* raster, Triangle triangle)
{
	const float voxelSize = raster->voxelSize;
	const float invVoxelSize = 1.0f / voxelSize;
	const Vector3f voxelSize3 = Vector3f(voxelSize, voxelSize, voxelSize);

	Boxf box;
	box.min = Min(Min(triangle.v0, triangle.v1), triangle.v2);
	box.max = Max(Max(triangle.v0, triangle.v1), triangle.v2);
	box.min.x = std::floor(box.min.x);
	box.min.y = std::floor(box.min.y);
	box.min.z = std::floor(box.min.z);

	for (float z = box.min.z; z < box.max.z; z += voxelSize)
	{
		for (float y = box.min.y; y < box.max.y; y += voxelSize)
		{
			for (float x = box.min.x; x < box.max.x; x += voxelSize)
			{
				Boxf voxel;
				voxel.min = Vector3f(x, y, z);
				voxel.max = voxel.min + voxelSize3;
				if (!BoxTriangleIntersect(voxel, triangle))
					continue;
				Fragment fragment;
				fragment.position = Vector3i(voxel.min * invVoxelSize);
				raster->fragments.push_back(fragment);
			}
		}
	}
}

void RasterizeModelGeometry(RasterData* raster, const Module& model, uint geomIdx)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	for (uint polyIdx = geometry.firstPolygon; polyIdx != geometry.lastPolygon; ++polyIdx)
	{
		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		Triangle triangle;
		triangle.v0 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		triangle.v1 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 1));

		for (uint vertIdx = 2; vertIdx < numVertices; ++vertIdx)
		{
			triangle.v2 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			RasterizeTriangle(raster, triangle);
			triangle.v1 = triangle.v2;
		}
	}
}

void RasterizeModel(RasterData* raster, const Module& model)
{
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		RasterizeModelGeometry(raster, model, geomIdx);
}

//////////////////////////////////////////////////////////////////////////

void DrawFragments(const RasterData& raster)
{
	const float voxelSz = raster.voxelSize;
	const Vector3f voxelSizeHalf = Vector3f(voxelSz, voxelSz, voxelSz) * 0.5f;

	/*/
	glColor4ub(255, 128, 64, 255);
	glBegin(GL_LINES);
	for (const Fragment frag : raster.fragments)
	{
	Vector3f vert = Vector3f(frag.position) * voxelSz + voxelSizeHalf;

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );
	}
	glEnd();
	/**/

	glBegin(GL_QUADS);
	for (const Fragment frag : raster.fragments)
	{
		Vector3f vert = Vector3f(frag.position) * voxelSz;

		glColor4ub(255, 128, 0, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);

		glColor4ub(0, 128, 255, 255);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);

		glColor4ub(128, 255, 0, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);

		glColor4ub(128, 0, 255, 255);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);

		glColor4ub(0, 255, 128, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);

		glColor4ub(255, 0, 128, 255);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
	}
	glEnd();
}

//////////////////////////////////////////////////////////////////////////

void TestGM()
{
#if 1

	std::cout << "Loading Model . . . ";
	// auto model = LoadObjModuleFromFile("sponza/sponza.obj");
	auto model = LoadObjModuleFromFile("leather_chair/leather_chair.obj");
	std::cout << "DONE" << std::endl;

	std::cout << "Voxelizing . . . ";
	RasterData raster;
	// raster.voxelSize = 20.0f;
	raster.voxelSize = 2.0f;
	// raster.voxelSize = 0.5f;
	RasterizeModel(&raster, *model);
	std::cout << "DONE" << std::endl;

	Vector3f center = Vector3f(raster.fragments[0].position);
	float dist = 0.0f;
	for (auto fragment : raster.fragments)
		center = center + Vector3f(fragment.position);
	center = center * (1.0f / raster.fragments.size()) * raster.voxelSize;
	for (auto fragment : raster.fragments)
		dist = Max(dist, Distance(center, Vector3f(fragment.position) * raster.voxelSize));

#endif

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc());

#if 0
	DebugDraw debugDraw;
	gDebugDraw = &debugDraw;

	ScriptReload scriptReload("scripts/main.gm");
	FileChangeNotificationSystem fileChangeNotificationSystem;
	fileChangeNotificationSystem.AddFileNotification(L"scripts\\main.gm", &scriptReload);
	fileChangeNotificationSystem.StartNotificationListener();
	scriptReload.ReloadScript();
	protoGL.GetCanvas()->AddListener(new InputListener(scriptReload));

	TimeCounter counter;
	while (protoGL.Update())
	{
		fileChangeNotificationSystem.DispatchNotifications();

		Time deltaTime = counter.GetElapsed();
		frameTime = deltaTime.AsFloat();
		scriptReload.GetMachine()->Execute(deltaTime.AsMiliseconds());
		bool first = true;
		while (const char* log = scriptReload.GetMachine()->GetLog().GetEntry(first))
			std::cout << log;
		scriptReload.GetMachine()->GetLog().Reset();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		debugDraw.Flush();

		protoGL.Swap();
	}
#endif

#if 1
	OrbitalCamera camera(protoGL.GetCanvas());
	camera.SetPerspective(1.4f, 1.0f, dist * 10.0f);
	camera.SetCamera(center, Quaternionf(zero), dist * 2.0f);

	glEnable(GL_DEPTH_TEST);

	TimeCounter counter;
	while (protoGL.Update())
	{
		Time deltaTime = counter.GetElapsed();

		camera.Update(deltaTime);
		camera.CommitToGL();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DrawModel(*model);
		DrawFragments(raster);

		protoGL.Swap();
	}
#endif
}

#endif
