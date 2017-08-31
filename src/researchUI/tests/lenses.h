#pragma once

#include "pch.h"
#include <QOpenGLWidget>
#include <QBasicTimer>
#include <QPaintEvent>
#include "QPropertyTree/QPropertyTree.h"
#include "yasli/Pointers.h"
#include "vidf/common/ray.h"


namespace lens
{


enum SurfaceType
{
	SurfaceType_Flat,
	SurfaceType_Spherical,
};



enum SymetryMode
{
	SymetryMode_Symetric,
	SymetryMode_Miniscus,
	SymetryMode_Asymetric,
};



struct Surface
{
	SurfaceType type = SurfaceType_Spherical;
	float radius = 100.0f;

	void serialize(yasli::Archive& ar);
};



struct Intersect
{
	vidf::Vector3f normal = vidf::Vector3f(vidf::zero);
	float distance = std::numeric_limits<float>::max();
	float n = 1.0f;
};



class Element : public yasli::RefCounter
{
public:
	bool enabled = true;
	float offset = 0.0f;

	virtual void serialize(yasli::Archive& ar);
	virtual void QtDraw(QPainter& painter, float parentOffset) {}
	virtual bool RayIntersect(const vidf::Rayf& ray, Intersect* intersect, float parentOffset) { return false; }
	virtual float GetMaxOffset(float parentOffset) const { return parentOffset; }
};



class LensElement : public Element
{
public:
	Surface backSurface;
	Surface frontSurface;
	SymetryMode symetryMode = SymetryMode_Symetric;
	float n = 1.5f;
	float d = 10.0f;
	float diameter = 45.0f;

	virtual void serialize(yasli::Archive& ar);
	virtual void QtDraw(QPainter& painter, float parentOffset) override;
	virtual bool RayIntersect(const vidf::Rayf& ray, Intersect* intersect, float parentOffset) override;
	virtual float GetMaxOffset(float parentOffset) const override;

private:
	float LensElement::GetFocusDistance() const;
};


class MarkerElement : public Element
{
public:
	virtual void QtDraw(QPainter& painter, float parentOffset) override;
};



class ElementCompound : public Element
{
public:
	void serialize(yasli::Archive& ar);
	virtual void QtDraw(QPainter& painter, float parentOffset) override;
	virtual bool RayIntersect(const vidf::Rayf& ray, Intersect* intersect, float parentOffset) override;
	virtual float GetMaxOffset(float parentOffset) const override;

	std::vector<yasli::SharedPtr<Element>> elements;
	std::string name;
};



struct Sensor
{
	float size = 35.0f;
	float offset = -10.0f;
	void serialize(yasli::Archive& ar)
	{
		ar(size, "size", "Size");
		ar(offset, "offset", "Offset");
	}
};



enum RayFocusType
{
	RayFocusType_RelativePlane,
	RayFocusType_AbsolutePlane,
	RayFocusType_Infinite,
};



struct RaySources
{
	RayFocusType focusType = RayFocusType::RayFocusType_Infinite;
	float angle = 0.0f;
	float spread = 0.5f;
	float offset = 2000.0f;
	int count = 3;
	bool enabled = true;

	void serialize(yasli::Archive& ar);
};



class Document
{
public:
	Document();

	ElementCompound elements;
	Sensor sensor;
	std::vector<RaySources> raySources;

	void serialize(yasli::Archive& ar);
};



// class Renderer : public QOpenGLWidget
class Renderer : public QWidget
{
public:
	Renderer(Document* _document);

	virtual void paintEvent(QPaintEvent* event) override;
	virtual void timerEvent(QTimerEvent* event) override;

private:
	void DrawRaytraceSource(QPainter& painter, const RaySources& source, float frontLensOffset);
	void DrawRayIntersect(QPainter& painter, const vidf::Rayf& ray);

private:
	Document*   document;
	QBasicTimer timer;
};



class LensesFrame : public QMainWindow
{
	Q_OBJECT

public:
	LensesFrame();

private slots:
	void OnNew();
	void OnOpen();
	void OnSave();
	void OnSaveAs();
	void OnChanged();

private:
	void Save();
	void Open(const QString& path);

private:
	Document       document;
	QPropertyTree* simulatorWidget;
	Renderer*      renderWidget;
	QString        documentPath;
	bool           documentChanged = false;
};

}