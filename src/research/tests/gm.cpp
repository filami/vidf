#include "pch.h"
#include "gmMachine.h"

void TestGM()
{
	gmMachine machine;

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
