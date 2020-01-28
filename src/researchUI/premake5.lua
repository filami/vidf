dofile "ext/premake-qt.git/qt.lua"
local qt = premake.extensions.qt

CreateVIDFAppProject("ResearchUI", --[["pch"]] nil, nil, "B1AF6BD6-530C-4224-B5DF-FE3BC0E0D84C")

AddIncludeFolder("src/research")
AddStaticLibLink("Research")

rtti "on"

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

	["fractals"] =
	{
		"fractals/fractals.h",
		"fractals/fractals.cpp",
	},

	["viewport"] =
	{
		"viewport/dx11viewport.h",
		"viewport/dx11viewport.cpp",
	},

	["tests"] =
	{
		"tests/assetbrowser.h",
		"tests/assetbrowser.cpp",
		"tests/notepad.h",
		"tests/notepad.cpp",
		"tests/lenses.h",
		"tests/lenses.cpp",
	},

	-- GAME

	["Platform"] =
	{
		"../research/platform/platform.cpp",
	},
	["Platform/WorldRender"] =
	{
		"../research/platform/worldrender/voxel.h",
		"../research/platform/worldrender/voxel.cpp",
		"../research/platform/worldrender/voxelmodelrenderer.h",
		"../research/platform/worldrender/voxelmodelrenderer.cpp",
		"../research/platform/worldrender/worldrender.h",
		"../research/platform/worldrender/worldrender.cpp",
	},
	["Platform/MagicaVoxel"] =
	{
		"../research/platform/MagicaVoxel/vox.h",
		"../research/platform/MagicaVoxel/vox.cpp",
	},
}


-- YASLI

AddFilesToProject
{
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

-- NodeEditor

defines {"NODE_EDITOR_STATIC"}

includedirs { _WORKING_DIR.."/ext/nodeeditor/include/" }

AddFilesToProject
{
	["NodeEditor/src"] =
	{
		_WORKING_DIR.."/ext/nodeeditor/src/Compiler.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Connection.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Connection.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionBlurEffect.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionBlurEffect.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionGeometry.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionGeometry.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionGraphicsObject.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionGraphicsObject.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionPainter.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionPainter.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionState.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionState.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionStyle.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/ConnectionStyle.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/DataModelRegistry.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/DataModelRegistry.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Export.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowScene.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowScene.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowView.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowView.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowViewStyle.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/FlowViewStyle.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/make_unique.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Node.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Node.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeConnectionInteraction.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeConnectionInteraction.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeData.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeDataModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeGeometry.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeGeometry.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeGraphicsObject.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeGraphicsObject.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodePainter.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodePainter.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodePainterDelegate.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeState.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeState.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeStyle.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/NodeStyle.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/OperatingSystem.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/PortType.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Properties.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Properties.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/QStringStdHash.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/QUuidStdHash.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Serializable.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/Style.hpp",
		_WORKING_DIR.."/ext/nodeeditor/src/StyleCollection.cpp",
		_WORKING_DIR.."/ext/nodeeditor/src/StyleCollection.hpp",
	},
}

AddFilesToProject
{
	["NodeEditor/resources"] =
	{
		_WORKING_DIR.."/ext/nodeeditor/resources/DefaultStyle.json",
		_WORKING_DIR.."/ext/nodeeditor/resources/resources.qrc",
	},

	["NodeEditor/include"] =
	{
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/Connection",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/ConnectionStyle",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/DataModelRegistry",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/FlowScene",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/FlowView",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/FlowViewStyle",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/Node",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/NodeData",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/NodeDataModel",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/NodeGeometry",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/NodePainterDelegate",
		_WORKING_DIR.."/ext/nodeeditor/include/nodes/NodeStyle",
	},

	["NodeEditor/examples/example2"] =
	{
		_WORKING_DIR.."/ext/nodeeditor/examples/example2/TextData.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/example2/TextDisplayDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/example2/TextDisplayDataModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/example2/TextSourceDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/example2/TextSourceDataModel.hpp",
	},

	["NodeEditor/examples/calculator"] =
	{
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/AdditionModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/DecimalData.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/DecimalToIntegerModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/DecimalToIntegerModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/DivisionModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/IntegerData.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/IntegerToDecimalModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/IntegerToDecimalModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/MathOperationDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/MathOperationDataModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/ModuloModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/ModuloModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/MultiplicationModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/NumberDisplayDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/NumberDisplayDataModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/NumberSourceDataModel.cpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/NumberSourceDataModel.hpp",
		_WORKING_DIR.."/ext/nodeeditor/examples/calculator/SubtractionModel.hpp",
	},
}
