#include "pch.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "vidf/platform/time.h"
#include "timeprofiler.h"


namespace vidf { namespace profiler {


	namespace
	{


		//const int bufferSize = 1024*8;
		const int bufferSize = 64;



		class TimeProfileDataStream
		{
		public:
			TimeProfileDataStream();
			~TimeProfileDataStream();

			void Write(const void* data, unsigned int size);

		private:
			void Flush();
			void Dispatcher();
			static DWORD WINAPI ThreadDispatcher(LPVOID ptr);

		//	SOCKET dataStream;
			FILE* file;
			
			unsigned char* currentBuffer;
			unsigned char* currentBufferEnd;
			unsigned char* currentBufferCursor;
			unsigned char* otherBuffer;
			unsigned int dispatchDataSize;

			HANDLE dispatcherThread;
			HANDLE dispatchEvent;
			HANDLE dispatchedEvent;
		};



		TimeProfileDataStream::TimeProfileDataStream()
		{
			currentBuffer = (unsigned char*)malloc(bufferSize);
			otherBuffer = (unsigned char*)malloc(bufferSize);
			currentBufferCursor = currentBuffer;
			currentBufferEnd = currentBuffer + bufferSize;

		/*	WSADATA wsaData;
			WSAStartup(MAKEWORD(2,2), &wsaData);
			struct addrinfo* result = NULL, *ptr = NULL, hints;
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			const char* port = "27015";
			getaddrinfo("127.0.0.1", port, &hints, &result);
			ptr=result;
			dataStream = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			int res = connect(dataStream, ptr->ai_addr, (int)ptr->ai_addrlen);*/

			file = fopen("profiler.vip", "wb");

			dispatcherThread = CreateThread(0, 0, &ThreadDispatcher, (LPVOID)this, 0, 0);
			dispatchEvent = CreateEvent(0, false, false, 0);
			dispatchedEvent = CreateEvent(0, false, true, 0);
		}



		TimeProfileDataStream::~TimeProfileDataStream()
		{
			Flush();
			std::free(currentBuffer);
			std::free(otherBuffer);
		}



		void TimeProfileDataStream::Write(const void* data, unsigned int size)
		{
			if (currentBufferCursor + size > currentBufferEnd)
				Flush();
			std::memcpy(currentBufferCursor, data, size);
			currentBufferCursor += size;
		}



		void TimeProfileDataStream::Flush()
		{
			WaitForSingleObject(dispatchedEvent, INFINITE);

			dispatchDataSize = (unsigned int)((UINT_PTR)currentBufferCursor - (UINT_PTR)currentBuffer);
			std::swap(currentBuffer, otherBuffer);
			currentBufferCursor = currentBuffer;
			currentBufferEnd = currentBuffer + bufferSize;

			SetEvent(dispatchEvent);
		}



		void TimeProfileDataStream::Dispatcher()
		{
			for (;;)
			{
				WaitForSingleObject(dispatchEvent, INFINITE);

				const char* writeBuffer = (const char*)otherBuffer;
				unsigned int writeBufferSize = dispatchDataSize;
				while (writeBufferSize)
				{
				//	int dataSent = send(dataStream, writeBuffer, writeBufferSize, 0);
				//	if (dataSent == -1)
				//		break;
					fwrite(writeBuffer, writeBufferSize, 1, file);
					fflush(file);

				//	writeBufferSize -= dataSent;
				//	writeBuffer += dataSent;
					writeBufferSize -= writeBufferSize;
					writeBuffer += writeBufferSize;
				}

				SetEvent(dispatchedEvent);
			}
		}



		DWORD WINAPI TimeProfileDataStream::ThreadDispatcher(LPVOID ptr)
		{
			((TimeProfileDataStream*)ptr)->Dispatcher();
			return 0;
		}




		class TimeProfileSystem
		{
		public:
			TimeProfileSystem();

			void RegisterFunction(const char* functionName, uint32 hash);
			void EnterFunctionCall(uint32 hash, uint64 timeStamp);
			void LeaveFunctionCall(uint32 hash, uint64 timeStamp);
			void MarkFrame(uint64 timeStamp);

			enum Commands
			{
				CMD_TPS,
				CMD_FN,
				CMD_IN,
				CMD_OUT,
				CMD_FM
			};

		private:
			TimeProfileDataStream dataStream;
		};



		TimeProfileSystem& GetTimeProfileSystem()
		{
			static TimeProfileSystem timeProfileSystem;
			return timeProfileSystem;
		}



		TimeProfileSystem::TimeProfileSystem()
		{
			struct {uint8 cmd; uint64 ticks;} cmd = {(uint8)CMD_TPS, Time::GetPlatformTicksPerSecond()};
			dataStream.Write(&cmd.cmd, sizeof(cmd.cmd));
			dataStream.Write(&cmd.ticks, sizeof(cmd.ticks));
		}



		void TimeProfileSystem::RegisterFunction(const char* functionName, uint32 hash)
		{
			struct {uint8 cmd; uint32 hash;} cmd = {(uint8)CMD_FN, hash};
			dataStream.Write(&cmd.cmd, sizeof(cmd.cmd));
			dataStream.Write(&cmd.hash, sizeof(cmd.hash));
			uint16 strLen = (uint16)std::strlen(functionName);
			dataStream.Write(&strLen, sizeof(uint16));
			dataStream.Write(functionName, strLen);
		}



		void TimeProfileSystem::EnterFunctionCall(uint32 hash, uint64 timeStamp)
		{
			struct {uint8 cmd; uint32 hash; uint64 timeStamp;} cmd = {(uint8)CMD_IN, hash, timeStamp};
			dataStream.Write(&cmd.cmd, sizeof(cmd.cmd));
			dataStream.Write(&cmd.hash, sizeof(cmd.hash));
			dataStream.Write(&cmd.timeStamp, sizeof(cmd.timeStamp));
		}



		void TimeProfileSystem::LeaveFunctionCall(uint32 hash, uint64 timeStamp)
		{
			struct {uint8 cmd; uint32 hash; uint64 timeStamp;} cmd = {(uint8)CMD_OUT, hash, timeStamp};
			dataStream.Write(&cmd.cmd, sizeof(cmd.cmd));
			dataStream.Write(&cmd.hash, sizeof(cmd.hash));
			dataStream.Write(&cmd.timeStamp, sizeof(cmd.timeStamp));
		}



		void TimeProfileSystem::MarkFrame(uint64 timeStamp)
		{
			struct {uint8 cmd; uint64 timeStamp;} cmd = {(uint8)CMD_FM, timeStamp};
			dataStream.Write(&cmd.cmd, sizeof(cmd.cmd));
			dataStream.Write(&cmd.timeStamp, sizeof(cmd.timeStamp));
		}


	}



	FunctionProfile::FunctionProfile(const char* function)
		:	functionName(function)
		,	functionNameHash((unsigned int)function)
	{
		GetTimeProfileSystem().RegisterFunction(functionName, functionNameHash);
	}



	FunctionProfileEvent::FunctionProfileEvent(const FunctionProfile& profilePoint)
		:	functionProfiler(profilePoint)
	{
		GetTimeProfileSystem().EnterFunctionCall(functionProfiler.GetHash(), GetTime().GetTicks());
	}



	FunctionProfileEvent::~FunctionProfileEvent()
	{
		GetTimeProfileSystem().LeaveFunctionCall(functionProfiler.GetHash(), GetTime().GetTicks());
	}



	void MarkFrame()
	{
		GetTimeProfileSystem().MarkFrame(GetTime().GetTicks());
	}


}}
