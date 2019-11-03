#include "pch.h"
#include "vidf/common/random.h"

#include "yasli/Enum.h"
#include "yasli/STL.h"
#include "yasli/decorators/Range.h"
#include "yasli/JSONIArchive.h"
#include "yasli/JSONOArchive.h"
#include "yasli/Pointers.h"

#include "gamestate.h"
#include "gui.h"


namespace yasli
{


template<class T>
class StdSharedPtrSerializer : public PointerInterface
{
public:
	StdSharedPtrSerializer(std::shared_ptr<T>& ptr)
	: ptr_(ptr)
	{}

	const char* registeredTypeName() const{
		if(ptr_)
			return ClassFactory<T>::the().getRegisteredTypeName(ptr_.get());
		else
			return "";
	}
	void create(const char* typeName) const{
		YASLI_ASSERT(!ptr_ || ptr_.unique());
		if(typeName && typeName[0] != '\0')
			ptr_.reset(factory()->create(typeName));
		else
			ptr_.reset((T*)0);
	}
	TypeID baseType() const{ return TypeID::get<T>(); }
	virtual Serializer serializer() const{
		return Serializer(*ptr_);
	}
	void* get() const{
		return reinterpret_cast<void*>(ptr_.get());
	}
	const void* handle() const{
		return &ptr_;
	}
	TypeID pointerType() const{ return TypeID::get<SharedPtr<T> >(); }
	virtual ClassFactory<T>* factory() const{ return &ClassFactory<T>::the(); }
protected:
	std::shared_ptr<T>& ptr_;
};


template<class T>
bool serialize(yasli::Archive& ar, std::shared_ptr<T>& ptr, const char* name, const char* label)
{
	yasli::StdSharedPtrSerializer<T> serializer(ptr);
	return ar(static_cast<yasli::PointerInterface&>(serializer), name, label);
}



template<typename T>
bool serialize(yasli::Archive& ar, vidf::Vector2<T>& vector, const char* name, const char* label)
{
	ar.openBlock(name, label);
	ar(vector.x, "x", "x");
	ar(vector.y, "y", "y");
	ar.closeBlock();
	return true;
}

}



namespace
{


using namespace vidf;
using namespace proto;
using namespace lasers;



void glVertex(Vector2f v)
{
	glVertex2f(v.x, v.y);
}



void glVertex(Vector2i v)
{
	glVertex2f(v.x, v.y);
}



void glColor(Color color)
{
	glColor4f(color.r, color.g, color.b, color.a);
}



const float epsilon = 1.0f / 1024.0f;
#define MachineEpsilon (std::numeric_limits<float>::epsilon() * 0.5)



inline float gamma(int n)
{
	return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
}


bool IntersectBox(const Vector2f rayOrigin, const Vector2f rayDirection, const Rectf& bounds, float* hitt0, float* hitt1)
{
	float t0 = 0, t1 = std::numeric_limits<float>::max();
	for (int i = 0; i < 2; ++i)
	{
		// Update interval for _i_th bounding box slab
		float invRayDir = 1 / rayDirection[i];
		float tNear = (bounds.min[i] - rayOrigin[i]) * invRayDir;
		float tFar = (bounds.max[i] - rayOrigin[i]) * invRayDir;

		// Update parametric interval from slab intersection $t$ values
		if (tNear > tFar)
			std::swap(tNear, tFar);

		// Update _tFar_ to ensure robust ray--bounds intersection
		tFar *= 1 + 2 * gamma(3);
		t0 = tNear > t0 ? tNear : t0;
		t1 = tFar < t1 ? tFar : t1;
		if (t0 > t1)
			return false;
	}
	*hitt0 = t0;
	*hitt1 = t1;
	return true;
}



Vector2f ClosestPointToRay(Vector2f rayOrigin, Vector2f rayDirection, Vector2f point)
{
	return rayOrigin + rayDirection * Dot(point - rayOrigin, rayDirection);
}



Vector2i Vector2fToVector2i(Vector2f vec)
{
	float x;
	float y;
	if (std::abs(vec.x) < epsilon)
	{
		y = Sign(vec.y);
		x = 0.0f;
	}
	else if (std::abs(vec.y) < epsilon)
	{
		x = Sign(vec.x);
		y = 0.0f;
	}
	else
	{
		x = Min(1.0f / std::abs(vec.y), 2.0f) * Sign(vec.x);
		y = Min(1.0f / std::abs(vec.x), 2.0f) * Sign(vec.y);
	}
	Vector2i result = Vector2i(uint(x), uint(y));
	return result;
}



Vector2i AngleToVector2i(float angle)
{
	const float rad = Degrees2Radians(angle);
	const float s = std::sin(rad);
	const float c = std::cos(rad);
	return Vector2fToVector2i(Vector2f(c, s));
}



Vector2f AngleToVector2f(float angle)
{
	const float rad = Degrees2Radians(angle);
	const float s = std::sin(rad);
	const float c = std::cos(rad);
	const Vector2f result = Vector2f(c, s);
	return result;
}



Vector2f RotateVector2f(Vector2f asComplex, Vector2f vector)
{
	return Vector2f(
		asComplex.x * vector.x - asComplex.y * vector.y,
		asComplex.x * vector.y + asComplex.y * vector.x);
}



Vector2f Reflect(Vector2f vector, Vector2f normal)
{
	const Vector2f vec = Vector2f(vector);
	const Vector2f output = Normalize(vec - 2.0f * normal * Dot(vec, normal));
	return output;
}



/////////////////////////////////////////////



enum class LaserColor : uint8
{
	Red   = 1,
	Green = 2,
	Blue  = 4,
};



struct LaserColorMask
{
	LaserColorMask() = default;
	explicit LaserColorMask(uint _mask)
		: mask(_mask) {}
	explicit LaserColorMask(LaserColor color)
		: mask(uint8(color)) {}

	LaserColorMask operator=(uint8 _mask)
	{
		mask = _mask;
		return *this;
	}

	bool operator==(LaserColorMask _mask) const
	{
		return mask == _mask.mask;
	}

	bool HasColor(LaserColor color) const { return (mask & uint8(color)) != 0; }
	void Set(LaserColor color, bool set = true)
	{
		if (set)
			mask |= uint8(color);
		else
			mask &= ~uint8(color);
	};

	uint8 mask = 0;
};

const LaserColorMask laserWhite{ uint8(LaserColor::Red) | uint8(LaserColor::Green) | uint8(LaserColor::Blue) };




struct Laser
{
	Vector2f start;
	Vector2f end;
	Vector2f direction;
	LaserColor color = LaserColor::Green;
	bool       gameplay = true;
};


typedef std::deque<Laser> PendingSegments;



Color GetLaserColor(LaserColor laserColor)
{
	switch (laserColor)
	{
	case LaserColor::Red:   return Color(0.75f, 0.15f, 0.0f);
	case LaserColor::Green: return Color(0.1f, 0.65f, 0.15f);
	case LaserColor::Blue:  return Color(0.1f, 0.1f, 0.85f);
	}
}


Color GetLaserColor(LaserColorMask mask)
{
	Color color{zero};
	if (mask.HasColor(LaserColor::Red))
		color = color + GetLaserColor(LaserColor::Red);
	if (mask.HasColor(LaserColor::Green))
		color = color + GetLaserColor(LaserColor::Green);
	if (mask.HasColor(LaserColor::Blue))
		color = color + GetLaserColor(LaserColor::Blue);
	return color;
}



enum class InputState
{
	Pressed,
	Hold,
	Released,
};



class Input
{
public:
	Input(int _key) : key(_key) {}

	bool IsPressed() const   { return isPressed; }
	bool IsReleased() const  { return !isPressed; }
	bool WasPressed() const  { return changed && isPressed; }
	bool WasReleased() const { return changed && !isPressed; }

	void Update()
	{
		bool newState = GetAsyncKeyState(key) & 0x8000;
		changed = newState != isPressed;
		isPressed = newState;
	}

private:
	int  key;
	bool isPressed = false;
	bool changed = false;
};



struct InputMap
{
	Input leftKey  = Input(VK_LBUTTON);
	Input rightKey = Input(VK_RBUTTON);

	void Update()
	{
		leftKey.Update();
		rightKey.Update();
	}
};



class Element;
typedef std::shared_ptr<Element> ElementPtr;
typedef std::weak_ptr<Element> ElementRef;

class Element : public enable_shared_from_this<Element>
{
public:
	static const uint maxOutputs = 3;
	typedef std::array<Laser, maxOutputs> Lasers;

	enum InputKey
	{
		LeftButton,
		RightButton,
	};

	enum InputType
	{
		Pressed,
		Hold,
		Released,
	};

public:
	Element() = default;
	Element(Vector2i _position)
		: position(_position) {}
	virtual ~Element();

	virtual void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) { }
	virtual void EmitLaser(PendingSegments* outputSegments) { }
	virtual void Update(Time deltaTime) {}
	virtual void PostUpdate(Time deltaTime) {}
	virtual void Draw() {}
	virtual void Input(const InputMap& inputMap) {}
	virtual bool BlocksLaser() const { return true; }
	virtual void serialize(yasli::Archive& ar);
	virtual void SetActive(bool active) {}
	virtual bool IsMapSolved() const { return false; }

	virtual void MakeEditorGui(WidgetPtr parent) {}

	void AddTarget(ElementRef element);
	void RemoveTarget(ElementRef element);
	void SetTargetsActive(bool active) const;
	void ClearTargets();
	const std::vector<ElementRef>& GetTargets() const { return targets; }

	Vector2i GetPosition() const { return position; }

protected:

private:
	friend class Map;
	Vector2i position = Vector2i(zero);
	vector<ElementRef> targets;
	vector<ElementRef> sources;
};



class Map
{
public:
	Map() = default;
	Map(uint _width, uint _height);

	uint GetWidth() const { return width; }
	uint GetHeigth() const { return height; }
	bool IsSolved() const { return solved; }
	ElementPtr GetElement(Vector2i coord);
	void SetElementPosition(ElementPtr element, Vector2i coord);

	void AddElement(ElementPtr element);
	void RemoveElement(ElementPtr element);
	void Update(Time deltaTime, Vector2f cursorPosition, bool gameplayActive);
	void Draw(float fade, bool drawTargetConnectors);

	void serialize(yasli::Archive& ar);

private:
	Vector2f Raytrace(ElementPtr* outIntersectedElement, Vector2f rayOrigin, Vector2f rayDirection) const;
	uint CoordToIndex(Vector2i coord) const;
	bool IsValidCoord(Vector2i coord) const;

private:
	std::vector<ElementPtr> elements;
	std::vector<ElementPtr> elementMap;
	std::vector<Laser> laserSegments;
	PendingSegments    pendingSegments;
	InputMap inputMap;
	Time time;
	uint width = 0;
	uint height = 0;
	bool solved = false;
};



/////////////////////////////////////////////



Element::~Element()
{
	while (!sources.empty())
		sources.back().lock()->RemoveTarget(weak_from_this());
}



void Element::serialize(yasli::Archive& ar)
{
	ar(position, "position");
}



vector<ElementRef>::iterator FindElement(vector<ElementRef>& elementRefs, ElementRef toFindRef)
{
	ElementPtr toFind = toFindRef.lock();
	auto it = find_if(
		elementRefs.begin(), elementRefs.end(),
		[toFind](ElementRef elementRef) { return elementRef.lock() == toFind; });
	return it;
}



void Element::AddTarget(ElementRef elementRef)
{
	if (FindElement(targets, elementRef) == targets.end())
		targets.push_back(elementRef);
	ElementPtr element = elementRef.lock();
	ElementRef thisRef = weak_from_this();
	if (FindElement(element->sources, thisRef) == element->sources.end())
		element->sources.push_back(thisRef);
}



void Element::SetTargetsActive(bool active) const
{
	for (auto& target : targets)
		target.lock()->SetActive(active);
}



void Element::RemoveTarget(ElementRef elementRef)
{
	auto it = FindElement(targets, elementRef);
	if (it == targets.end())
		return;
	ElementPtr element = elementRef.lock();
	ElementRef thisRef = weak_from_this();
	auto it2 = FindElement(element->sources, thisRef);
	if (it2 != element->sources.end())
		element->sources.erase(it2);
	targets.erase(it);
}



void Element::ClearTargets()
{
	while (!targets.empty())
		RemoveTarget(targets.back());
}



/////////////////////////////////////////////



Map::Map(uint _width, uint _height)
	: width(_width)
	, height(_height)
{
	elementMap.resize(width * height);
}



ElementPtr Map::GetElement(Vector2i coord)
{
	return elementMap[CoordToIndex(coord)];
}



void Map::SetElementPosition(ElementPtr element, Vector2i coord)
{
	assert(GetElement(coord) == nullptr);
	elementMap[CoordToIndex(element->position)].reset();
	element->position = coord;
	elementMap[CoordToIndex(coord)] = element;
}



void Map::AddElement(ElementPtr element)
{
	elements.push_back(element);
	elementMap[CoordToIndex(element->GetPosition())] = element;
}



void Map::RemoveElement(ElementPtr element)
{
	elementMap[CoordToIndex(element->GetPosition())].reset();
	elements.erase(
		std::remove(elements.begin(), elements.end(), element),
		elements.end());
}



void Map::Update(Time deltaTime, Vector2f cursorPosition, bool gameplayActive)
{
	time += deltaTime;
	
	laserSegments.clear();
	
	inputMap.Update();
	if (!solved && gameplayActive)
	{
		Vector2i hiliteCoord = Vector2i(int(cursorPosition.x + 0.5f), int(cursorPosition.y + 0.5f));
		if (IsValidCoord(hiliteCoord))
		{
			ElementPtr hiliteElement = elementMap[CoordToIndex(hiliteCoord)];
			if (hiliteElement)
				hiliteElement->Input(inputMap);
		}
	}

	for (ElementPtr element : elements)
		element->Update(deltaTime);

	for (ElementPtr element : elements)
		element->EmitLaser(&pendingSegments);

	while (!pendingSegments.empty())
	{
		Laser laser = pendingSegments.front();
		pendingSegments.pop_front();
		ElementPtr element;
		laser.end = Raytrace(
			laser.gameplay ? &element : nullptr,
			laser.start, laser.direction);
		if (element)
			element->ProcessLaser(&pendingSegments, laser);
		laserSegments.push_back(laser);
	}

	for (ElementPtr element : elements)
	{
		element->PostUpdate(deltaTime);
		if (element->IsMapSolved())
			solved = true;
	}
}



Vector2f Map::Raytrace(ElementPtr* outIntersectedElement, Vector2f rayOrigin, Vector2f rayDirection) const
{
	bool intersect = false;
	float bestDist = std::numeric_limits<float>::max();
	ElementPtr bestElement;
	float hit0, hit1;
	Rectf bounds;

	for (ElementPtr element : elements)
	{
		const Vector2f position = Vector2f(element->GetPosition());
		bounds.min = position - Vector2f(0.5f, 0.5f);
		bounds.max = position + Vector2f(0.5f, 0.5f);
		if (IsPointInside(bounds, rayOrigin))
			continue;
		const bool blocksLaser = element->BlocksLaser();
		const bool intersected = IntersectBox(rayOrigin, rayDirection, bounds, &hit0, &hit1);
		if (!intersected || hit0 > bestDist)
			continue;

		const float distToElement = Distance(ClosestPointToRay(rayOrigin, rayDirection, position), position);

		if (outIntersectedElement && distToElement < 0.2f)
		{
			intersect = true;
			*outIntersectedElement = element;
			bestDist = Distance(rayOrigin, position);
		}
		else if (blocksLaser)
		{
			intersect = true;
			bestDist = hit0;
		}
	}

	if (!intersect)
	{
		bounds.min = Vector2f(-0.5f, -0.5f);
		bounds.max = Vector2f(width - 0.5f, height - 0.5f);
		IntersectBox(rayOrigin, rayDirection, bounds, &hit0, &hit1);
		bestDist = hit1;
	}

	return rayOrigin + rayDirection * bestDist;
}



void Map::serialize(yasli::Archive& ar)
{
	ar(width, "width");
	ar(height, "height");
	ar(elements, "elements");

	std::vector<std::pair<uint, uint>> connectors;
	if (ar.isOutput())
	{
		for (uint sourceIdx = 0; sourceIdx < elements.size(); ++sourceIdx)
		{
			ElementPtr source = elements[sourceIdx];
			for (uint j = 0; j < source->GetTargets().size(); ++j)
			{
				ElementPtr target = source->GetTargets()[j].lock();
				uint targetIdx = std::find(elements.begin(), elements.end(), target) - elements.begin();
				connectors.emplace_back(sourceIdx, targetIdx);
			}
		}
	}
	ar(connectors, "connectors");

	if (ar.isInput())
	{
		elementMap.resize(width * height);
		for (ElementPtr element : elements)
			elementMap[CoordToIndex(element->GetPosition())] = element;
		for (auto connector : connectors)
			elements[connector.first]->AddTarget(elements[connector.second]);
	}
}



void Map::Draw(float fade, bool drawTargetConnectors)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLineWidth(1);
	glColor4ub(128, 0, 0, 255);
	glBegin(GL_LINES);
	for (uint i = 0; i < width; ++i)
	{
		glVertex2f(i, -0.5f);
		glVertex2f(i, height - 0.5f);
	}
	for (uint i = 0; i < height; ++i)
	{
		glVertex2f(-0.5f, i);
		glVertex2f(width - 0.5f, i);
	}
	glEnd();

	glLineWidth(3);
	glColor4ub(192, 16, 0, 255);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.5f, -0.5f);
	glVertex2f(width - 0.5f, -0.5f);
	glVertex2f(width - 0.5f, height - 0.5f);
	glVertex2f(-0.5f, height - 0.5f);
	glEnd();

	for (ElementPtr element : elements)
	{
		glPushMatrix();
		glTranslatef(element->GetPosition().x, element->GetPosition().y, 0.0f);
		element->Draw();
		glPopMatrix();
	}

	glBlendFunc(GL_ONE, GL_ONE);
	
	glLineWidth(5);
	const Color black = Color(0.0f, 0.0f, 0.0f);
	glBegin(GL_QUADS);
	for (Laser laser : laserSegments)
	{
		const float width = 0.25f;
		const float segmentSize = 0.5f;
		const float totalDistance = Distance(Vector2f(laser.start), Vector2f(laser.end));
		const Vector2f direction = Normalize(Vector2f(laser.direction));
		const Vector2f right = Vector2f(direction.y, -direction.x) * width * 0.5f;
		Vector2f lastPoint = Vector2f(laser.start);

		for (float t = 0.0f; t < totalDistance; t += segmentSize)
		{
			Vector2f curPoint = Vector2f(laser.start) + direction * Min(t + segmentSize, totalDistance);

			const Color laserColor =
				GetLaserColor(laser.color) *
				(1.0f - (std::sin(time.AsFloat() * 2.0f * PI * 4.0f) * 0.5f + 0.5f) * 0.2f) * 
				(1.0f - (std::sin(t * 0.5f - time.AsFloat() * 30.0f) * 0.5f + 0.5f) * 0.5f);

			glColor(laserColor);
			glVertex(Vector2f(lastPoint));
			glVertex(Vector2f(curPoint));
			glColor(black);
			glVertex(Vector2f(curPoint) + right);
			glVertex(Vector2f(lastPoint) + right);

			glColor(laserColor);
			glVertex(Vector2f(lastPoint));
			glVertex(Vector2f(curPoint));
			glColor(black);
			glVertex(Vector2f(curPoint) - right);
			glVertex(Vector2f(lastPoint) - right);

			lastPoint = curPoint;
		}
	}
	glEnd();

	if (drawTargetConnectors)
	{
		glLineWidth(3);
		glBegin(GL_LINES);
		for (ElementPtr element : elements)
		{
			for (ElementRef targetRef : element->GetTargets())
			{
				ElementPtr target = targetRef.lock();
				glVertex(element->GetPosition());
				glVertex(target->GetPosition());
			}
		}
		glEnd();
	}

	if (fade < 1.0f)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f - fade);
		glBegin(GL_QUADS);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f(width, -1.0f);
		glVertex2f(width, height);
		glVertex2f(-1.0f, height);
		glEnd();
	}
}



uint Map::CoordToIndex(Vector2i coord) const
{
	assert(IsValidCoord(coord));
	return coord.x + coord.y * width;
}



bool Map::IsValidCoord(Vector2i coord) const
{
	return coord.x >= 0 && coord.y >= 0 && coord.x < width && coord.y < height;
}



/////////////////////////////////////////////


const float angleStep = 360.0f / 16.0f;


const Color baseLineColor = Color(1.0f, 0.5f, 0.0f);


void DrawBase(Color baseColor = Color{zero})
{
	glColor(baseColor);
	const float size = 0.5f;
	glBegin(GL_QUADS);
	glVertex2f(-size, -size);
	glVertex2f( size, -size);
	glVertex2f( size,  size);
	glVertex2f(-size,  size);
	glEnd();

	glLineWidth(3);
	glColor(baseLineColor);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-size, -size);
	glVertex2f(size, -size);
	glVertex2f(size, size);
	glVertex2f(-size, size);
	glEnd();
}



Vector2f RoundDirection(Vector2f direction)
{
	const float mult = Max(std::abs(direction.x), std::abs(direction.y));
	float x = uint(std::abs(direction.x) / mult * 2.0f + 0.5f) * 2.0f * Sign(direction.x);
	float y = uint(std::abs(direction.y) / mult * 2.0f + 0.5f) * 2.0f * Sign(direction.y);
	return Normalize(Vector2f(x, y));
}



class Emitter : public Element
{
public:
	Emitter() = default;
	Emitter(Vector2i position)
		: Element(position) {}

	void EmitLaser(PendingSegments* outputSegments) override
	{
		Laser laser;
		laser.start = Vector2f(GetPosition());
		laser.direction = AngleToVector2f(angle);
		if (colorMask.HasColor(LaserColor::Red))
		{
			laser.color = LaserColor::Red;
			outputSegments->push_back(laser);
		}
		if (colorMask.HasColor(LaserColor::Green))
		{
			laser.color = LaserColor::Green;
			outputSegments->push_back(laser);
		}
		if (colorMask.HasColor(LaserColor::Blue))
		{
			laser.color = LaserColor::Blue;
			outputSegments->push_back(laser);
		}
	}

	void Draw() override
	{
		DrawBase();

		const Vector2f rotation = AngleToVector2f(angle);
		glColor(GetLaserColor(colorMask));
		glBegin(GL_TRIANGLES);
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f,  0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f,  0.5f)));
		glEnd();
	}

	virtual void MakeEditorGui(WidgetPtr parent)
	{
		const Vector2f buttonSz = Vector2f(0.04f, 0.04f);
		std::shared_ptr<Button> addButton;

		addButton = std::make_shared<Button>("<");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(0.0f, 0.0f));
		addButton->SetOnClicked([this]() { angle += 90.0f; });
		parent->AddChild(addButton);
		
		addButton = std::make_shared<Button>(">");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(buttonSz.x, 0.0f));
		addButton->SetOnClicked([this]() { angle -= 90.0f; });
		parent->AddChild(addButton);
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angle, "angle");
		ar(colorMask, "mask");
	}

private:
	LaserColorMask colorMask = laserWhite;
	float angle = 0.0f;
};

YASLI_CLASS(Element, Emitter, "Emitter")



class Receiver : public Element
{
public:
	Receiver() = default;
	Receiver(Vector2i position)
		: Element(position) {}

	void Update(Time time) override
	{
		received = 0;
	}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		if (Dot(-AngleToVector2f(angle), inputSegment.direction) > 0.95f)
			received.Set(inputSegment.color);
	}

	void Draw() override
	{
		DrawBase(GetLaserColor(received));

		const Vector2f rotation = AngleToVector2f(angle);
		glColor(GetLaserColor(laserWhite));
		glBegin(GL_TRIANGLES);
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f,  0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f,  0.5f)));
		glEnd();
	}

	virtual void MakeEditorGui(WidgetPtr parent)
	{
		const Vector2f buttonSz = Vector2f(0.04f, 0.04f);
		std::shared_ptr<Button> addButton;

		addButton = std::make_shared<Button>("<");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(0.0f, 0.0f));
		addButton->SetOnClicked([this]() { angle += 90.0f; });
		parent->AddChild(addButton);

		addButton = std::make_shared<Button>(">");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(buttonSz.x, 0.0f));
		addButton->SetOnClicked([this]() { angle -= 90.0f; });
		parent->AddChild(addButton);
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angle, "angle");
	}

	bool IsMapSolved() const override { return received == laserWhite; }

private:
	float angle = 0.0f;
	LaserColorMask received{ 0 };
};

YASLI_CLASS(Element, Receiver, "Receiver")



class Mirror : public Element
{
public:
	Mirror() = default;
	Mirror(Vector2i position)
		: Element(position) {}

	void Update(Time time) override
	{
		moveFraction = Min(moveFraction + time.AsFloat() * moveSpeed, 1.0f);
	}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		Laser outputLaser;
		outputLaser.start = Vector2f(GetPosition());
		if (dichroicReflect.HasColor(inputSegment.color))
		{
			outputLaser.direction = LerpReflect(inputSegment.direction);
			outputLaser.gameplay = !IsRotating();
		}
		else
			outputLaser.direction = inputSegment.direction;
		outputLaser.color = inputSegment.color;
		outputSegments->push_back(outputLaser);
	}

	void Draw() override
	{
		glColor(baseLineColor);
		glPointSize(7.0f);
		glBegin(GL_POINTS);
		glVertex2f(0.0f, 0.0f);
		glEnd();

		const Vector2f rotation = AngleToVector2f(CurrentAngle());
		glLineWidth(3);
		glColor(GetLaserColor(dichroicReflect));
		glBegin(GL_LINES);
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, -0.55f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.55f)));
		glEnd();
	}

	void Input(const InputMap& inputMap) override
	{
		if (IsRotating())
			return;
		const float time = 0.15f;
		if (inputMap.leftKey.IsPressed())
			RotateTo(CurrentAngle() + angleStep * 0.5f, time);
		else if (inputMap.rightKey.IsPressed())
			RotateTo(CurrentAngle() - angleStep * 0.5f, time);
	}

	bool BlocksLaser() const override
	{
		return false;
	}

	virtual void MakeEditorGui(WidgetPtr parent)
	{
		const Vector2f buttonSz = Vector2f(0.04f, 0.04f);
		std::shared_ptr<Button> addButton;

		addButton = std::make_shared<Button>("<");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(0.0f, 0.0f));
		addButton->SetOnClicked([this]() { nextAngle += angleStep; });
		parent->AddChild(addButton);

		addButton = std::make_shared<Button>(">");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(buttonSz.x, 0.0f));
		addButton->SetOnClicked([this]() { nextAngle -= angleStep; });
		parent->AddChild(addButton);

		addButton = std::make_shared<Button>("");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(0.0f, buttonSz.y));
		addButton->SetOnClicked([this]() { dichroicReflect.Set(LaserColor::Red, !dichroicReflect.HasColor(LaserColor::Red)); });
		parent->AddChild(addButton);

		addButton = std::make_shared<Button>("");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(buttonSz.x, buttonSz.y));
		addButton->SetOnClicked([this]() { dichroicReflect.Set(LaserColor::Green, !dichroicReflect.HasColor(LaserColor::Green)); });
		parent->AddChild(addButton);

		addButton = std::make_shared<Button>("");
		addButton->SetSize(buttonSz);
		addButton->SetOffset(Vector2f(buttonSz.x * 2.0f, buttonSz.y));
		addButton->SetOnClicked([this]() { dichroicReflect.Set(LaserColor::Blue, !dichroicReflect.HasColor(LaserColor::Blue)); });
		parent->AddChild(addButton);
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(nextAngle, "angle");
		ar(dichroicReflect, "dichroicReflect");
		if (ar.isInput())
		{
			prevAngle = nextAngle;
			moveFraction = 1.0f;
			moveSpeed = 0.0f;
		}
	}

private:
	void RotateTo(float _nextAngle, float blendTime)
	{
		if (blendTime >= 0.0f)
		{
			prevAngle = nextAngle;
			nextAngle = _nextAngle;
			moveFraction = 0.0f;
			moveSpeed = 1.0f / blendTime;
		}
		else
		{
			prevAngle = nextAngle = _nextAngle;
			moveFraction = 1.0f;
		}
	}

	float CurrentAngle() const
	{
		return Lerp(prevAngle, nextAngle, Hermite2(moveFraction));
	}

	Vector2f LerpReflect(Vector2f direction)
	{
		const Vector2f prevNormal = AngleToVector2f(prevAngle);
		const Vector2f nextNormal = AngleToVector2f(nextAngle);
		const Vector2f prevRefl = RoundDirection(Reflect(direction, prevNormal));
		const Vector2f nextRefl = RoundDirection(Reflect(direction, nextNormal));
		return Normalize(Lerp(prevRefl, nextRefl, Hermite2(moveFraction)));
	}

	bool IsRotating() const
	{
		return moveFraction < 1.0f;
	}

private:
	LaserColorMask dichroicReflect = laserWhite;
	float prevAngle = 0.0f;
	float nextAngle = 0.0f;
	float moveFraction = 1.0f;
	float moveSpeed = 0.0f;
};

YASLI_CLASS(Element, Mirror, "Mirror")



class DichroicPrism : public Element
{
public:
	DichroicPrism() = default;
	DichroicPrism(Vector2i position)
		: Element(position) {}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		Laser outputLaser;
		outputLaser.start = Vector2f(GetPosition());
		outputLaser.color = inputSegment.color;
		
		if (inputSegment.color == LaserColor::Red && Dot(-AngleToVector2f(angleRed), inputSegment.direction) > 0.95f)
		{
			outputLaser.direction = AngleToVector2f(angleWhite);
			outputSegments->push_back(outputLaser);
		}
		if (inputSegment.color == LaserColor::Green && Dot(-AngleToVector2f(angleGreen), inputSegment.direction) > 0.95f)
		{
			outputLaser.direction = AngleToVector2f(angleWhite);
			outputSegments->push_back(outputLaser);
		}
		if (inputSegment.color == LaserColor::Blue && Dot(-AngleToVector2f(angleBlue), inputSegment.direction) > 0.95f)
		{
			outputLaser.direction = AngleToVector2f(angleWhite);
			outputSegments->push_back(outputLaser);
		}
		if (Dot(-AngleToVector2f(angleWhite), inputSegment.direction) > 0.95f)
		{
			switch (inputSegment.color)
			{
			case LaserColor::Red:
				outputLaser.direction = AngleToVector2f(angleRed);
				outputSegments->push_back(outputLaser);
				break;
			case LaserColor::Green:
				outputLaser.direction = AngleToVector2f(angleGreen);
				outputSegments->push_back(outputLaser);
				break;
			case LaserColor::Blue:
				outputLaser.direction = AngleToVector2f(angleBlue);
				outputSegments->push_back(outputLaser);
				break;
			}
		}
	}

	void Draw() override
	{
		DrawBase();
		
		Vector2f rotation;
		glBegin(GL_TRIANGLES);
		glColor(GetLaserColor(laserWhite));
		rotation = AngleToVector2f(angleWhite);
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, 0.5f)));
		glColor(GetLaserColor(LaserColor::Red));
		rotation = AngleToVector2f(angleRed);
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, 0.5f)));
		glColor(GetLaserColor(LaserColor::Green));
		rotation = AngleToVector2f(angleGreen);
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, 0.5f)));
		glColor(GetLaserColor(LaserColor::Blue));
		rotation = AngleToVector2f(angleBlue);
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.5f, 0.5f)));
		glEnd();
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angleRed,   "angleRed");
		ar(angleGreen, "angleGreen");
		ar(angleBlue,  "angleBlue");
		ar(angleWhite, "angleWhite");
	}

private:
	float angleRed   = 0.0f;
	float angleGreen = 90.0f;
	float angleBlue  = 180.0f;
	float angleWhite = 270.0f;
};

YASLI_CLASS(Element, DichroicPrism, "DichroicPrism")



class Difractor : public Element
{
public:
	Difractor() = default;
	Difractor(Vector2i position)
		: Element(position) {}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		Laser outputLaser;
		outputLaser.start = Vector2f(GetPosition());
		outputLaser.color = inputSegment.color;
		const Vector2f facing = AngleToVector2f(angle);
		const float dot = Dot(facing, inputSegment.direction);
		if (std::abs(dot) > 0.95f)
		{
			outputLaser.direction = inputSegment.direction;
			outputSegments->push_back(outputLaser);

			outputLaser.direction = RoundDirection(AngleToVector2f(angle + angleStep) * Sign(dot));
			outputSegments->push_back(outputLaser);

			outputLaser.direction = RoundDirection(AngleToVector2f(angle - angleStep) * Sign(dot));
			outputSegments->push_back(outputLaser);
		}
	}

	void Draw() override
	{
		DrawBase();

		Vector2f rotation = AngleToVector2f(angle);
		glPointSize(3.0f);
		glColor(GetLaserColor(laserWhite));
		glBegin(GL_POINTS);
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, -0.4f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, -0.2f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f,  0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f,  0.2f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f,  0.4f)));
		glEnd();
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angle, "angle");
	}

private:
	float angle = 0.0f;
};

YASLI_CLASS(Element, Difractor, "Difractor")



class Filter : public Element
{
public:
	Filter() = default;
	Filter(Vector2i position)
		: Element(position) {}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		if (mask.HasColor(inputSegment.color))
			return;
		Laser outputLaser;
		outputLaser.start = Vector2f(GetPosition());
		outputLaser.color = inputSegment.color;
		outputLaser.direction = inputSegment.direction;
		if (std::abs(Dot(AngleToVector2f(angle), inputSegment.direction)) > 0.95f)
			outputSegments->push_back(outputLaser);
	}

	void Draw() override
	{
		DrawBase();

		Vector2f rotation = AngleToVector2f(angle);
		glLineWidth(3.0f);
		glColor(GetLaserColor(mask));
		glBegin(GL_LINES);
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, -0.45f)));
		glVertex(RotateVector2f(rotation, Vector2f(0.0f, 0.45f)));
		glEnd();
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(mask, "mask");
		ar(angle, "angle");
	}

private:
	LaserColorMask mask = laserWhite;
	float angle = 0.0f;
};

YASLI_CLASS(Element, Filter, "Filter")



class Sensor : public Element
{
public:
	Sensor() = default;
	Sensor(Vector2i position)
		: Element(position)
	{
	}

	void Update(Time timer) override
	{
		active = false;
	}

	void PostUpdate(Time deltaTime) override
	{
		if (active != wasActive)
			SetTargetsActive(active);
		wasActive = active;
	}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		if (!mask.HasColor(inputSegment.color))
			return;
		if (std::abs(Dot(-AngleToVector2f(angle), inputSegment.direction)) > 0.95f)
			active = true;
	}

	void Draw() override
	{
		const Vector2f rotation = AngleToVector2f(angle);
		glColor(GetLaserColor(mask));
		if (active)
			glBegin(GL_TRIANGLES);
		else
			glBegin(GL_LINE_LOOP);
		glVertex(RotateVector2f(rotation, Vector2f(-0.5f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.0f,  0.0f)));
		glVertex(RotateVector2f(rotation, Vector2f(-0.5f,  0.5f)));
		glEnd();
	}

	virtual bool BlocksLaser() const override
	{
		return false;
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angle, "angle");
		ar(mask, "mask");
	}

private:
	LaserColorMask mask = laserWhite;
	float angle = 0.0f;
	bool active = false;
	bool wasActive = false;
};

YASLI_CLASS(Element, Sensor, "Sensor")



class Blocker : public Element
{
public:
	Blocker() = default;
	Blocker(Vector2i position)
		: Element(position) {}

	void SetActive(bool active) override
	{
		moveDir = active ? -1.0f : 1.0f;
	}

	void ProcessLaser(PendingSegments* outputSegments, const Laser& inputSegment) override
	{
		Laser outputLaser = inputSegment;
		outputLaser.start = Vector2f(GetPosition());
		if (state < 0.75f)
			outputSegments->push_back(outputLaser);
	}

	void Update(Time timer) override
	{
		state = Clamp(state + timer.AsFloat() * moveDir * 2.0f, 0.15f, 1.0f);
	}

	void Draw() override
	{
		const Vector2f rotation = AngleToVector2f(angle);

		glColor(Color{zero});
		const float size = 0.5f;
		glBegin(GL_QUADS);
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, 0.5f - state * 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, 0.5f - state * 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, -(0.5f - state * 0.5f))));
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, -(0.5f - state * 0.5f))));
		glEnd();

		glLineWidth(2);
		glColor(baseLineColor);
		glBegin(GL_LINE_LOOP);
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, 0.5f - state * 0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, 0.5f - state * 0.5f)));
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, -0.5f)));
		glVertex(RotateVector2f(rotation, Vector2f( 0.175f, -(0.5f - state * 0.5f))));
		glVertex(RotateVector2f(rotation, Vector2f(-0.175f, -(0.5f - state * 0.5f))));
		glEnd();
	}

	virtual void serialize(yasli::Archive& ar)
	{
		Element::serialize(ar);
		ar(angle, "angle");
	}

private:
	float angle = 0.0f;
	float state = 1.0f;
	float moveDir = 1.0f;
};

YASLI_CLASS(Element, Blocker, "Blocker")



/////////////////////////////////////////////


}


namespace yasli
{


bool serialize(yasli::Archive& ar, LaserColorMask& mask, const char* name, const char* label)
{
	ar(mask.mask, name, label);
	return true;
}


}



namespace
{



class LevelState : public GameState
{
public:
	LevelState(ProtoGL& graphics, Text& text, const char* levelName)
		: GameState(graphics, text)
		, camera(graphics.GetCanvas(), CameraOrtho2D::Upward, CameraListener_None)
	{
		map = std::make_shared<Map>();
		yasli::JSONIArchive ar;
		ar.load(levelName);
		ar(*map);
		camera.SetCamera(Vector2f(map->GetWidth() * 0.5f, map->GetHeigth() * 0.5f), 18.0f);
		map->Update(Time(), Vector2f(-1.0f, -1.0f), true);
		FadeIn();
	}

	void StateUpdate(Time deltaTime) override
	{
		const bool wasSolved = map->IsSolved();
		Vector2f cursorPosition = camera.CursorPosition();
		map->Update(deltaTime, cursorPosition, active);
		bool isSolved = map->IsSolved();
		if (!wasSolved && isSolved)
		{
			active = false;
			WaitAndFadeOut();
		}
	}

	void StateDraw() override
	{
		camera.CommitToGL();
		map->Draw(Hermite2(fade), false);
	}

private:
	void FadeIn()
	{
		AddActiveAction([this](Time deltaTime)
		{
			fade = Min(fade + deltaTime.AsFloat() / 0.35f, 1.0f);
			if (fade == 1.0f)
			{
				active = true;
				return false;
			}
			return true;
		});
	}

	void WaitAndFadeOut()
	{
		AddLatentAction(0.5f, [this]()
		{
			AddActiveAction([this](Time deltaTime)
			{
				fade = Max(fade - deltaTime.AsFloat() / 0.35f, 0.0f);
				if (fade == 0.0f)
				{
					FinishState();
					return false;
				}
				return true;
			});
		});
	}

private:
	std::shared_ptr<Map> map;
	CameraOrtho2D camera;
	float fade = 0.0f;
	bool active = false;
};




class LevelListState : public GameState
{
public:
	LevelListState(ProtoGL& graphics, Text& text)
		: GameState(graphics, text)
	{
		levelList.push_back("data/lasers/basic01.json");
		levelList.push_back("data/lasers/basic02.json");
		levelList.push_back("data/lasers/basic03.json");
		levelList.push_back("data/lasers/basic04.json");
		levelList.push_back("data/lasers/basic05.json");
		levelList.push_back("data/lasers/basic06.json");
		levelList.push_back("data/lasers/level01.json");
	}

	void StateUpdate(Time deltaTime) override;

private:
	std::vector<const char*> levelList;
	uint currentLevel = -1;
};



class GUIState : public GameState
{
public:
	GUIState(ProtoGL& graphics, Text& text)
		: GameState(graphics, text)
		, camera(graphics.GetCanvas(), CameraOrtho2D::Downward, CameraListener_None)
		, widget(std::make_shared<Widget>())
	{
		camera.SetCamera(Vector2f(0.5f, 0.5f), 1.0f);
		widget->SetSize(Vector2f(1.0f, 1.0f));
	}

	void StateUpdate(Time deltaTime) override
	{
		Vector2f cursorPos = CursorPosition();
		leftButton.Update();
		rightButton.Update();
		if (leftButton.WasPressed())
			widget->ButtonDown(cursorPos, MouseButton::Left);
		if (leftButton.WasReleased())
			widget->ButtonUp(cursorPos, MouseButton::Left);
		if (rightButton.WasPressed())
			widget->ButtonDown(cursorPos, MouseButton::Right);
		if (rightButton.WasReleased())
			widget->ButtonUp(cursorPos, MouseButton::Right);
		widget->Update(cursorPos);
	}

	void StateDraw() override
	{
		camera.CommitToGL();
		Painter painter{ GetGraphics(), GetText() };
		widget->Draw(painter);
	}

protected:
	Vector2f  CursorPosition() const { return camera.CursorPosition(); }
	WidgetPtr GetWidget() { return widget; }

private:
	WidgetPtr     widget;
	CameraOrtho2D camera;
	Input leftButton  { VK_LBUTTON };
	Input rightButton { VK_RBUTTON };
};



class LevelEditorState : public GUIState
{
private:
	static const uint maxMapSize = 18;

public:
	LevelEditorState(ProtoGL& graphics, Text& text)
		: GUIState(graphics, text)
	{
		map = std::make_shared<Map>(8, 8);

	//	map = std::make_shared<Map>();
	//	yasli::JSONIArchive ar;
	//	ar.load("data/lasers/level01.json");
	//	ar(*map);

		auto playButton = std::make_shared<Button>("Play");
		playButton->SetOffset(Vector2f(0.0f, 0.05f));
		playButton->SetSize(Vector2f(0.15f, 0.04f));
		playButton->SetOnClicked([this]()
		{
			const char* testMapName = "data/lasers/__temp.json";
			SaveMap(testMapName);
			SetSubState(std::make_shared<LevelState>(GetGraphics(), GetText(), testMapName));
		});
		GetWidget()->AddChild(playButton);

		elementEditor = make_shared<Widget>();
		elementEditor->SetOffset(Vector2f(1.0f - 0.12f, 0.5f));
		elementEditor->SetSize(Vector2f(0.12f, 0.5f));
		GetWidget()->AddChild(elementEditor);

		{
			const Vector2f buttonSz = Vector2f(0.12f, 0.04f);
			float offset = 0.2f;
			std::shared_ptr<Button> addButton;

			addButton = std::make_shared<Button>("Emitter");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Emitter>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;

			addButton = std::make_shared<Button>("Receiver");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Receiver>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;

			addButton = std::make_shared<Button>("Mirror");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Mirror>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;

			addButton = std::make_shared<Button>("Dichroic Prism");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<DichroicPrism>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;
			
			addButton = std::make_shared<Button>("Difractor");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Difractor>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;
			
			addButton = std::make_shared<Button>("Filter");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Filter>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;
			
			addButton = std::make_shared<Button>("Sensor");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Sensor>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;
			
			addButton = std::make_shared<Button>("Blocker");
			addButton->SetSize(buttonSz);
			addButton->SetOffset(Vector2f(1.0f - 0.12f, offset));
			addButton->SetOnClicked([this]() { CreateElement<Blocker>(); });
			GetWidget()->AddChild(addButton);
			offset += buttonSz.y;
		}
	}

	void StateUpdate(Time deltaTime) override
	{
		GUIState::StateUpdate(deltaTime);
		UpdateMapTransform();
	}

	void StateDraw() override
	{
		Vector2f cursorPos = CursorPosition();

		Painter painter{ GetGraphics(), GetText() };

		GUIState::StateDraw();
		glPushMatrix();
		glScalef(mapScale.x, mapScale.y, 1.0f);
		glTranslatef(mapOffset.x, mapOffset.y, 0.0f);
		map->Draw(1.0f, true);
		glPopMatrix();

		Vector2i coord = CursorToMap(cursorPos);
		if (selectedPoint)
		{
			painter.SetPen({Color(0.7f, 0.7f, 0.7f), 1.0f});
			painter.SetBrush({});
			painter.DrawCircle(CoordtoCanvas(selectedPointCoord), 0.02f);
		}
		if (selectedElement)
		{
			painter.SetPen({ Color(0.25f, 0.75f, 0.25f), 1.0f });
			painter.SetBrush({});
			painter.DrawCircle(CoordtoCanvas(selectedElement->GetPosition()), 0.022f);
		}
		if (selectedElement && targeting)
		{
			painter.SetPen({ validTarget ? Color(0.25f, 0.75f, 0.25f) : Color(0.75f, 0.25f, 0.25f), 2.0f });
			painter.DrawLine(
				CoordtoCanvas(selectedElement->GetPosition()),
				CursorPosition());
		}
	}

private:
	void UpdateMapTransform()
	{
		const float scale = 1.0f / float(maxMapSize) * 0.5f;
		const Vector2f mapSize = Vector2f(map->GetWidth(), map->GetHeigth()) * scale;
		const Vector2f center = Vector2f(0.5f, 0.5f);
		mapRect.min = center - mapSize * 0.5f;
		mapRect.max = center + mapSize * 0.5f;
		mapScale = Vector2f(scale, -scale);
		mapOffset = Vector2f(mapRect.min.x, -mapRect.max.y) / scale;
	}

	Vector2i CursorToMap(Vector2f cursor)
	{
		return Vector2i((cursor - mapOffset * mapScale) / mapScale + Vector2f(0.5f, 0.5f));
	}

	Vector2f CoordtoCanvas(Vector2i coord)
	{
		return (Vector2f(coord) + mapOffset) * mapScale;
	}

	bool IsCoordOnMap(Vector2i coord)
	{
		return
			coord.x >= 0 && coord.y >= 0 &&
			coord.x < map->GetWidth() && coord.y < map->GetHeigth();
	}

	template<typename ElementT>
	void CreateElement()
	{
		assert(selectedPoint);
		map->AddElement(std::make_shared<ElementT>(selectedPointCoord));
	}

	void SaveMap(const char* fileName)
	{
		yasli::JSONOArchive ar;
		ar(*map);
		ar.save(fileName);
	}

	void LeftMouseDown(Vector2i point) override
	{
		Vector2f cursorPos = CursorPosition();
		Vector2i coord = CursorToMap(cursorPos);

	//	selectedPointCoord = coord;
	//	selectedPoint = false;
	//	movingElement = false;
		if (targeting)
		{
			if (validTarget)
			{
				auto target = map->GetElement(coord);
				selectedElement->AddTarget(target);
			}
		}
		else if (IsCoordOnMap(coord))
		{
			selectedPointCoord = coord;
			selectedPoint = false;
			movingElement = false;

			auto newSelectedElement = map->GetElement(coord);
			if (newSelectedElement)
			{
				if (newSelectedElement != selectedElement)
				{
					elementEditor->Clear();
					newSelectedElement->MakeEditorGui(elementEditor);
				}
				movingElement = true;
			}
			else
			{
				elementEditor->Clear();
				selectedPoint = true;
			}
			selectedElement = newSelectedElement;
		}
		else
		{
			selectedElement.reset();
		}
	}

	void LeftMouseUp(Vector2i point)
	{
		movingElement = false;
	}

	void MouseMove(Vector2i point)
	{
		const Vector2f cursorPos = CursorPosition();
		const Vector2i coord = CursorToMap(cursorPos);
		if (movingElement)
		{
			if (IsCoordOnMap(coord) && !map->GetElement(coord))
				map->SetElementPosition(selectedElement, coord);
		}
		if (targeting)
		{
			validTarget = false;
			if (IsCoordOnMap(coord))
			{
				auto element = map->GetElement(coord);
				if (element && element != selectedElement)
					validTarget = true;
			}
		}
	}

	void RightMouseDown(Vector2i point) override
	{
		selectedElement.reset();
		movingElement = false;
		selectedPoint = false;
	}

	void KeyDown(KeyCode keyCode)
	{
		if (uint(keyCode) == VK_DELETE && selectedElement)
		{
			map->RemoveElement(selectedElement);
			selectedElement.reset();
		}
		else if (!targeting && uint(keyCode) == 'T' && selectedElement)
		{
			targeting = true;
			validTarget = false;
		}
	}

	void KeyUp(KeyCode keyCode)
	{
		if (uint(keyCode) == 'T')
		{
			targeting = false;
			validTarget = true;
		}
		if (uint(keyCode) == 'Y' && selectedElement)
		{
			selectedElement->ClearTargets();
		}
	}

private:
	std::shared_ptr<Map> map;
	ElementPtr selectedElement;
	WidgetPtr widget;
	WidgetPtr elementEditor;
	Rectf     mapRect;
	Vector2f  mapOffset;
	Vector2f  mapScale;
	Vector2i  selectedPointCoord;
	bool      movingElement = false;
	bool      selectedPoint = false;
	bool      targeting = false;
	bool      validTarget = false;
};



class MainMenuState : public GUIState
{
public:
	MainMenuState(ProtoGL& graphics, Text& text)
		: GUIState(graphics, text)
	{
		auto playButton = std::make_shared<Button>("Play");		
		playButton->SetOffset(Vector2f(0.1f, 0.2f));
		playButton->SetSize(Vector2f(0.3f, 0.1f));
		playButton->SetOnClicked([this]()
		{
			GetParent()->SetSubState(std::make_shared<LevelListState>(GetGraphics(), GetText()));
		});
		GetWidget()->AddChild(playButton);

		auto editorButton = std::make_shared<Button>("Editor");
		editorButton->SetOffset(Vector2f(0.1f, 0.35f));
		editorButton->SetSize(Vector2f(0.3f, 0.1f));
		editorButton->SetOnClicked([this]()
		{
			GetParent()->SetSubState(std::make_shared<LevelEditorState>(GetGraphics(), GetText()));
		});
		GetWidget()->AddChild(editorButton);

		auto exitButton = std::make_shared<Button>("Exit");
		exitButton->SetOffset(Vector2f(0.1f, 0.5f));
		exitButton->SetSize(Vector2f(0.3f, 0.1f));
		exitButton->SetOnClicked([this]()
		{
			FinishState();
		});
		GetWidget()->AddChild(exitButton);
	}

private:
};




void LevelListState::StateUpdate(Time deltaTime)
{
	if (!HasSubState())
	{
		currentLevel++;
		if (currentLevel >= levelList.size())
			GetParent()->SetSubState(std::make_shared<MainMenuState>(GetGraphics(), GetText()));
		else
			SetSubState(std::make_shared<LevelState>(GetGraphics(), GetText(), levelList[currentLevel]));
	}
}




}



void Lasers()
{
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	Text text;
	text.Init();

	GameStatePtr gameState;
	gameState = std::make_shared<GameState>(protoGL, text);
	gameState->SetSubState(std::make_shared<MainMenuState>(protoGL, text));

	TimeCounter timer;
	while (gameState->HasSubState() && protoGL.Update())
	{
		Time deltaTime = timer.GetElapsed();

		gameState->Update(deltaTime);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
				
		gameState->Draw();

		protoGL.Swap();
	}
}
