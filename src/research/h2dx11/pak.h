#pragma once

#include <unordered_map>


namespace h2
{


	class FileManager
	{
	public:
		typedef std::shared_ptr<std::ifstream> PakFileHandle;

	private:
		struct PakfileRef
		{
			PakFileHandle handle;
			vidf::uint    offset;
		};

	public:
		void          AddPak(const char* pakPath);
		PakFileHandle OpenFile(const char* fileName);

	private:
		std::unordered_map<std::string, PakfileRef> files;
	};



}
