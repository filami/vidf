#include "pch.h"


bool TestVulkan();
void TestGM();
void Voxelizer();


int main()
{
	vidf::SetCurrentDirectory("../../");
	TestVulkan();
	// TestGM();
	// Voxelizer();
	return 0;
}
