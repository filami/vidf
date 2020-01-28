#include <vidf/pch.h>
#include <vidf/platform/system.h>


void TestDx11();
void TestDx12();
bool TestVulkan();
void TestGM();
void Voxelizer();
void Voxelizer2();
void Voxelizer3();
void Gomoku();
void H2Dx11();
void H2Dx12();
void Stuff();
void PathTracer();
void Lasers();
void Synth();
void Platform();
void Fractals();
void KC();


int main()
{
	vidf::SetCurrentDirectory("../../");
	// TestDx11();
	// TestDx12();
	// TestVulkan();
	// TestGM();
	// Voxelizer();
	// Voxelizer2();
	// Voxelizer3();
	// Gomoku();
	// H2Dx11();
	H2Dx12();
	// Stuff();
	// PathTracer();
	// Lasers();
	// Synth();
	// Platform();
	// Fractals();
	// KC();
	return 0;
}
