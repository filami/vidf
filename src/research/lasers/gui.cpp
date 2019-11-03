#include "pch.h"
#include "gui.h"



namespace lasers
{



void Widget::Clear()
{
	children.clear();
}



void Widget::SetOffset(Vector2f offset)
{
	Vector2f sz = GetSize();
	rect.min = offset;
	rect.max = offset + sz;
}



void Widget::SetSize(Vector2f size)
{
	rect.max = rect.min + size;
}



Vector2f Widget::GetSize() const
{
	return rect.max - rect.min;
}



void Widget::AddChild(WidgetPtr widget)
{
	assert(!widget->parent.lock());
	widget->parent = shared_from_this();
	children.push_back(widget);
}



void Widget::Update(Vector2f mouse)
{
	bool isInside = IsPointInside(GetWorldRect(), mouse);
	if (isInside && !wasMouseInside)
		OnMouseEnter();
	if (!isInside && wasMouseInside)
		OnMouseLeave();
	for (WidgetPtr child : children)
		child->Update(mouse);
	wasMouseInside = isInside;
}



void Widget::Draw(Painter& painter)
{
	OnPaint(painter);
	for (WidgetPtr child : children)
		child->Draw(painter);
}



bool Widget::ButtonDown(Vector2f mouse, MouseButton button)
{
	if (!IsPointInside(GetWorldRect(), mouse))
		return false;
	if (OnButtonDown(button))
		return true;
	for (WidgetPtr child : children)
	{
		if (child->ButtonDown(mouse, button))
			return true;
	}
	return false;
}



bool Widget::ButtonUp(Vector2f mouse, MouseButton button)
{
	if (!IsPointInside(GetWorldRect(), mouse))
		return false;
	if (OnButtonUp(button))
		return true;
	for (WidgetPtr child : children)
	{
		if (child->ButtonUp(mouse, button))
			return true;
	}
	return false;
}



Rectf Widget::GetWorldRect() const
{
	auto parentLc = parent.lock();
	const Rectf localRect = GetRect();
	if (!parentLc)
		return localRect;
	const Vector2f offset = parentLc->GetWorldRect().min;
	return Rectf(
		offset + localRect.min,
		offset + localRect.max);
}



void Button::OnPaint(Painter& painter)
{
	const Rectf rect = GetWorldRect();

	switch (state)
	{
	case Released:
		painter.SetPen(Pen(Color(1.0f, 0.5f, 0.0f), 3.0f));
		painter.SetBrush(Brush());
		break;
	case Hover:
		painter.SetPen(Pen(Color(1.0f, 0.5f, 0.0f), 5.0f));
		painter.SetBrush(Brush());
		break;
	case Pressed:
		painter.SetPen(Pen(Color(1.0f, 1.0f, 0.0f), 5.0f));
		painter.SetBrush(Brush(Color(0.25f, 0.175f, 0.0f)));
		break;
	}
	painter.DrawBox(rect);

	painter.SetBrush(Brush(Color(1.0f, 0.5f, 0.0f)));
	painter.DrawText(
		(rect.min + rect.max) * 0.5f,
		HAlign::Center, VAlign::Center,
		label.c_str());
}



void Button::OnMouseEnter()
{
	state = Hover;
}



void Button::OnMouseLeave()
{
	state = Released;
}



bool Button::OnButtonDown(MouseButton button)
{
	if (button == MouseButton::Left)
	{
		state = Pressed;
		return true;
	}
	return false;
}



bool Button::OnButtonUp(MouseButton button)
{
	if (button == MouseButton::Left)
	{
		state = Hover;
		onClicked();
		return true;
	}
	return false;
}



}
