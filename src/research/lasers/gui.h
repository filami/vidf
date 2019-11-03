#include "game.h"



namespace lasers
{

	
	
using namespace vidf;



enum class MouseButton
{
	Left,
	Right,
	Middle
};



class Widget;
typedef std::shared_ptr<Widget> WidgetPtr;
typedef std::weak_ptr<Widget> WidgetRef;

class Widget : public std::enable_shared_from_this<Widget>
{
public:
	void Clear();
	void SetOffset(Vector2f offset);
	void SetSize(Vector2f size);
	Vector2f GetSize() const;
	Rectf GetRect() const { return rect; }
	Rectf GetWorldRect() const;
	void AddChild(WidgetPtr widget);
	void Update(Vector2f mouse);
	void Draw(Painter& painter);
	bool ButtonDown(Vector2f mouse, MouseButton button);
	bool ButtonUp(Vector2f mouse, MouseButton button);

protected:
	virtual void OnPaint(Painter& painter) {}
	virtual void OnMouseEnter() { }
	virtual void OnMouseLeave() { }
	virtual bool OnButtonDown(MouseButton button) { return false; }
	virtual bool OnButtonUp(MouseButton button) { return false; }

private:
	std::vector<WidgetPtr> children;
	WidgetRef parent;
	Rectf     rect{ Vector2f(zero), Vector2f(zero) };
	bool      wasMouseInside = false;
};



class Button : public Widget
{
public:
	enum State
	{
		Released,
		Hover,
		Pressed,
	};

	typedef std::function<void()> ClickedEvent;

public:
	Button(const char* _label) : label(_label) {}

	void SetOnClicked(ClickedEvent event) { onClicked = event; }

private:
	void OnPaint(Painter& painter) override;

	void OnMouseEnter() override;
	void OnMouseLeave() override;
	bool OnButtonDown(MouseButton button) override;
	bool OnButtonUp(MouseButton button) override;

private:
	string label;
	State  state = Released;
	ClickedEvent onClicked{ []() {} };
};



}
