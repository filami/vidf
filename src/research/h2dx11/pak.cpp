#include "pch.h"
#include "pak.h"
#include "stream.h"

namespace h2
{

	using namespace vidf;


	struct PakHeader
	{
		uint32 signature = 0;
		uint32 offset = 0;
		uint32 length = 0;
	};



	struct PakFile
	{
		char   fileName[56];
		uint32 position;
		uint32 length;
	};



	template<typename TStream>
	StreamResult Stream(TStream& stream, PakHeader& header)
	{
		Stream(stream, header.signature);
		if (header.signature != 0x4b434150)
			return StreamResult::Fail;
		Stream(stream, header.offset);
		Stream(stream, header.length);
		return StreamResult::Ok;
	}



	template<typename TStream>
	StreamResult Stream(TStream& stream, PakFile& dir)
	{
		Stream(stream, dir.fileName);
		Stream(stream, dir.position);
		Stream(stream, dir.length);
		return StreamResult::Ok;
	}



	void FileManager::AddPak(const char* pakPath)
	{
		PakFileHandle fileHandle = std::make_shared<std::ifstream>(pakPath, std::ios::binary);
		if (!*fileHandle)
			return;

		PakHeader header;
		if (Stream(*fileHandle, header) != StreamResult::Ok)
			return;
		const uint numFiles = header.length / sizeof(PakFile);
		fileHandle->seekg(header.offset);
		for (uint i = 0; i < numFiles; ++i)
		{
			PakFile directory;
			Stream(*fileHandle, directory);
			files[ToLower(directory.fileName)] = PakfileRef{ fileHandle, directory.position };
		}
	}



	FileManager::PakFileHandle FileManager::OpenFile(const char* fileName)
	{
		auto it = files.find(ToLower(fileName));
		if (it == files.end())
			return PakFileHandle();
		it->second.handle->seekg(it->second.offset);
		return it->second.handle;
	}



}
