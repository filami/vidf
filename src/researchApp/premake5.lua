
CreateVIDFAppProject("ResearchApp", "pch", nil, "86823B72-7234-4F85-83CB-52CEBCEF9103")

AddIncludeFolder("src/research")
AddStaticLibLink("Research")


AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
	
	["common"] =
	{
		"main.cpp",
	},
}
