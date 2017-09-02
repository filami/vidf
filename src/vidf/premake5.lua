vidfProj = CreateVIDFProject()

AddFilesToProject
{
	["build"] =
	{
		"premake5.lua",
	},
	
	["buildlib"] =
	{
		"../../premake5.lua",
		"../build/buildlib.lua",
	},

	["Assets"] =
	{
		"assets/asset.h",
		"assets/asset.cpp",
		"assets/assetmanager.h",
		"assets/assetmanager.cpp",
	},
	
	["Common"] =
	{
		"pch.h",
		"pch.cpp",
		"common/aspectratio.h",
		"common/bitfield.h",
		"common/box.h",
		"common/color.h",
		"common/half.cpp",
		"common/half.h",
		"common/interpolate.h",
		"common/intersect.h",
		"common/matrix33.h",
		"common/matrix44.h",
		"common/montecarlo.h",
		"common/noise.h",
		"common/plane.h",
		"common/pointer.h",
		"common/quaternion.h",
		"common/random.h",
		"common/randomlinearcongruential.inl",
		"common/randomnormaldist.inl",
		"common/randomrand48.inl",
		"common/randomsignedcompare.inl",
		"common/randomuniformint.inl",
		"common/randomuniformreal.inl",
		"common/randomuniformsphere.inl",
		"common/ray.h",
		"common/rect.h",
		"common/rtti.h",
		"common/saltedarray.h",
		"common/sphere.h",
		"common/sphericalharmonics.cpp",
		"common/sphericalharmonics.h",
		"common/transformations.h",
		"common/triangle.h",
		"common/types.h",
		"common/utils.h",
		"common/vector2.h",
		"common/vector3.h",
		"common/vector4.h",
	},
	
	["Platform"] =
	{	
		"platform/canvas.cpp",
		"platform/canvas.h",
		"platform/filechangenotification.cpp",
		"platform/filechangenotification.h",
		"platform/filesystemutils.h",
		"platform/system.h",
		"platform/taskmanager.h",
		"platform/time.cpp",
		"platform/time.h",
	},
	["Platform/Win32"] =
	{
		"platform/win32/canvaswin32.cpp",
		"platform/win32/filechangenotificationwin32.cpp",
		"platform/win32/systemwin32.cpp",
		"platform/win32/timewin32.cpp",
	},

	["Renderer Dx11"] =
	{
		"rendererdx11/common.h",
		"rendererdx11/debug.h",
		"rendererdx11/pipeline.h",
		"rendererdx11/pipeline.cpp",
		"rendererdx11/renderdevice.h",
		"rendererdx11/renderdevice.cpp",
		"rendererdx11/resources.h",
		"rendererdx11/resources.cpp",
		"rendererdx11/shaders.h",
		"rendererdx11/shaders.cpp",
		"rendererdx11/wikidraw.h",
		"rendererdx11/wikidraw.cpp",
	},

	["Renderer Vulkan"] =
	{
		"renderervulkan/common.h",
		"renderervulkan/rendercontext.cpp",
		"renderervulkan/rendercontext.h",
		"renderervulkan/renderdevice.cpp",
		"renderervulkan/renderdevice.h",
		"renderervulkan/renderpass.cpp",
		"renderervulkan/renderpass.h",
		"renderervulkan/swapchain.cpp",
		"renderervulkan/swapchain.h",
		"renderervulkan/vulkanext.cpp",
		"renderervulkan/vulkanext.h",
	},
	["Renderer Vulkan/Vulkan Win32"] =
	{
		"renderervulkan/vulkanwin32/renderdevicevkwin32.cpp",
		"renderervulkan/vulkanwin32/swapchainvkwin32.cpp",
	},

	["Proto"] =
	{
		"proto/camera.cpp",
		"proto/camera.h",
		"proto/mesh.cpp",
		"proto/mesh.h",
		"proto/protogl.cpp",
		"proto/protogl.h",
		"proto/protoglpragma.h",
		"proto/text.cpp",
		"proto/text.h",
	},
}
