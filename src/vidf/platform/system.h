#pragma once


namespace vidf
{


	enum SystemMessageResult
	{
		SMR_Continue,
		SMR_PostQuit,
	};


# ifdef WIN32

	enum KeyCode
	{
		KC_ESCAPE = VK_ESCAPE,
		KC_RETURN = VK_RETURN,
		// ... //
	};

#endif



	SystemMessageResult UpdateSystemMessages();


}
