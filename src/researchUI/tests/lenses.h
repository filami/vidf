#pragma once

#include "pch.h"
#include <QOpenGLWidget>
#include <QBasicTimer>
#include <QPaintEvent>
#include "QPropertyTree/QPropertyTree.h"
#include "yasli/Pointers.h"
#include "vidf/common/ray.h"



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
	float radius = 200.0f;

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
	float diameter = 25.0f;

	virtual void serialize(yasli::Archive& ar);
	virtual void QtDraw(QPainter& painter, float parentOffset) override;
	virtual bool RayIntersect(const vidf::Rayf& ray, Intersect* intersect, float parentOffset) override;
	virtual float GetMaxOffset(float parentOffset) const override;

private:
	float LensElement::GetFocusDistance() const;
};



class ElementCompound : public Element
{
public:
	void serialize(yasli::Archive& ar);
	virtual void QtDraw(QPainter& painter, float parentOffset) override;
	virtual bool RayIntersect(const vidf::Rayf& ray, Intersect* intersect, float parentOffset) override;
	virtual float GetMaxOffset(float parentOffset) const override;

private:
	std::vector<yasli::SharedPtr<Element>> elements;
};



class Renderer : public QOpenGLWidget
{
public:
	Renderer(ElementCompound* _compound);

	virtual void paintEvent(QPaintEvent* event) override;
	virtual void timerEvent(QTimerEvent* event) override;

private:
	void DrawRayIntersect(QPainter& painter, const vidf::Rayf& ray);

private:
	ElementCompound* compounds;
	QBasicTimer      timer;
};



class LensesFrame : public QMainWindow
{
	Q_OBJECT

public:
	LensesFrame();

private:
	ElementCompound elements;
	QPropertyTree*  simulatorWidget;
	Renderer*       renderWidget;
};
