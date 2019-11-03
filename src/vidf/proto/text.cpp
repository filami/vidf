#include "pch.h"
#include "text.h"

namespace vidf { namespace proto {



	Text::Text()
		:	base(0)
		,	texture(0)
		,	size(4.0f)
		,	flip(false)
	{
	}



	Text::~Text()
	{
		glDeleteLists(base, 96);
	}



	void Text::Init(int size)
	{
		if (!texture)
		{
			HFONT font;

			size = 256;

			font = CreateFont(
				size, 0, 0, 0, FW_BOLD, false, false, false,
				ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH,
			//	TEXT("Arial"));
				TEXT("Courier New"));

			HDC screenDC = ::GetDC(::GetDesktopWindow());
			HDC bitmapDC = ::CreateCompatibleDC(screenDC);
			SelectObject(bitmapDC, font);

			int width = 2048;
			int height = size;

			WCHAR text[numChars];
			for (int i = 0; i < numChars; ++i)
				text[i] = WCHAR(i + firstChar);
			INT widths[numChars];
			GetCharWidth32(bitmapDC, firstChar, firstChar+numChars-1, widths);
			int x = 0;
			int y = 0;
			scale = 1.0f;
			for (int i = 0; i < numChars; ++i)
			{
				if (x + widths[i] > width)
				{
					x = 0;
					y += size;
				}
				if (y + size > height)
				{
					scale *= 2.0f;
					height *= 2;
				}
				x += widths[i];
			}

			BITMAPINFO canvas;
			ZeroMemory(&canvas, sizeof(canvas));
			canvas.bmiHeader.biSize = sizeof(canvas.bmiHeader);
			canvas.bmiHeader.biWidth = width;
			canvas.bmiHeader.biHeight = height;
			canvas.bmiHeader.biPlanes = 1;
			canvas.bmiHeader.biBitCount = 32;
			HBITMAP compBitmap = CreateDIBSection(bitmapDC, &canvas, DIB_RGB_COLORS, 0, 0, 0); 
			SelectObject(bitmapDC, compBitmap);

			RECT rect;
			ZeroMemory(&rect, sizeof(rect));
			rect.right = width;
			rect.bottom = height;
			COLORREF backgroundCl = RGB(0, 0, 0);
			COLORREF foregroundCl = RGB(255, 255, 255);
			HBRUSH background = ::CreateSolidBrush(backgroundCl);
			FillRect(bitmapDC, &rect, background);
			SetTextColor(bitmapDC, foregroundCl);
			SetBkMode(bitmapDC, TRANSPARENT);

			x = 0;
			int chars = 0;
			WCHAR curChar = firstChar;
			for (int i = 0; i < numChars; ++i)
			{
				if (x + widths[i] > width)
				{
					DrawTextW(bitmapDC, text, chars, &rect, DT_LEFT|DT_NOCLIP);
					rect.left = 0;
					rect.top += size;
					chars = 0;
					x = 0;
				}
				rects[i].min.x = x / float(width);
				rects[i].min.y = 1.0f - (rect.top / float(height));
				rects[i].max.x = (x + widths[i]) / float(width);
				rects[i].max.y = 1.0f - ((rect.top + size) / float(height));
				text[chars++] = curChar++;
				x += widths[i];
			}
			DrawTextW(bitmapDC, text, chars, &rect, DT_LEFT|DT_NOCLIP);

			BITMAPINFOHEADER bi;
			ZeroMemory(&bi, sizeof(bi));
			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biWidth = width;
			bi.biHeight = height;
			bi.biPlanes = 1;
			bi.biBitCount = 32;
			bi.biCompression = BI_RGB;
			LONG bufferSz = width*height*4;
			LPVOID buffer = operator new(bufferSz);
			GetDIBits(
				bitmapDC, compBitmap,
				0, (UINT)height,
				buffer,
				(BITMAPINFO*)&bi, DIB_RGB_COLORS);
			for (int i = 0; i < bufferSz; i+=4)
			{
				unsigned char* ptr = ((unsigned char*)buffer)+i;
				*(ptr+3) = *(ptr+0);
				*(ptr+0) = 0xff;
				*(ptr+1) = 0xff;
				*(ptr+2) = 0xff;
			}

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

			operator delete (buffer);

			aspect = width / float(height);
		}
	}



	void Text::SetGLOrtho2D()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0f, 1.0f, 1.0f, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}



	void Text::OutputText(const Vector2f& position, const char* text, ...) const
	{
		const int bufferSize = 256;
		char buffer[bufferSize];
		va_list ap;

		if (text == 0)
			return;	

		va_start(ap, text);
		vsprintf_s<bufferSize>(buffer, text, ap);
		va_end(ap);

		if (!texture)
		{
			glRasterPos2f(position.x, position.y);

			glPushAttrib(GL_LIST_BIT);
			glListBase(base - 32);
			glCallLists(GLsizei(std::strlen(buffer)), GL_UNSIGNED_BYTE, buffer);
			glPopAttrib();
		}
		else
		{
			glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			Vector2f p = Vector2f(position.x, position.y);
			size_t strLen = std::strlen(buffer);
			glBegin(GL_QUADS);
			for (int i = 0; i < strLen; ++i)
			{
				int charIdx = buffer[i]-firstChar;

				if (charIdx < 0 || charIdx >= numChars)
				{
					if (text[i] == '\n')
					{
						p.x = position.x;
						p.y += size;
					}
				}
				else
				{
					Rectf rect = rects[charIdx];
					Vector2f sz = Vector2f((rect.max.x-rect.min.x)*aspect, rect.max.y-rect.min.y)*scale*size;
					if (flip)
						Swap(rect.min.y, rect.max.y);

					glTexCoord2f(rect.min.x, rect.max.y);
					glVertex2f(p.x, p.y);

					glTexCoord2f(rect.max.x, rect.max.y);
					glVertex2f(p.x+sz.x, p.y);

					glTexCoord2f(rect.max.x, rect.min.y);
					glVertex2f(p.x+sz.x, p.y-sz.y);

					glTexCoord2f(rect.min.x, rect.min.y);
					glVertex2f(p.x, p.y-sz.y);

					p.x += sz.x;
				}
			}
			glEnd();

			glPopAttrib();
		}
	}



	void Text::OutputText(const Vector3f& position, const char* text, ...) const
	{
		const int bufferSize = 256;
		char buffer[bufferSize];
		va_list ap;

		if (text == 0)
			return;	

		va_start(ap, text);
		vsprintf_s<bufferSize>(buffer, text, ap);
		va_end(ap);

		OutputText(Vector2f(position.x, position.y), buffer);
	}



	Vector2f Text::CalculateSize(const char* text) const
	{
		const char* buffer = text;

		const size_t strLen = std::strlen(text);
		if (strLen == 0)
			return Vector2f{ zero };
		
		float maxWidth = 0;
		float lineWidth = 0;
		float height = size;

		for (int i = 0; i < strLen; ++i)
		{
			int charIdx = buffer[i] - firstChar;

			if (charIdx < 0 || charIdx >= numChars)
			{
				if (text[i] == '\n')
				{
					lineWidth = 0;
					height += size;
				}
			}
			else
			{
				Rectf rect = rects[charIdx];
				lineWidth += (rect.max.x - rect.min.x) * aspect * scale * size;
				maxWidth = max(maxWidth, lineWidth);
			}
		}

		return Vector2f{ maxWidth , height };
	}


} }
