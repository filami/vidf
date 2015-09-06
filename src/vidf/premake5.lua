vidfProj = CreateVIDFProject()

files
{
	"pch.h", "pch.cpp",
	"common/**.h", "common/**.cpp", "common/**.inl",
	"platform/*.h", "platform/*.cpp",
	"platform/win32/*.h", "platform/win32/*.cpp",
	"proto/**.h", "proto/**.cpp",
}

AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
}
