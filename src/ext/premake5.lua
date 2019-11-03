CreateVIDFLibProject("viext", nil, nil, "34DA4A57-49AE-427E-8519-679304DDAD52")

cppdialect "C++11"

AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
}

dofile "gm/gm.lua"
