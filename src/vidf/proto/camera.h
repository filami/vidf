#pragma once

#include "vidf/common/aspectratio.h"
#include "vidf/common/vector2.h"
#include "vidf/common/vector3.h"
#include "vidf/common/quaternion.h"
#include "vidf/common/transformations.h"
#include "vidf/platform/time.h"
#include "vidf/platform/canvas.h"


namespace vidf { namespace proto {


	enum CameraListenerType
	{
		CameraListener_None,
		CameraListener_Full,
	};


	class CameraOrtho2D
	{
	private:

		class Listerner : public CanvasListener
		{
		public:
			explicit Listerner(CameraOrtho2D& _camera);

		private:
			Listerner& operator= (const Listerner&) {}

			virtual void RightMouseDown(Vector2i point);
			virtual void RightMouseUp(Vector2i point);
			virtual void MouseMove(Vector2i point);
			virtual void MouseWheel(float delta);

			CameraOrtho2D& camera;
			Vector2i lastPoint;
			bool panning;
		};

	public:

		enum OrthoYAxis
		{
			Upward,
			Downward,
		};

	public:
		CameraOrtho2D(CanvasPtr _canvas, OrthoYAxis orthoYAxis=Upward, CameraListenerType type=CameraListener_Full);
		~CameraOrtho2D();

		void SetCamera(const Vector2f& camCenter, float camSize, float _angle = 0.0f);
		CanvasPtr GetCanvas() const {return canvas;}

		void CommitToGL() const;

		const Vector2f& GetCenter() const {return center;}
		float GetSize() const {return size;}
		Vector2f CursorPosition() const;
		AspectRatio GetAspectRation() const {return aspectRatio;}

	private:
		Listerner listener;
		CanvasPtr canvas;
		AspectRatio aspectRatio;

		OrthoYAxis yAxis;
		Vector2f center;
		float size;
		float angle = 0.0f;
	};



	class OrbitalCamera
	{
	private:
		class Listerner : public CanvasListener
		{
		public:
			enum CameraMode
			{
				NONE,
				ROTATE,
				PAN,
				FREE
			};

		public:
			Listerner(OrbitalCamera& _camera);

			CameraMode GetCameraMode() const {return cameraMode;}

		private:
			Listerner& operator=(const Listerner&) {}

			virtual void LeftMouseDown(Vector2i point);
			virtual void LeftMouseUp(Vector2i point);
			virtual void MiddleMouseDown(Vector2i point);
			virtual void MiddleMouseUp(Vector2i point);
			virtual void RightMouseDown(Vector2i point);
			virtual void RightMouseUp(Vector2i point);
			virtual void MouseMove(Vector2i point);
			virtual void MouseWheel(float delta);

			OrbitalCamera& camera;
			Vector2i lastPoint;
			CameraMode cameraMode;
		};

	public:
		OrbitalCamera(CanvasPtr _canvas, CameraListenerType type=CameraListener_Full);
		~OrbitalCamera();

		CanvasPtr GetCanvas() const {return canvas;}

		void SetPerspective(float _radFoV, float _nearPlane, float _farPlane);
		void SetCamera(const Vector3f& _target, const Quaternionf& _rotation, float _distance);
		void Update(Time deltaTime);

		void CommitToGL() const;

		void SetTarget(const Vector3f& _target) {target = _target;}
		void SetDistance(float _distance) {distance = _distance;}

		const Vector3f& GetTarget() const {return target;}
		const Quaternionf GetRotation() const {return rotation;}
		const float GetDistance() const {return distance;}
		const float GetNearPlane() const {return nearPlane;}
		const float GetFarPlane() const {return farPlane;}

		static const Vector3f FrontVector() {return Vector3f(1, 0, 0);}
		static const Vector3f SideVector() {return Vector3f(0, -1, 0);}
		static const Vector3f UpVector() {return Vector3f(0, 0, 1);}

		Vector3f Position() const;
		Vector3f Front() const {return Rotate(rotation, FrontVector());}
		Vector3f Side() const {return Rotate(rotation, SideVector());}
		Vector3f Up() const {return Rotate(rotation, UpVector());}
		Matrix44f PerspectiveMatrix() const;
		Matrix44f ViewMatrix() const;

	private:
		Listerner listener;
		CanvasPtr canvas;
		AspectRatio aspectRatio;

		Quaternionf rotation;
		Vector3f target;
		float fov;
		float distance;
		float nearPlane;
		float farPlane;
	};


} }
