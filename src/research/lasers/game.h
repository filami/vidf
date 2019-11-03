#pragma once

#include "vidf/proto/text.h"


namespace lasers
{



using namespace vidf;
using namespace proto;



struct Brush
{
	Brush() = default;
	Brush(Color _color)
		: color(_color)
		, transparent(false) {}
	Color color{zero};
	bool  transparent = true;
};



struct Pen
{
	Pen() = default;
	Pen(Color _color, float _width)
		: color(_color)
		, width(_width)
		, transparent(false) {}
	Color color{ zero };
	float width = 0.0f;
	bool  transparent = true;
};



enum class HAlign
{
	Left,
	Center,
	Right,
};



enum class VAlign
{
	Top,
	Center,
	Bottom,
};



class Painter
{
public:
	Painter(ProtoGL& _graphics, Text& _text)
		: graphics(_graphics)
		, text(_text) {}

	void SetBrush(Brush _brush)
	{
		brush = _brush;
	}

	void SetPen(Pen _pen)
	{
		pen = _pen;
	}

	void DrawLine(Vector2f start, Vector2f end);
	void DrawBox(Rectf rect);
	void DrawCircle(Vector2f center, float radius);
	void DrawText(Vector2f position, HAlign halign, VAlign valign, const char* label);

private:
	ProtoGL& graphics;
	Text&    text;
	Brush    brush;
	Pen      pen;
};



}
