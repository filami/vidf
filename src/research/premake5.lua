CreateVIDFAppProject("Research", "pch", nil, "5117F2A9-B4A2-400C-AD7B-9C560B85BF99")


AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
	
	["common"] =
	{
		"main.cpp",
		"pch.h",
		"pch.cpp",
	},
	
	["tests"] =
	{
		"tests/dx11.cpp",
		"tests/vulkan.cpp",
		-- "tests/gm.cpp",
		"tests/stuff.cpp",
		"tests/voxelizer.cpp",
	},
	
	["Gomoku"] =
	{
		"gomoku/game.cpp",
	},

	["h2dx11"] =
	{
		"h2dx11/h2dx11.cpp",
		"h2dx11/bsptexture.h",
		"h2dx11/bsptextureimpl.h",
		"h2dx11/pak.h",
		"h2dx11/pak.cpp",
		"h2dx11/stream.h",
	},
}
