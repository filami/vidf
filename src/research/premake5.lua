rule "Flex"
	display "Flex++"
	fileextension ".l"
	buildmessage  '%(Filename)%(Extension)'
	buildcommands ('cd %(RootDir)%(Directory) && '.._WORKING_DIR..'/tools/flex++.exe -o"%(Filename).cpp" -h"%(Filename).h" "%(Filename)%(Extension)"')
	buildoutputs  '%(RootDir)%(Directory)%(Filename).cpp'

rule "Bison"
	display "Bison++"
	fileextension ".y"
	buildmessage  '%(Filename)%(Extension)'
	buildcommands ('cd %(RootDir)%(Directory) && '.._WORKING_DIR..'/tools/bison++.exe -dv -o"%(Filename).cpp" -h"%(Filename).h" "%(Filename)%(Extension)"')
	buildoutputs  '%(RootDir)%(Directory)%(Filename).cpp'


CreateVIDFLibProject("Research", "pch", nil, "5117F2A9-B4A2-400C-AD7B-9C560B85BF99")

AddStaticLibLink("vidf")


AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
	
	["common"] =
	{
		"pch.h",
		"pch.cpp",
	},

	["RendererDx12"] =
	{
		"rendererdx12/common.h",
		"rendererdx12/d3dx12.h",
		"rendererdx12/descriptorheap.h",
		"rendererdx12/descriptorheap.cpp",
		"rendererdx12/pipeline.h",
		"rendererdx12/pipeline.cpp",
		"rendererdx12/rendercontext.h",
		"rendererdx12/rendercontext.cpp",
		"rendererdx12/renderdevice.h",
		"rendererdx12/renderdevice.cpp",
		"rendererdx12/resources.h",
		"rendererdx12/resources.cpp",
		"rendererdx12/shadermanager.h",
		"rendererdx12/shadermanager.cpp",
	},
	
	["tests"] =
	{
		"tests/dx11.cpp",
		"tests/dx12.cpp",
		-- "tests/vulkan.cpp",
		-- "tests/gm.cpp",
		"tests/stuff.cpp",
		"tests/synth.cpp",
		"tests/voxelizer.cpp",
		"tests/voxelizer2.cpp",
		"tests/voxelizer3.cpp",
	},
	
	["Gomoku"] =
	{
		"gomoku/game.cpp",
	},

	["renderpasses"] =
	{
		"renderpasses/averagebrightness.h",
		"renderpasses/averagebrightness.cpp",
		"renderpasses/utilitypasses.h",
		"renderpasses/utilitypasses.cpp",
	},

	["h2dx11"] =
	{
		"h2dx11/bih.h",
		"h2dx11/bih.cpp",
		"h2dx11/h2dx11.cpp",
		"h2dx11/h2dx12.cpp",
		"h2dx11/bsptexture.h",
		"h2dx11/bsptextureimpl.h",
		"h2dx11/pak.h",
		"h2dx11/pak.cpp",
		"h2dx11/kdtreeaccel.h",
		"h2dx11/kdtreeaccel.cpp",
		"h2dx11/renderpass.h",
		"h2dx11/renderpass.cpp",
		"h2dx11/stream.h",
	},

	["PathTracer"] =
	{
		"pathtracer/pathtracer.cpp",
		"pathtracer/kdtree.h",
		"pathtracer/kdtree.cpp",
		"pathtracer/halton.h",
		"pathtracer/halton.cpp",
		"pathtracer/powitacq_rgb.h",
		"pathtracer/powitacq_rgb.inl",
	},

	["Platform"] =
	{
		"platform/platform.cpp",
	},
	["Platform/WorldRender"] =
	{
		"platform/worldrender/voxel.h",
		"platform/worldrender/voxel.cpp",
		"platform/worldrender/voxelmodelrenderer.h",
		"platform/worldrender/voxelmodelrenderer.cpp",
		"platform/worldrender/worldrender.h",
		"platform/worldrender/worldrender.cpp",
	},
	["Platform/MagicaVoxel"] =
	{
		"platform/MagicaVoxel/vox.h",
		"platform/MagicaVoxel/vox.cpp",
	},

	["Lasers"] =
	{
		"lasers/lasers.cpp",
		"lasers/gui.cpp",
		"lasers/gui.h",
		"lasers/gamestate.cpp",
		"lasers/gamestate.h",
		"lasers/game.cpp",
		"lasers/game.h",
	},

	["Fractals"] =
	{
		"fractals/fractals.cpp",
		"fractals/flamegpu.cpp",
		"fractals/kaleidoscopeifs.cpp",
		"fractals/common.h",
		"fractals/common.cpp",
	},

	["KC"] =
	{
		"kc/kcscanner.l",
		"kc/kcscanner.h",
		"kc/kcscanner.cpp",
		"kc/kcparser.y",
		"kc/kcparser.h",
		"kc/kcparser.cpp",
		"kc/kc.cpp",
		"kc/kc.h",
		"kc/kcparser.output",
	},
}

rules { "Flex", "Bison" }
