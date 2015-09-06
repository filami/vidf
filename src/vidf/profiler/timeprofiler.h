#pragma once


namespace vidf
{
#if 0

	namespace profiler
	{


		class FunctionProfile
		{
		public:
			FunctionProfile(const char* function);
			uint32 GetHash() const {return functionNameHash;}
		private:
			const char* functionName;
			uint32 functionNameHash;
		};


		class FunctionProfileEvent
		{
		public:
			FunctionProfileEvent(const FunctionProfile& profilePoint);
			~FunctionProfileEvent();

		private:
			const FunctionProfile& functionProfiler;
		};


		void MarkFrame();


	}


#	define VIDF_PROFILE_FUNCTION													\
		static vidf::profiler::FunctionProfile __thisFnProfPoint(__FUNCTION__);		\
		vidf::profiler::FunctionProfileEvent __thisFnProfEvent(__thisFnProfPoint);
#	define VIDF_PROFILE_FRAME														\
		vidf::profiler::MarkFrame();

#endif

#	define VIDF_PROFILE_FUNCTION
#	define VIDF_PROFILE_FRAME

}
