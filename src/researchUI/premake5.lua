dofile "ext/premake-qt.git/qt.lua"
local qt = premake.extensions.qt

CreateVIDFAppProject("ResearchUI", --[["pch"]] nil, nil, "B1AF6BD6-530C-4224-B5DF-FE3BC0E0D84C")

qtpath "C:/Qt/5.9.1/msvc2017_64/"
qtprefix "Qt5"
qtgenerateddir(_WORKING_DIR.."/obj/ResearchUI_moc/")
qt.enable()

qtmodules { "core", "widgets", "gui" }


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
