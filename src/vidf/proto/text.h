#pragma once

#include "Protogl.h"
#include "vidf/common/vector3.h"


namespace vidf { namespace proto {



	const int numChars = 96;
	const int firstChar = 32;

	class Text
	{
	public:
		Text();
		~Text();

		void Init(int size=16);
		void SetGLOrtho2D();
		void OutputText(const Vector2f& position, const char* text, ...) const;
		void OutputText(const Vector3f& position, const char* text, ...) const;

		GLuint texture;
		float size;
		bool flip;

	private:
		GLuint base;
		Rectf rects[numChars];
		float aspect;
		float scale;
	};


} }
