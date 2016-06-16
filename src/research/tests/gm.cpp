#include "pch.h"
#include "gmMachine.h"
#include "gmArrayLib.h"
#include "gmCall.h"
#include "gmGCRoot.h"
#include "gmGCRootUtil.h"
#include "gmHelpers.h"
#include "gmMathLib.h"
#include "gmStringLib.h"
#include "gmSystemLib.h"
#include "gmVector3Lib.h"


void TestGM()
{
	gmMachine machine;

	gmBindArrayLib(&machine);
	gmBindMathLib(&machine);
	gmBindStringLib(&machine);
	gmBindSystemLib(&machine);
	gmBindVector3Lib(&machine);

	std::string cmd;
	for (;;)
	{
		std::cout << "> ";
		std::getline(std::cin, cmd);
		if (cmd == "quit")
			break;

		machine.ExecuteString(cmd.c_str());

		bool first = true;
		while (const char* log = machine.GetLog().GetEntry(first))
			std::cout << log;
		machine.GetLog().Reset();
	}
}
