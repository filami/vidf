#pragma once

namespace vidf
{



inline string ToString(const wchar_t* str)
{
	string out;
	while (*str != 0)
		out.push_back(char((*str++)&0xff));
	return out;
}



inline wstring ToWString(const char* str)
{
	wstring out;
	while (*str != 0)
		out.push_back(wchar_t(*str++));
	return out;
}



template<typename TChar>
inline basic_string<TChar> ToLower(const TChar* str)
{
	basic_string<TChar> out;
	while (*str != 0)
		out.push_back(std::tolower(*str++));
	return out;
}



template<typename T>
inline string to_string(T v)
{
	return std::to_string(v);
}



inline string to_string(const char* str)
{
	return str;
}



inline string to_string(const string& str)
{
	return str;
}



inline string Format(const char* format)
{
	return format;
}



inline string Format(const char* format, const deque<string>& elements)
{
	string result;
	result.reserve(strlen(format));
	for (; *format != 0; ++format)
	{
		if (*format != '%')
		{
			result.push_back(*format);
			continue;
		}
		format++;
		if (*format == 0)
		{
			result.push_back('%');
			break;
		}
		if (*format == '%')
		{
			result.push_back('%');
			continue;
		}
		int index = 0;
		for (; *format != '0' && isdigit(*format); ++format)
			index = index * 10 + (*format - '0');
		if (*format != 0)
			result.push_back(*format);
		if (index < elements.size())
			result += elements[index];
	}
	return result;
}



template<typename Car, typename... Cdr>
inline string Format(const char* format, deque<string>& elements, Car car, Cdr ...cdr)
{
	elements.push_back(to_string(car));
	return Format(format, elements, cdr...);
}



template<typename Car, typename... Cdr>
inline string Format(const char* format, Car car, Cdr ...cdr)
{
	deque<string> elements;
	elements.push_back(to_string(car));
	return Format(format, elements, cdr...);
}



}
