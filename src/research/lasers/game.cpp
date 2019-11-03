#include "pch.h"
#include "game.h"

namespace lasers
{



void glVertex(Vector2f v)
{
	glVertex2f(v.x, v.y);
}



void glColor(Color color)
{
	glColor4f(color.r, color.g, color.b, color.a);
}



void glBrush(const Brush& brush)
{
	glColor(brush.color);
}




void glPen(const Pen& pen)
{
	glLineWidth(pen.width);
	glColor(pen.color);
}



void Painter::DrawLine(Vector2f start, Vector2f end)
{
	assert(!pen.transparent);
	if (!pen.transparent)
	{
		glPen(pen);
		glBegin(GL_LINE_LOOP);
		glVertex2f(start.x, start.y);
		glVertex2f(end.x, end.y);
		glEnd();
	}
}



void Painter::DrawBox(Rectf rect)
{
	auto PushVertices = [=]()
	{
		glVertex2f(rect.min.x, rect.min.y);
		glVertex2f(rect.max.x, rect.min.y);
		glVertex2f(rect.max.x, rect.max.y);
		glVertex2f(rect.min.x, rect.max.y);
	};
	if (!brush.transparent)
	{
		glBrush(brush);
		glBegin(GL_QUADS);
		PushVertices();
		glEnd();
	}
	if (!pen.transparent)
	{
		glPen(pen);
		glBegin(GL_LINE_LOOP);
		PushVertices();
		glEnd();
	}
}



void Painter::DrawCircle(Vector2f center, float radius)
{
	auto PushVertices = [=]()
	{
		const uint numSegment = 24;
		for (uint i = 0; i < numSegment; ++i)
		{
			const float a = i / float(numSegment) * 2.0f * PI;
			glVertex2f(
				std::cos(a) * radius + center.x,
				std::sin(a) * radius + center.y);
		}
	};
	if (!brush.transparent)
	{
		glBrush(brush);
		glBegin(GL_POLYGON);
		PushVertices();
		glEnd();
	}
	if (!pen.transparent)
	{
		glPen(pen);
		glBegin(GL_LINE_LOOP);
		PushVertices();
		glEnd();
	}
}



void Painter::DrawText(Vector2f position, HAlign halign, VAlign valign, const char* label)
{
	glBrush(brush);
	text.flip = true;
	text.size = 0.05f;
	Vector2f size = text.CalculateSize(label);
	text.OutputText(position - size * 0.5f, label);
}



}
