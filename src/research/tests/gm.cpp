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
	SetCurrentDirectory("../../");

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
}
