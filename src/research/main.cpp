#include "pch.h"


bool TestVulkan();
void TestGM();
void Voxelizer();
void Gomoku();


int main()
{
	vidf::SetCurrentDirectory("../../");
	TestVulkan();
	// TestGM();
	// Voxelizer();
	// Gomoku();
	return 0;
}
