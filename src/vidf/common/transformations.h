#pragma once

#include "vector3.h"
#include "matrix44.h"

namespace vidf
{



	template<typename T>
	Matrix44<T> Translate(const Vector3<T>& offset)
	{
		Matrix44<T> result(zero);
		result.m30 = offset.x;
		result.m31 = offset.y;
		result.m32 = offset.z;
		return result;
	}



	template<typename T>
	Matrix44<T> Scale(T scale)
	{
		Matrix44<T> result(zero);
		result.m00 = scale;
		result.m11 = scale;
		result.m22 = scale;
		return result;
	}



	template<typename T>
	Matrix44<T> NonUniformScale(const Vector3<T>& scale)
	{
		Matrix44<T> result(zero);
		result.m00 = scale.x;
		result.m11 = scale.y;
		result.m22 = scale.z;
		return result;
	}




	template<typename T>
	Matrix44<T> PerspectiveFovLH(T fov, T aspect, T nearPlane, T farPlane)
	{
		Matrix44<T> result(zero);
		T yScale = cotg(fov * (T)0.5);
		T xscale = yScale / aspect;
		result.m00 = xscale;
		result.m11 = yScale;
		result.m22 = farPlane / (farPlane - nearPlane);
		result.m23 = (T)1;
		result.m32 = -nearPlane*farPlane/(farPlane - nearPlane);
		result.m33 = 0.0f;
		return result;
	}

	template<typename T>
	Matrix44<T> PerspectiveFovRH(T fov, T aspect, T nearPlane, T farPlane)
	{
		Matrix44<T> result(zero);
		T yScale = cotg(fov * (T)0.5);
		T xscale = yScale / aspect;
		result.m00 = xscale;
		result.m11 = yScale;
		result.m22 = farPlane / (nearPlane - farPlane);
		result.m23 = (T)-1;
		result.m32 = nearPlane*farPlane/(nearPlane - farPlane);
		result.m33 = 0.0f;
		return result;
	}




	template<typename T>
	Matrix44<T> LookAtLH(const Vector3<T>& eye, const Vector3<T>& at, const Vector3<T>& up)
	{
		Matrix44<T> result;
		Vector3<T> zaxis = Normalize(at - eye);
		Vector3<T> xaxis = Normalize(Cross(up, zaxis));
		Vector3<T> yaxis = Cross(zaxis, xaxis);
		result.m00=xaxis.x; result.m01=yaxis.x; result.m02=zaxis.x; result.m03=0;
		result.m10=xaxis.y; result.m11=yaxis.y; result.m12=zaxis.y; result.m13=0;
		result.m20=xaxis.z; result.m21=yaxis.z; result.m22=zaxis.z; result.m23=0;
		result.m30=-Dot(xaxis,eye); result.m31=-Dot(yaxis,eye); result.m32=-Dot(zaxis,eye); result.m33=(T)1;
		return result;
	}

	template<typename T>
	Matrix44<T> LookAtRH(const Vector3<T>& eye, const Vector3<T>& at, const Vector3<T>& up)
	{
		Matrix44<T> result;
		Vector3<T> zaxis = Normalize(eye - at);
		Vector3<T> xaxis = Normalize(Cross(up, zaxis));
		Vector3<T> yaxis = Cross(zaxis, xaxis);
		result.m00=xaxis.x; result.m01=yaxis.x; result.m02=zaxis.x; result.m03=0;
		result.m10=xaxis.y; result.m11=yaxis.y; result.m12=zaxis.y; result.m13=0;
		result.m20=xaxis.z; result.m21=yaxis.z; result.m22=zaxis.z; result.m23=0;
		result.m30=-Dot(xaxis,eye); result.m31=-Dot(yaxis,eye); result.m32=-Dot(zaxis,eye); result.m33=(T)1;
		return result;
	}



}
