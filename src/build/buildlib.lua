buildLocation = _WORKING_DIR.."/make/"
allConfigs = "*"
debugConfig = "debug"
profileConfig = "profile"

solution "vidf"
uuid(os.uuid("vidf"))
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
	if projPch ~= nil then
		pchheader(projPch..".h")
		pchsource(projPch..".cpp")
	end
	if options.pedantic then
		-- flags {"ExtraWarnings", "FatalWarnings"}
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
		_WORKING_DIR.."/ext/include/",
	}
	defines {"VK_USE_PLATFORM_WIN32_KHR", "WIN64"}
	flags {"NoManifest", "NoRTTI"}
	
	-- debug
	configuration(debugConfig)
	defines {"DEBUG"}
	flags {"Symbols"}
	ConfigPaths(projName, debugConfig, targetLoc)
	
	-- profile
	configuration(profileConfig)
	defines {"NDEBUG", "PROFILE"}
	flags {
		"Symbols", "FloatFast",
		"NoFramePointer", "Optimize", "OptimizeSpeed" }
	ConfigPaths(projName, profileConfig, targetLoc)
end



function ConfigVIDFDependencies()
	configuration(allConfigs)
	libdirs
	{
		_WORKING_DIR.."/lib/",
		_WORKING_DIR.."/ext/lib/vulkan/",
	}
	
	links
	{
	--	"vulkan-1",
		"viext",
	}
	
	configuration(debugConfig)
	links{"vidf_x64_"..debugConfig}
	
	configuration(profileConfig)
	links{"vidf_x64_"..profileConfig}
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



function AddShader(fileName, entryPoint, shaderType)
	local entryPointCmd = string.format("/E\"%s\"", entryPoint)
	local outputCmd = string.format("/Fo\"%so\"", fileName)
	local typeCmd = string.format("/%s\"%s\"", shaderType, "_5_0")
	
	local command = string.format(
		"FxCompile /Zi %s /Od %s %s /nologo %s", 
		entryPointCmd, outputCmd, typeCmd, fileName)

	prebuildcommands{command}
end