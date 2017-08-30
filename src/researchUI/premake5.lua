QtLibLocation = "C:/Qt/5.9.1/msvc2017_64/"
QtOutputPath = _WORKING_DIR.."/bin/ResearchUI/"
QtMocTool = QtLibLocation.."bin/moc.exe"


CreateVIDFAppProject("ResearchUI", --[["pch"]] nil, nil, "B1AF6BD6-530C-4224-B5DF-FE3BC0E0D84C")


includedirs
{
	QtLibLocation.."/include/",
	QtLibLocation.."/include/QtCore/",
	QtLibLocation.."/include/QtWidgets/",
	QtLibLocation.."/include/QtGui/",
}



libdirs
{
	QtLibLocation.."lib/",
}



links
{
	"Qt5Core.lib",
	"Qt5Widgets.lib",
	"Qt5Gui.lib",
}



function QtCopyDll(dllName)
	os.copyfile(QtLibLocation.."bin/"..dllName..".dll", QtOutputPath..dllName..".dll");
	os.copyfile(QtLibLocation.."bin/"..dllName..".pdb", QtOutputPath..dllName..".pdb");
end

QtCopyDll("QtCore");
QtCopyDll("QtWidgets");
QtCopyDll("QtGui");



function QtMoc(files)
	local mocFiles = {}
	for i, fileName in ipairs(files) do
		local command = string.format(
			"%s -o%s %s",
			QtMocTool,
			fileName..".moc.cpp",
			fileName..".h")
		prebuildcommands{command};
		table.insert(mocFiles, fileName..".moc.cpp");
	end
	AddFilesToProject
	{
		["mocs"] = mocFiles,
	}
end



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
		"tests/notepad.h",
		"tests/notepad.cpp",
		"tests/lenses.h",
		"tests/lenses.cpp",
	},
}

QtMoc
{
	_WORKING_DIR.."/src/researchUI/tests/notepad",
	_WORKING_DIR.."/src/researchUI/tests/lenses",
}

-- YASLI

includedirs
{
	_WORKING_DIR.."/ext/yasli",
}

AddFilesToProject
{
	["yasli/yasli"] =
	{
		_WORKING_DIR.."/ext/yasli/yasli/**.h",
		_WORKING_DIR.."/ext/yasli/yasli/**.cpp",
	},
	["yasli/PropertyTree"] =
	{
		_WORKING_DIR.."/ext/yasli/PropertyTree/**.h",
		_WORKING_DIR.."/ext/yasli/PropertyTree/**.cpp",
	},
	["yasli/QPropertyTree"] =
	{
		_WORKING_DIR.."/ext/yasli/QPropertyTree/IconXPMCache.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/IconXPMCache.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetComboBox.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetNumber.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetString.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/PropertyRowFileOpen.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/PropertyRowFileOpen.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/PropertyRowFileSave.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QDrawContext.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QDrawContext.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QPropertyTree.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QPropertyTree.h",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QUIFacade.cpp",
		_WORKING_DIR.."/ext/yasli/QPropertyTree/QUIFacade.h",
	},
}

QtMoc
{
	_WORKING_DIR.."/ext/yasli/QPropertyTree/QPropertyTree",
	_WORKING_DIR.."/ext/yasli/QPropertyTree/QUIFacade",
	_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetComboBox",
	_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetNumber",
	_WORKING_DIR.."/ext/yasli/QPropertyTree/InplaceWidgetString",
}
