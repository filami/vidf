
-- prebuildcommands{_WORKING_DIR.."/src/ext/gm/src/gm/gmfrontend.bat"}

includedirs
{
	"src/platform/win32msvc"
}

AddFilesToProject
{
	["build"] =
	{
		"gm.lua",
	},

	["gm"] =
	{
		"src/gm/bison.hairy",
		"src/gm/bison.simple",
		"src/gm/flex.skl",
		"src/gm/gmArraySimple.cpp",
		"src/gm/gmArraySimple.h",
		"src/gm/gmByteCode.cpp",
		"src/gm/gmByteCode.h",
		"src/gm/gmByteCodeGen.cpp",
		"src/gm/gmByteCodeGen.h",
		"src/gm/gmCodeGen.cpp",
		"src/gm/gmCodeGen.h",
		"src/gm/gmCodeGenHooks.cpp",
		"src/gm/gmCodeGenHooks.h",
		"src/gm/gmCodeTree.cpp",
		"src/gm/gmCodeTree.h",
		"src/gm/gmConfig.h",
		"src/gm/gmCrc.cpp",
		"src/gm/gmCrc.h",
		"src/gm/gmDebug.cpp",
		"src/gm/gmDebug.h",
		"src/gm/gmDebugger.cpp",
		"src/gm/gmDebugger.h",
		"src/gm/gmfrontend.bat",
		"src/gm/gmFunctionObject.cpp",
		"src/gm/gmFunctionObject.h",
		"src/gm/gmHash.cpp",
		"src/gm/gmHash.h",
		"src/gm/gmIncGC.cpp",
		"src/gm/gmIncGC.h",
		"src/gm/gmIterator.h",
		"src/gm/gmLibHooks.cpp",
		"src/gm/gmLibHooks.h",
		"src/gm/gmListDouble.cpp",
		"src/gm/gmListDouble.h",
		"src/gm/gmLog.cpp",
		"src/gm/gmLog.h",
		"src/gm/gmMachine.cpp",
		"src/gm/gmMachine.h",
		"src/gm/gmMachineLib.cpp",
		"src/gm/gmMachineLib.h",
		"src/gm/gmMem.cpp",
		"src/gm/gmMem.h",
		"src/gm/gmMemChain.cpp",
		"src/gm/gmMemChain.h",
		"src/gm/gmMemFixed.cpp",
		"src/gm/gmMemFixed.h",
		"src/gm/gmMemFixedSet.cpp",
		"src/gm/gmMemFixedSet.h",
		"src/gm/gmOperators.cpp",
		"src/gm/gmOperators.h",
		"src/gm/gmParser.cpp",
		"src/gm/gmParser.cpp.h",
		"src/gm/gmParser.y",
		"src/gm/gmScanner.cpp",
		"src/gm/gmScanner.h",
		"src/gm/gmScanner.l",
		"src/gm/gmStream.cpp",
		"src/gm/gmStream.h",
		"src/gm/gmStreamBuffer.cpp",
		"src/gm/gmStreamBuffer.h",
		"src/gm/gmStringObject.cpp",
		"src/gm/gmStringObject.h",
		"src/gm/gmTableObject.cpp",
		"src/gm/gmTableObject.h",
		"src/gm/gmThread.cpp",
		"src/gm/gmThread.h",
		"src/gm/gmUserObject.cpp",
		"src/gm/gmUserObject.h",
		"src/gm/gmUtil.cpp",
		"src/gm/gmUtil.h",
		"src/gm/gmVariable.cpp",
		"src/gm/gmVariable.h",
	},
	["gmbinds"] =
	{
		"src/binds/gmArrayLib.cpp",
		"src/binds/gmArrayLib.h",
		"src/binds/gmCall.cpp",
		"src/binds/gmCall.h",
		"src/binds/gmGCRoot.cpp",
		"src/binds/gmGCRoot.h",
		"src/binds/gmGCRootUtil.cpp",
		"src/binds/gmGCRootUtil.h",
		"src/binds/gmHelpers.cpp",
		"src/binds/gmHelpers.h",
		"src/binds/gmMathLib.cpp",
		"src/binds/gmMathLib.h",
		"src/binds/gmStringLib.cpp",
		"src/binds/gmStringLib.h",
		"src/binds/gmSystemLib.cpp",
		"src/binds/gmSystemLib.h",
		"src/binds/gmVector3Lib.cpp",
		"src/binds/gmVector3Lib.h",
	},
}