#pragma once


namespace vidf
{


	enum class SystemMessageResult
	{
		Continue,
		Quit,
	};


# ifdef WIN32

	enum class KeyCode
	{
		Escape = VK_ESCAPE,
		Return = VK_RETURN,
		// ... //
	};

#endif


	void PostQuitMessage();
	SystemMessageResult UpdateSystemMessages();


}
