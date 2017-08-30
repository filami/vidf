#include "pch.h"
#include "lenses.h"
#include "yasli/Enum.h"
#include "yasli/STL.h"
#include "yasli/decorators/Range.h"

using namespace vidf;



YASLI_ENUM_BEGIN(SurfaceType, "SurfaceType")
YASLI_ENUM(SurfaceType_Flat, "flat", "Flat")
YASLI_ENUM(SurfaceType_Spherical, "spherical", "Spherical")
YASLI_ENUM_END()



YASLI_ENUM_BEGIN(SymetryMode, "SymetryMode")
YASLI_ENUM(SymetryMode_Symetric, "Symetric", "Symetric")
YASLI_ENUM(SymetryMode_Miniscus, "Miniscus", "Miniscus")
YASLI_ENUM(SymetryMode_Asymetric, "Asymetric", "Asymetric")
YASLI_ENUM_END()



YASLI_CLASS(Element, LensElement, "Lens")
YASLI_CLASS(Element, ElementCompound, "Compound")



void DrawArc(QPainter& painter, QPointF center, double radius, double t0, double t1, uint segments)
{
	QVector<QPointF> points;

	for (uint i = 0; i <= segments; ++i)
	{
		const double t = Lerp(t0, t1, i / double(segments));
		const double x = std::cos(t) * radius + center.x();
		const double y = std::sin(t) * radius + center.y();
		points.push_back(QPointF(x, y));
	}

	painter.drawLines(points);
}



Vector3f Refract(Vector3f I, Vector3f N, float eta)
{
	const float k = 1.0 - eta * eta * (1.0 - Dot(N, I) * Dot(N, I));
	if (k < 0.0)
		return Vector3f(zero);
	else
		return eta * I - (eta * Dot(N, I) + std::sqrt(k)) * N;
}



bool RaySphereIntersect(const Rayf& ray, const Vector3f center, const float radius, const float n, Intersect* intersect)
{
	const Vector3f q = center - ray.origin;
	const float c = Length(q);
	const float v = Dot(q, ray.direction);
	const float d = radius * radius - (c*c - v*v);
	if (d < 0.0f)
		return false;
	const float d0 = v - std::sqrt(d);
	const float d1 = v + std::sqrt(d);
	if (d0 < 0.0f && d1 < 0.0f)
		return false;
	const float dist = d0 < 0.0f ? d1 : d0;
	if (dist < intersect->distance)
	{
		intersect->distance = dist;
		intersect->normal = Normalize((ray.origin + ray.direction * dist) - center);
		intersect->normal = intersect->normal * (d0 < 0.0f ? -1.0f : 1.0f);
		intersect->n = (d0 < 0.0f && radius > 0.0f) ? 1.0f : n;
	}
	return true;
}



bool RaySphereIntersect(const Rayf& ray, const Vector3f center, const float radius, Vector3f axis, float theta, const float n, Intersect* intersect)
{
	Intersect _intersect;
	const bool _result = RaySphereIntersect(ray, center, radius, n, &_intersect);
	if (!_result)
		return false;

	const Vector3f p = ray.origin + ray.direction * _intersect.distance;
	const Vector3f dir = Normalize(p - center);
	const float dt = theta;
	const float d = std::acos(Dot(dir, axis * Sign(radius)));

	if (d > dt)
		return false;

	if (_intersect.distance < intersect->distance)
		*intersect = _intersect;

	return true;
}



void Surface::serialize(yasli::Archive& ar)
{
	ar(type, "type", "^");
	if (type != SurfaceType_Flat)
		ar(radius, "radius", "^");
}



void Element::serialize(yasli::Archive& ar)
{
	ar(enabled, "enabled", "^^");
	ar(yasli::Range(offset, 0.0f, 120.0f), "offset", "Offset");
}



void LensElement::serialize(yasli::Archive& ar)
{
	Element::serialize(ar);
	ar(diameter, "diameter", "Diameter");
	ar(n, "n", "n");
	ar(d, "d", "d");
	ar(symetryMode, "symetryMode", "Symmetry");
	if (symetryMode == SymetryMode_Asymetric)
	{
		ar(backSurface, "backSurface", "Back");
		ar(frontSurface, "frontSurface", "Front");
	}
	else
	{
		ar(frontSurface, "shape", "Shape");
		backSurface = frontSurface;
		if (symetryMode == SymetryMode_Miniscus)
			backSurface.radius = -backSurface.radius;
	}
	if (ar.isEdit())
	{
		float f = GetFocusDistance();
		ar(f, "f", "!f");
	}
}



void LensElement::QtDraw(QPainter& painter, float parentOffset)
{
	const QPointF c1 = QPointF( backSurface.radius - d * 0.5 + (parentOffset + offset), 0.0);
	const QPointF c2 = QPointF(-frontSurface.radius + d * 0.5 + (parentOffset + offset), 0.0);

	const double t1 = std::asin(diameter * 0.5 / backSurface.radius);
	const double t2 = std::asin(diameter * 0.5 / frontSurface.radius);
		
	painter.setPen(QPen(Qt::white, 0.5));
	DrawArc(painter, c1, backSurface.radius, -t1 + PI, t1 + PI, 64);
	DrawArc(painter, c2, frontSurface.radius, -t2, t2, 64);

	painter.setPen(QPen(Qt::gray, 0.25));
	painter.drawLine(
		QPointF(parentOffset + offset,  diameter * 0.5),
		QPointF(parentOffset + offset, -diameter * 0.5));
}



bool LensElement::RayIntersect(const Rayf& ray, Intersect* intersect, float parentOffset)
{
	const float conv = 1.0 / 1000.0;

	Vector3f center1 = Vector3f{ backSurface.radius - d * 0.5f + parentOffset + offset, 0.0f, 0.0f } * conv;
	Vector3f center2 = Vector3f{ -frontSurface.radius + d * 0.5f + parentOffset + offset, 0.0f, 0.0f } * conv;
	const float t1 = std::asin(diameter * conv * 0.5f / std::abs(backSurface.radius * conv));
	const float t2 = std::asin(diameter * conv * 0.5f / std::abs(frontSurface.radius * conv));

	bool result = false;
	result |= RaySphereIntersect(ray, center1, backSurface.radius * conv, Vector3f(-1.0f, 0.0f, 0.0f), t1, n, intersect);
	result |= RaySphereIntersect(ray, center2, frontSurface.radius * conv, Vector3f(1.0f, 0.0f, 0.0f), t2, n, intersect);

	return result;
}



float LensElement::GetMaxOffset(float parentOffset) const
{
	return parentOffset + offset + d * 0.5f;
}



float LensElement::GetFocusDistance() const
{
	const float conv = 1000.0f;
	const float invConv = 1.0f / conv;
	const float _d = d * invConv;
	const float _r1 = backSurface.radius * invConv;
	const float _r2 = frontSurface.radius * invConv;
	return (1.0f / ((n - 1.0f)*(1.0f / _r1 - 1.0f / _r2 + (n - 1.0f)*_d / (n*_r1*_r2)))) * conv;
}



void ElementCompound::serialize(yasli::Archive& ar)
{
	Element::serialize(ar);
	ar(elements, "elements", "Elements");
}



void ElementCompound::QtDraw(QPainter& painter, float parentOffset)
{
	for (auto& element : elements)
		element->QtDraw(painter, parentOffset + offset);
}



bool ElementCompound::RayIntersect(const Rayf& ray, Intersect* intersect, float parentOffset)
{
	bool result = false;
	for (auto& element : elements)
		result |= element->RayIntersect(ray, intersect, parentOffset + offset);
	return result;
}



float ElementCompound::GetMaxOffset(float parentOffset) const
{
	float maxOffset = parentOffset;
	for (auto& element : elements)
		maxOffset = Max(maxOffset, element->GetMaxOffset(parentOffset + offset));
	return maxOffset;
}



Renderer::Renderer(ElementCompound* _compound)
	: compounds(_compound)
{
	QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
	gradient.setColorAt(0.0, Qt::white);
	gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

	timer.start(12, this);
}



void Renderer::timerEvent(QTimerEvent *e)
{
	update();
}



void Renderer::paintEvent(QPaintEvent* event)
{
	const QSize sz = size();
	const float aspect = sz.width() / float(sz.height());
	const Vector2f center = Vector2f(500.0f, 0.0f);
	const float sizeY = 2000.0f;
	const float sizeX = sizeY * aspect;
	const float sensorSize = 36.0f;

	QPainter painter;
	painter.begin(this);
	painter.setRenderHint(QPainter::Antialiasing);

	painter.fillRect(event->rect(), QBrush(QColor(0, 0, 0)));
	painter.translate(350, 450);
	painter.scale(4 * aspect, 4);
	
	painter.save();
	painter.setBrush(QBrush());
	/*
	painter.setPen(QPen(Qt::darkBlue, 0.5));
	painter.drawLine(QPointF(0, 0), QPointF(2500, 0));
	*/
	painter.setPen(QPen(Qt::white, 1.5));
	painter.drawLine(
		QPointF(0, -sensorSize*0.5),
		QPointF(0,  sensorSize*0.5));

	/*
	const uint count = 12;
	const float t1 = -0.5f;
	const float t2 = 0.5f;
	painter.setPen(QPen(Qt::red, 0.5));
	for (uint i = 0; i < count; ++i)
	{
		const float t = Lerp(t1, t2, i / float(count - 1));
		DrawRayIntersect(painter, Rayf{ Vector3f{zero}, Normalize(Vector3f{1.0f, std::tan(t), 0.0f}) });
	}
	*/
	
	const uint count = 3;
	painter.setPen(QPen(Qt::red, 0.5));
	/*
	for (uint i = 0; i < count; ++i)
	{
		const float t = Lerp(-0.02f, 0.02f, i / float(count - 1));
		DrawRayIntersect(painter, Rayf{ Vector3f{ 25.0f, t, 0.0f }, Vector3f{ -1.0f, 0.0f, 0.0f } });
	}
	*/

	/*
	painter.setPen(QPen(Qt::green, 0.5));
	for (uint i = 0; i < count; ++i)
	{
		const float t = Lerp(-0.25f, 0.25f, i / float(count - 1));
		DrawRayIntersect(painter, Rayf{ Vector3f{ 0.25f, 0.0f, 0.0f }, Normalize(Vector3f{ -1.0f, std::tan(t), 0.0f }) });
	}

	painter.setPen(QPen(Qt::blue, 0.5));
	for (uint i = 0; i < count; ++i)
	{
		const float t = Lerp(-0.25f, 0.25f, i / float(count - 1));
		DrawRayIntersect(painter, Rayf{ Vector3f{ 0.25f, -0.01f, 0.0f }, Normalize(Vector3f{ -1.0f, std::tan(t), 0.0f }) });
	}
	*/

	const float rayDist = 25.0f;
	const float aperatureSize = 0.045f;
	const float aperatureOffset = compounds->GetMaxOffset(0.0f) / 1000.0f;
	painter.setPen(QPen(Qt::blue, 0.5));
	for (uint i = 0; i < count; ++i)
	{
		// const float th = 0.05f;
		const float th = Degrees2Radians(27.0f * 0.5f);
		const float t = Lerp(-aperatureSize*0.5f, aperatureSize*0.5f, i / float(count - 1)) * 0.5f;
		const Vector3f dir = Normalize(Vector3f{ -1.0f, std::tan(th), 0.0f });
		const Vector3f orig = Vector3f{ aperatureOffset - dir.x * rayDist, t - dir.y * rayDist, 0.0f };
		DrawRayIntersect(painter, Rayf{ orig, dir });
	}
	painter.setPen(QPen(Qt::green, 0.5));
	for (uint i = 0; i < count; ++i)
	{
		const float th = 0.0f;
		const float t = Lerp(-aperatureSize*0.5f, aperatureSize*0.5f, i / float(count - 1)) * 0.5f;
		const Vector3f dir = Normalize(Vector3f{ -1.0f, std::tan(th), 0.0f });
		const Vector3f orig = Vector3f{ aperatureOffset - dir.x * rayDist, t - dir.y * rayDist, 0.0f };
		DrawRayIntersect(painter, Rayf{ orig, dir });
	}

	compounds->QtDraw(painter, 0.0f);

	painter.end();
}



void Renderer::DrawRayIntersect(QPainter& painter, const Rayf& ray)
{
	const float conv = 1000.0;
	const float invConv = 1.0 / 1000.0;

	auto ToQPoint = [](Vector3f v) { return QPointF(v.x, v.y); };

	Rayf curRay = ray;
	float curN = 1.0f;

	QVector<QPointF> points;

	for (uint i = 0; i < 64; ++i)
	{
		points.push_back(ToQPoint(curRay.origin) * conv);
		Intersect intersect;
		if (!compounds->RayIntersect(curRay, &intersect, 0.0f))
			break;
		curRay.origin = curRay.origin + curRay.direction * (intersect.distance + 1.0f / 1024.0f / 1024.0f);
		curRay.direction = Normalize(Refract(curRay.direction, intersect.normal, curN / intersect.n));
		curN = intersect.n;
		points.push_back(ToQPoint(curRay.origin) * conv);
	};
	points.push_back(ToQPoint(curRay.origin + curRay.direction * 2500.0) * conv);

	painter.drawLines(points);
}



/*
void Renderer::paintGL()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const QSize sz = size();
	const float aspect = sz.width() / float(sz.height());
	const Vector2f center = Vector2f(500.0f, 0.0f);
	const float sizeY = 2000.0f;
	const float sizeX = sizeY * aspect;
	const float sensorSize = 36.0f;

	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(
		center.x - sizeX, center.x + sizeX,
		center.y - sizeY, center.y + sizeY);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glLineWidth(1.5f);
	glColor4ub(0, 0, 255, 255);
	glBegin(GL_LINES);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(2500.0f, 0.0f);
	glEnd();

	glLineWidth(3.0f);
	glColor4ub(255, 255, 255, 255);
	glBegin(GL_LINES);
	glVertex2f(0.0f, -sensorSize * 0.5f);
	glVertex2f(0.0f, sensorSize * 0.5f);
	glEnd();
}
*/



LensesFrame::LensesFrame()
{
	QWidget* mainWidget = new QWidget();
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
	mainWidget->setLayout(layout);
		
	renderWidget = new Renderer(&elements);
	layout->addWidget(renderWidget);

	simulatorWidget = new QPropertyTree();
	simulatorWidget->setUndoEnabled(true, false);
	simulatorWidget->attach(yasli::Serializer(elements));
	simulatorWidget->setMaximumWidth(250);
	layout->addWidget(simulatorWidget);

	setCentralWidget(mainWidget);
}



int LensesTest(int argc, char** argv)
{
	QApplication app{ argc, argv };

	LensesFrame lenses;
	lenses.showMaximized();

	return app.exec();
}