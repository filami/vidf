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
}