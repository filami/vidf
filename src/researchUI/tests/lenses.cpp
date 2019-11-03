#include "pch.h"
#include "lenses.h"
#include "yasli/Enum.h"
#include "yasli/STL.h"
#include "yasli/decorators/Range.h"
#include "yasli/JSONIArchive.h"
#include "yasli/JSONOArchive.h"

using namespace vidf;


namespace lens
{


YASLI_ENUM_BEGIN(SurfaceType, "SurfaceType")
YASLI_ENUM(SurfaceType_Flat, "flat", "Flat")
YASLI_ENUM(SurfaceType_Spherical, "spherical", "Spherical")
YASLI_ENUM_END()



YASLI_ENUM_BEGIN(SymetryMode, "SymetryMode")
YASLI_ENUM(SymetryMode_Symetric, "Symetric", "Symetric")
YASLI_ENUM(SymetryMode_Miniscus, "Miniscus", "Miniscus")
YASLI_ENUM(SymetryMode_Asymetric, "Asymetric", "Asymetric")
YASLI_ENUM_END()



YASLI_ENUM_BEGIN(RayFocusType, "RayFocusType")
YASLI_ENUM(RayFocusType_RelativePlane, "RelativePlane", "RelativePlane")
YASLI_ENUM(RayFocusType_AbsolutePlane, "AbsolutePlane", "AbsolutePlane")
YASLI_ENUM(RayFocusType_Infinite, "Infinite", "Infinite")
YASLI_ENUM_END()



YASLI_CLASS(Element, LensElement, "Lens")
YASLI_CLASS(Element, MarkerElement, "Marker")
YASLI_CLASS(Element, ElementCompound, "Compound")



void AddSurfacePoints(QPainter& painter, QPointF center, double radius, double t0, double t1, uint segments, QVector<QPointF>* points)
{
	assert(points != nullptr);

	for (uint i = 0; i <= segments; ++i)
	{
		const double t = Lerp(t0, t1, i / double(segments));
		const double x = std::cos(t) * radius + center.x();
		const double y = std::sin(t) * radius + center.y();
		points->push_back(QPointF(x, y));
	}
}



Vector3f Refract(Vector3f I, Vector3f N, float eta)
{
	const float k = 1.0 - eta * eta * (1.0 - Dot(N, I) * Dot(N, I));
	if (k < 0.0)
		return Vector3f(zero);
	else
		return eta * I - (eta * Dot(N, I) + std::sqrt(k)) * N;
}



bool RaySphereIntersect(const Rayf& ray, const Vector3f center, const float radius, Intersect* intersect)
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
	}
	return true;
}



bool RaySphereIntersect(const Rayf& ray, const Vector3f center, const float radius, Vector3f axis, float theta, Intersect* intersect)
{
	Intersect _intersect;
	const bool _result = RaySphereIntersect(ray, center, radius, &_intersect);
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
		ar(yasli::Range(radius, -200.0f, 200.0f), "radius", "^");
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



void LensElement::QtDraw(QPainter& painter, float parentOffset, float pixelSize)
{
	const QPointF c1 = QPointF( backSurface.radius - d * 0.5 + (parentOffset + offset), 0.0);
	const QPointF c2 = QPointF(-frontSurface.radius + d * 0.5 + (parentOffset + offset), 0.0);

	const double t1 = std::asin(diameter * 0.5 / backSurface.radius);
	const double t2 = std::asin(diameter * 0.5 / frontSurface.radius);
		
	QVector<QPointF> points;
	painter.setPen(QPen(Qt::white, pixelSize * 1.5f));
	painter.setBrush(QBrush(QColor(0, 128, 255, 64)));
	AddSurfacePoints(painter, c1, backSurface.radius,  -t1 + PI, t1 + PI, 64, &points);
	AddSurfacePoints(painter, c2, frontSurface.radius, -t2, t2, 64, &points);
	painter.drawPolygon(points.data(), points.size());

	painter.setPen(QPen(Qt::gray, pixelSize));
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

	const bool back = RaySphereIntersect(ray, center1, backSurface.radius * conv, Vector3f(-1.0f, 0.0f, 0.0f), t1, intersect);
	const bool front = RaySphereIntersect(ray, center2, frontSurface.radius * conv, Vector3f(1.0f, 0.0f, 0.0f), t2, intersect);
	const bool result = back || front;
	if (front)
		intersect->n = n;
	else if (back)
		intersect->n = 1.0f;

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
	const float _r2 = -frontSurface.radius * invConv;
	return (1.0f / ((n - 1.0f)*(1.0f / _r1 - 1.0f / _r2 + (n - 1.0f)*_d / (n*_r1*_r2)))) * conv;
}



void MarkerElement::QtDraw(QPainter& painter, float parentOffset, float pixelSize)
{
	painter.setPen(QPen(Qt::gray, pixelSize));
	painter.drawLine(
		QPointF(parentOffset + offset,  1000),
		QPointF(parentOffset + offset, -1000));
}



void ElementCompound::serialize(yasli::Archive& ar)
{
	Element::serialize(ar);
	ar(name, "name", "^");
	ar(elements, "elements", "Elements");
}



void ElementCompound::QtDraw(QPainter& painter, float parentOffset, float pixelSize)
{
	for (auto& element : elements)
	{
		if (element->enabled)
			element->QtDraw(painter, parentOffset + offset, pixelSize);
	}
}



bool ElementCompound::RayIntersect(const Rayf& ray, Intersect* intersect, float parentOffset)
{
	bool result = false;
	for (auto& element : elements)
	{
		if (element->enabled)
			result |= element->RayIntersect(ray, intersect, parentOffset + offset);
	}
	return result;
}



float ElementCompound::GetMaxOffset(float parentOffset) const
{
	float maxOffset = parentOffset;
	for (auto& element : elements)
	{
		if (element->enabled)
			maxOffset = Max(maxOffset, element->GetMaxOffset(parentOffset + offset));
	}
	return maxOffset;
}



Renderer::Renderer(Document* _document)
	: document(_document)
{
	timer.start(12, this);
}



void Renderer::timerEvent(QTimerEvent *e)
{
	update();
}



void Renderer::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() == Qt::RightButton)
	{
		lastPoint = event->pos();
		event->accept();
	}
	QWidget::mousePressEvent(event);
}



void Renderer::mouseReleaseEvent(QMouseEvent* event)
{
	QWidget::mouseReleaseEvent(event);
}



void Renderer::mouseDoubleClickEvent(QMouseEvent* event)
{
	QWidget::mouseDoubleClickEvent(event);
}



void Renderer::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() == Qt::RightButton)
	{
		QPoint move = event->pos() - lastPoint;
		position = position + move;
		event->accept();
	}
	lastPoint = event->pos();
	QWidget::mouseMoveEvent(event);
}



void Renderer::wheelEvent(QWheelEvent* event)
{
//	if (event->delta() > 0)
//		_size *= 1.5f;
//	else
//		_size /= 1.5f;
//	event->accept();

	if (event->delta() > 0)
		scale *= 1.5f;
	else
		scale /= 1.5f;
	event->accept();
}



void Renderer::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Plus)
	{
		scale *= 1.5f;
		return;
	}
	if (event->key() == Qt::Key_Minus)
	{
		scale /= 1.5f;
		return;
	}
	QWidget::keyPressEvent(event);
}



void Renderer::paintEvent(QPaintEvent* event)
{
	/*/
	const QSize sz = size();
	const float aspect = sz.width() / float(sz.height());
	const float pixelSize = 1.0f / sz.height() * _size;

	QPainter painter;
	painter.begin(this);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPointF sizeF = QPointF(_size * aspect, _size) * 0.5f;
	QPointF minPt = position - sizeF;
	QPointF maxPt = position + sizeF;

	painter.fillRect(event->rect(), QBrush(QColor(0, 0, 0)));
//	painter.translate(minPt / pixelSize);
//	painter.scale((1.0 / pixelSize) * aspect, 1.0 / pixelSize);

	painter.translate(QPointF(minPt.x() * (sz.width() / _size), minPt.y() * (sz.height() / _size)));
	painter.scale(1 / (_size / sz.width()), 1 / (_size / sz.height()));
	/**/

	/**/
	const QSize sz = size();
	const float aspect = sz.width() / float(sz.height());
	const float pixelSize = 1.0f / sz.height() * _size;

	QPainter painter;
	painter.begin(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(event->rect(), QBrush(QColor(0, 0, 0)));

	painter.translate(position.x() + 1000.0, position.y());
	painter.scale(scale * aspect, scale);
	/**/

	painter.save();
	painter.setBrush(QBrush());
	
	painter.setPen(QPen(Qt::white, pixelSize * 1.5f));
	painter.drawLine(
		QPointF(document->sensor.offset, -document->sensor.size*0.5),
		QPointF(document->sensor.offset,  document->sensor.size*0.5));

	const float frontLensOffset = document->elements.GetMaxOffset(0.0f) / 1000.0f;
	for (const auto& source : document->raySources)
	{
		if (source.enabled)
			DrawRaytraceSource(painter, source, frontLensOffset, pixelSize);
	}

	document->elements.QtDraw(painter, 0.0f, pixelSize);

	painter.end();
}



void Renderer::DrawRaytraceSource(QPainter& painter, const RaySources& source, float frontLensOffset, float pixelSize)
{
	// const float rayDist = 25.0f;
	const float rayDist = 1.5f;
	const float spread = (source.spread * document->sensor.size * 0.5f) / 1000.0f;
	const float mult = 1.0f / float(source.count - 1);
	const float theta = Degrees2Radians(source.angle * 0.5f);
	const float thetaTan = std::tan(theta);
		
	for (uint i = 0; i < source.count; ++i)
	{
		/*
		if (i == source.count - 1)
			painter.setPen(QPen(Qt::green, pixelSize * 1.5f));
		else
			painter.setPen(QPen(Qt::blue, pixelSize * 1.5f));
			*/
		painter.setPen(QPen(Qt::blue, pixelSize * 1.5f));
		const float t = Lerp(-spread, spread, i * mult);
		const Vector3f dir = Normalize(Vector3f{ -1.0f, thetaTan, 0.0f });
		const Vector3f orig = Vector3f{
			frontLensOffset - dir.x * rayDist,
			t - dir.y * rayDist + source.offset * document->sensor.size * 0.5f / 1000.0f,
			0.0f };
		DrawRayIntersect(painter, Rayf{ orig, dir });
	}
}



void Renderer::DrawRayIntersect(QPainter& painter, const Rayf& ray)
{
	const float conv = 1000.0;
	const float invConv = 1.0 / 1000.0;
	const float epsilon = 1.0f / (1024.0f * 16.0f);

	auto ToQPoint = [](Vector3f v) { return QPointF(v.x, v.y); };

	Rayf curRay = ray;
	float curN = 1.0f;

	QVector<QPointF> points;

	for (uint i = 0; i < 64; ++i)
	{
		points.push_back(ToQPoint(curRay.origin) * conv);
		Intersect intersect;
		if (!document->elements.RayIntersect(curRay, &intersect, 0.0f))
			break;
		// const float intersectDir = (intersect.n == 1.0f ? -1.0f : 1.0f);
		curRay.origin = curRay.origin + curRay.direction * (intersect.distance + epsilon);
		curRay.direction = Normalize(Refract(curRay.direction, intersect.normal, curN / intersect.n));
		curN = intersect.n;
		points.push_back(ToQPoint(curRay.origin) * conv);
	};
	points.push_back(ToQPoint(curRay.origin + curRay.direction * 2500.0) * conv);

	painter.drawLines(points);

	painter.setPen(QPen(Qt::red, 0.2f));
	painter.drawPoints(points);
}



void RaySources::serialize(yasli::Archive& ar)
{
	ar(enabled, "enabled", "^");
	ar(focusType, "focusType", "Type");
	// if (focusType != RayFocusType_Infinite)
		ar(offset, "offset", "Offset");
	ar(angle, "angle", "angle");
	ar(spread, "spread", "spread");
	ar(count, "count", "count");
}



Document::Document()
{
	raySources.emplace_back(RaySources{});
	raySources.emplace_back(RaySources{});
	raySources.back().angle = 27.0f;

	elements.elements.emplace_back(new MarkerElement{});
	elements.elements.emplace_back(new LensElement{});
	elements.elements.back()->offset = 90.0f;
}



void Document::serialize(yasli::Archive& ar)
{
	ar(sensor, "sensor", "Sensor");
	ar(raySources, "raySources", "Rays");
	ar(elements, "elements", "Elements");
}



template<typename Parent, typename Callback>
void AddMenuAction(Parent* parent, QMenu* menu, const QString& name, Callback callback)
{
	QAction* action = new QAction(name, parent);
	parent->connect(action, &QAction::triggered, parent, callback);
	menu->addAction(action);
}



template<typename SaveCallback>
bool DialogVerifyCloseDocument(bool documentChanged, SaveCallback saveCallback)
{
	if (!documentChanged)
		return true;
	QMessageBox msgBox;
	msgBox.setText("The document has been modified.");
	msgBox.setInformativeText("Do you want to save your changes?");
	msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Save);
	int ret = msgBox.exec();
	if (ret == QMessageBox::Save)
		saveCallback();
	return ret != QMessageBox::Cancel;
}



LensesFrame::LensesFrame()
{
	QWidget* mainWidget = new QWidget();
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
	mainWidget->setLayout(layout);
		
	renderWidget = new Renderer(&document);
	layout->addWidget(renderWidget);

	simulatorWidget = new QPropertyTree();
	simulatorWidget->setUndoEnabled(true, false);
	simulatorWidget->attach(yasli::Serializer(document));
	simulatorWidget->setMaximumWidth(380);
	connect(simulatorWidget, &QPropertyTree::signalChanged, this, &LensesFrame::OnChanged);
	layout->addWidget(simulatorWidget);

	setCentralWidget(mainWidget);

	QMenu* menu = menuBar()->addMenu(tr("&File"));
	AddMenuAction(this, menu, tr("New"), &LensesFrame::OnNew);
	AddMenuAction(this, menu, tr("Open"), &LensesFrame::OnOpen);
	AddMenuAction(this, menu, tr("Save"), &LensesFrame::OnSave);
	AddMenuAction(this, menu, tr("Save As..."), &LensesFrame::OnSaveAs);

	renderWidget->setFocus();
}

void LensesFrame::OnNew()
{
	const bool discardCurrent = DialogVerifyCloseDocument(documentChanged, [this]() { OnSave(); });
	if (discardCurrent)
	{
		document = Document();
		documentPath.clear();
		documentChanged = false;
	}
}

void LensesFrame::OnOpen()
{
	const bool discardCurrent = DialogVerifyCloseDocument(documentChanged, [this]() { OnSave(); });
	if (!discardCurrent)
		return;
	QString path = QFileDialog::getOpenFileName(this, tr("Open..."), QString(), tr("Lenses(*.len)"));
	if (!path.isEmpty())
		Open(path);
}

void LensesFrame::OnSave()
{
	if (documentPath.isEmpty())
		OnSaveAs();
	else
		Save();
}

void LensesFrame::OnSaveAs()
{
	documentPath = QFileDialog::getSaveFileName(this, tr("Save..."), QString(), tr("Lenses(*.len)"));
	if (documentPath.isEmpty())
		return;
	Save();
}

void LensesFrame::OnChanged()
{
	documentChanged = true;
}

void LensesFrame::Save()
{
	assert(!documentPath.isEmpty());
	yasli::JSONOArchive ar;
	ar(document);
	ar.save(documentPath.toUtf8().data());
	documentChanged = false;
}

void LensesFrame::Open(const QString& path)
{
	yasli::JSONIArchive ar;
	ar.load(path.toUtf8().data());
	ar(document);
	documentPath = path;
	documentChanged = false;
	simulatorWidget->revert();
}

}

int LensesTest(int argc, char** argv)
{
	QApplication app{ argc, argv };

	lens::LensesFrame lenses;
	lenses.showMaximized();

	return app.exec();
}