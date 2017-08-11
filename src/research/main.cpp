#include "pch.h"


void TestDx11();
bool TestVulkan();
void TestGM();
void Voxelizer();
void Gomoku();


int main()
{
	vidf::SetCurrentDirectory("../../");
	TestDx11();
	// TestVulkan();
	// TestGM();
	// Voxelizer();
	// Gomoku();
	return 0;
}
