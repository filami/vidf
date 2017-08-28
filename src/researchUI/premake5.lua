QtLibLocation = "C:/Qt/5.9.1/msvc2017_64/"
QtOutputPath = _WORKING_DIR.."/bin/ResearchUI/"


CreateVIDFAppProject("ResearchUI", "pch", nil, "B1AF6BD6-530C-4224-B5DF-FE3BC0E0D84C")


includedirs
{
	QtLibLocation.."/include/",
	QtLibLocation.."/include/QtWidgets/",
}



libdirs
{
	QtLibLocation.."lib/",
}



links
{
	"Qt5Core.lib",
	"Qt5Widgets.lib",
}



function QtCopyDll(dllName)
	os.copyfile(QtLibLocation.."bin/"..dllName..".dll", QtOutputPath..dllName..".dll");
	os.copyfile(QtLibLocation.."bin/"..dllName..".pdb", QtOutputPath..dllName..".pdb");
end



QtCopyDll("QtCore");
QtCopyDll("QtWidgets");
QtCopyDll("QtGui");


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
