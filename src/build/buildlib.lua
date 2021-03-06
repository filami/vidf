buildLocation = _WORKING_DIR.."/make/"
allConfigs = "*"
debugConfig = "debug"
profileConfig = "profile"

location (buildLocation)
platforms {"x64"}
configurations {debugConfig, profileConfig}



function MakeDefaultOptions(options)
	if options == nil then
		options = {}
	end
	if options.pedantic == nil then
		options.pedantic = true
	end
	return options
end



function CreateProject(projName, projPch, options, projUUID)
	project(projName)
	uuid(projUUID)
	location(buildLocation)
	language "c++"
	cppdialect "C++17"
	if options.pedantic then
		flags {"NoWarnings"}
	else
		flags {"NoWarnings"}
	end
end



function ConfigPaths(projName, configName, targetLoc)
	objdir(_WORKING_DIR.."/obj/"..projName.."_"..configName)
	if (targetLoc == "lib") then
		targetdir(_WORKING_DIR.."/lib");
	elseif (targetLoc == "bin") then
		local path = _WORKING_DIR.."/bin/"..projName;
		targetdir(path);
		debugdir(path);
	end
	targetname(projName.."_x64_"..configName)
end



function ConfigCommon(projName, targetLoc)
	-- all configs
	configuration(allConfigs)
	includedirs
	{
		".",
		_WORKING_DIR.."/src/",
		_WORKING_DIR.."/src/vidf/",
		_WORKING_DIR.."/src/ext/gm/src/gm/",
		_WORKING_DIR.."/src/ext/gm/src/platform/win32msvc/",
		_WORKING_DIR.."/src/ext/gm/src/binds/",
		_WORKING_DIR.."/ext/",
		_WORKING_DIR.."/ext/include/",
		_WORKING_DIR.."/ext/yasli",
		_WORKING_DIR.."/ext/json11",
		_WORKING_DIR.."/ext/dxc",
	}
	defines {
		"VK_USE_PLATFORM_WIN32_KHR",
		"WIN64",
	}
	flags {"NoManifest"}
	rtti "off"
	symbols "on"

	-- systemversion "10.0.15063.0"
	systemversion "10.0.17763.0"
	
	-- debug
	configuration(debugConfig)
	defines {"DEBUG"}
	ConfigPaths(projName, debugConfig, targetLoc)
	
	-- profile
	configuration(profileConfig)
	defines {"NDEBUG", "PROFILE"}
	flags {
		"FloatFast",
		"NoFramePointer", "Optimize", "OptimizeSpeed" }
	ConfigPaths(projName, profileConfig, targetLoc)
end



function ConfigVIDFDependencies()
	configuration(allConfigs)
	libdirs
	{
		_WORKING_DIR.."/lib/",
		_WORKING_DIR.."/ext/lib/vulkan/",
		_WORKING_DIR.."/ext/brofiler/",
		_WORKING_DIR.."/ext/dxc/",
	}
end



function CreateVIDFProject()
	options = MakeDefaultOptions(nil)
	CreateProject("vidf", "pch", options, "DD97FFC2-D13C-4317-AD1F-300F8513C7CE")
	kind "StaticLib"
	
	ConfigCommon("vidf", "lib");
	
	configuration(allConfigs)
end


function CreateVIDFLibProject(projName, projPch, options, projUUID)
	options = MakeDefaultOptions(options)
	CreateProject(projName, projPch, options, projUUID)
	kind "StaticLib"
	
	ConfigCommon(projName, "lib");
	ConfigVIDFDependencies();
	
	configuration(allConfigs)
end


function CreateVIDFAppProject(projName, projPch, options, projUUID)
	options = MakeDefaultOptions(options)
	CreateProject(projName, projPch, options, projUUID)
	kind "ConsoleApp"
	
	ConfigCommon(projName, "bin");
	ConfigVIDFDependencies();
	
	configuration(allConfigs)
end


function AddFilesToProject(projFiles)
	local allFiles = {}
	for path,files in pairs(projFiles) do
		for i,file in ipairs(files) do
			table.insert(allFiles, file)
		end
	end
	vpaths(projFiles)
	files(allFiles)
end



function AddIncludeFolder(incName)
	includedirs
	{
		_WORKING_DIR.."/"..incName.."/",
	}
end



function AddStaticLibLink(libName)
	configuration(debugConfig)
	links{libName.."_x64_"..debugConfig}
	
	configuration(profileConfig)
	links{libName.."_x64_"..profileConfig}

	configuration(allConfigs)
end



function AddShader(fileName, entryPoint, shaderType)
	local entryPointCmd = string.format("/E\"%s\"", entryPoint)
	local outputCmd = string.format("/Fo\"%so\"", fileName)
	local typeCmd = string.format("/%s\"%s\"", shaderType, "_5_0")
	
	local command = string.format(
		"FxCompile /Zi %s /Od %s %s /nologo %s", 
		entryPointCmd, outputCmd, typeCmd, fileName)

	prebuildcommands{command}
end