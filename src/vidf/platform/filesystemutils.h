#pragma once


namespace vidf
{



	inline bool IsPathDivisor(int character)
	{
		return character == '/' || character == '\\';
	}



	inline std::string GetPathFromFilePath(const std::string& filePath)
	{
		if (filePath.empty())
			return std::string();

		size_t i;
		for (i = filePath.size()-1; !IsPathDivisor((int)filePath[i]) && i > 0; --i);

		return filePath.substr(0, i+1);
	}



	template<typename CHAR>
	inline std::basic_string<CHAR> StandardPath(const CHAR* filePath)
	{
		std::basic_string<CHAR> result = filePath;
		for (auto& character : result)
		{
			if (character == '\\')
				character = '/';
		}
		return result;
	}


}
