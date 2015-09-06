#include "pch.h"
#include "camera.h"


namespace vidf { namespace proto {



	CameraOrtho2D::CameraOrtho2D(Canvas_ptr _canvas, OrthoYAxis orthoYAxis, CameraListenerType type)
		:	canvas(_canvas)	
		,	center(zero)
		,	size(1.0f)
		,	aspectRatio(_canvas->GetCanvasDesc().width, _canvas->GetCanvasDesc().height)
		,	listener(*this)
		,	yAxis(orthoYAxis)
	{
		if (type == CameraListener_Full)
			canvas->AddListener(&listener);
	}



	CameraOrtho2D::~CameraOrtho2D()
	{
		canvas->RemoveListener(&listener);
	}



	void CameraOrtho2D::SetCamera(const Vector2f& camCenter, float camSize)
	{
		center = camCenter;
		size = camSize;
	}



	void CameraOrtho2D::CommitToGL() const
	{
		float aspect = aspectRatio.GetAsFloat();
		float sizeX = size * aspect * 0.5f;
		float sizeY = size * 0.5f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (yAxis == Upward)
		{
			gluOrtho2D(
				center.x-sizeX, center.x+sizeX,
				center.y-sizeY, center.y+sizeY);
		}
		else
		{
			gluOrtho2D(
				center.x-sizeX, center.x+sizeX,
				center.y+sizeY, center.y-sizeY);
		}
		glMatrixMode(GL_MODELVIEW);
	}



	Vector2f CameraOrtho2D::CursorPosition() const
	{
		POINT cursorPos;
		RECT clientRect;
		::GetCursorPos(&cursorPos);
		::ScreenToClient((HWND)canvas->GetHandle(), &cursorPos);
		::GetClientRect((HWND)canvas->GetHandle(), &clientRect);

		if (yAxis == Downward)
			std::swap(clientRect.bottom, clientRect.top);

		Vector2f result = Vector2f(
			1.0f-(cursorPos.x-(float)clientRect.left)/(clientRect.right-(float)clientRect.left),
			(cursorPos.y-(float)clientRect.top)/(clientRect.bottom-(float)clientRect.top));

		result = result * 2.0f - Vector2f(1.0f, 1.0f);
		result = center - result * Vector2f(size*0.5f*aspectRatio.GetAsFloat(), size*0.5f);

		return result;
	}




	CameraOrtho2D::Listerner::Listerner( CameraOrtho2D& _camera )
		:	camera(_camera)
		,	panning(false)
	{
	}



	void CameraOrtho2D::Listerner::RightMouseDown(Vector2i /*point*/)
	{
		panning = true;
	}



	void CameraOrtho2D::Listerner::RightMouseUp(Vector2i /*point*/)
	{
		panning = false;
	}



	void CameraOrtho2D::Listerner::MouseMove(Vector2i point)
	{
		Vector2i delta = lastPoint - point;
		if (camera.yAxis == Upward)
			delta.y = -delta.y;
		if (panning)
		{
			Vector2f cameraPosition = camera.GetCenter();
			float cameraSize = camera.GetSize();
			int windowSize = camera.GetCanvas()->GetCanvasDesc().height;

			cameraPosition = cameraPosition + Vector2f(delta)*(cameraSize/windowSize);

			camera.SetCamera(cameraPosition, cameraSize);
		}
		lastPoint = point;
	}



	void CameraOrtho2D::Listerner::MouseWheel(float delta)
	{
		Vector2f cameraPosition = camera.GetCenter();
		float cameraSize = camera.GetSize();

		const float zoomSpeed = 0.9f;
		if (delta > 0)
			cameraSize *= zoomSpeed;
		else
			cameraSize /= zoomSpeed;

		camera.SetCamera(cameraPosition, cameraSize);
	}




	OrbitalCamera::OrbitalCamera(Canvas_ptr _canvas, CameraListenerType type)
		:	canvas(_canvas)	
		,	listener(*this)
		,	target(zero)
		,	rotation(zero)
		,	aspectRatio(_canvas->GetCanvasDesc().width, _canvas->GetCanvasDesc().height)
		,	fov(1.4f)
		,	distance(10.0f)
		,	nearPlane(0.1f)
		,	farPlane(100.0f)
	{
		if (type == CameraListener_Full)
			canvas->AddListener(&listener);
	}



	OrbitalCamera::~OrbitalCamera()
	{
		canvas->RemoveListener(&listener);
	}



	void OrbitalCamera::SetPerspective(float _radFoV, float _nearPlane, float _farPlane)
	{
		fov = _radFoV;
		nearPlane = _nearPlane;
		farPlane = _farPlane;
	}



	void OrbitalCamera::SetCamera(const Vector3f& _target, const Quaternionf& _rotation, float _distance)
	{
		target = _target;
		rotation = _rotation;
		distance = _distance;
	}



	void OrbitalCamera::Update(Time deltaTime)
	{
		if (listener.GetCameraMode() != Listerner::FREE)
			return;
		Vector3f front = Front();
		Vector3f side = Side();

		Vector3f moveVector(zero);

		if (GetAsyncKeyState('W') & 0x8000)
			moveVector = moveVector + front;
		if (GetAsyncKeyState('S') & 0x8000)
			moveVector = moveVector - front;
		if (GetAsyncKeyState('A') & 0x8000)
			moveVector = moveVector - side;
		if (GetAsyncKeyState('D') & 0x8000)
			moveVector = moveVector + side;

		if (moveVector != Vector3f(zero))
			moveVector = Normalize(moveVector);
		target = target + moveVector * distance * 0.25f * deltaTime.AsFloat();
	}



	void OrbitalCamera::CommitToGL() const
	{
		glMatrixMode(GL_PROJECTION);
		Matrix44f perspective = PerspectiveMatrix();
		glLoadMatrixf(&perspective.m00);
		glMatrixMode(GL_MODELVIEW);

		Matrix44f view = ViewMatrix();
		glLoadMatrixf(&view.m00);
	}



	Vector3f OrbitalCamera::Position() const
	{
		return target - Front() * distance;
	}



	Matrix44f OrbitalCamera::PerspectiveMatrix() const
	{
		return PerspectiveFovRH(fov, aspectRatio.GetAsFloat(), nearPlane, farPlane);
	}



	Matrix44f OrbitalCamera::ViewMatrix() const
	{
		Vector3f up = Up();
		Vector3f position = Position();
		return LookAtRH(position, target, up);
	}



	OrbitalCamera::Listerner::Listerner(OrbitalCamera& _camera)
		:	camera(_camera)
		,	cameraMode(NONE)
	{
	}



	void OrbitalCamera::Listerner::LeftMouseDown(Vector2i /*point*/)
	{
		cameraMode = FREE;
	}



	void OrbitalCamera::Listerner::LeftMouseUp(Vector2i /*point*/)
	{
		if (cameraMode == FREE)
			cameraMode = NONE;
	}



	void OrbitalCamera::Listerner::MiddleMouseDown(Vector2i /*point*/)
	{
		cameraMode = PAN;
	}



	void OrbitalCamera::Listerner::MiddleMouseUp(Vector2i /*point*/)
	{
		if (cameraMode == PAN)
			cameraMode = NONE;
	}



	void OrbitalCamera::Listerner::RightMouseDown(Vector2i /*point*/)
	{
		cameraMode = ROTATE;
	}



	void OrbitalCamera::Listerner::RightMouseUp(Vector2i /*point*/)
	{
		if (cameraMode == ROTATE)
			cameraMode = NONE;
	}



	void OrbitalCamera::Listerner::MouseMove(Vector2i point)
	{
		Vector2i delta = lastPoint - point;
		// delta.y = -delta.y;

		Vector3f target = camera.GetTarget();
		Quaternionf rotation = camera.GetRotation();
		float distance = camera.GetDistance();

		Vector3f up = camera.Up();
		Vector3f side = camera.Side();

		if (cameraMode == ROTATE || cameraMode == FREE)
		{
			Quaternionf deltaX = QuaternionAxisAngle(up, delta.x * 0.01f);
			Quaternionf deltaY = QuaternionAxisAngle(side, delta.y * 0.01f);
			Quaternionf deltaRotation = deltaX * deltaY;

			rotation = deltaRotation * rotation;
			if (cameraMode == FREE)
			{
				Vector3f position = camera.Position();
				target = Rotate(deltaRotation, target-position) + position;
			}
		}
		else if (cameraMode == PAN)
		{
			target = target + side*delta.x*distance*0.005f;
			target = target + up*delta.y*distance*0.005f;
		}

		camera.SetCamera(target, rotation, distance);

		lastPoint = point;
	}



	void OrbitalCamera::Listerner::MouseWheel(float delta)
	{
		Vector3f front = camera.Front();
		Vector3f target = camera.GetTarget();
		Quaternionf rotation = camera.GetRotation();
		float distance = camera.GetDistance();
		float newDistance = distance;

		const float zoomSpeed = 0.9f;
		if (delta > 0)
			newDistance *= zoomSpeed;
		else
			newDistance /= zoomSpeed;

		if (cameraMode == FREE)
			target = target - front * (distance-newDistance);

		camera.SetCamera(target, rotation, newDistance);
	}


} }
