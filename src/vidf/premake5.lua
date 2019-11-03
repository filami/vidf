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
		"assets/texture.h",
		"assets/texture.cpp",
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
		"platform/logger.cpp",
		"platform/logger.h",
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
		"rendererdx11/dds.h",
		"rendererdx11/gputimer.h",
		"rendererdx11/gputimer.cpp",
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

	--[[
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
	]]

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


-- YASLI

AddFilesToProject
{
	["Ext/yasli/yasli"] =
	{
		_WORKING_DIR.."/ext/yasli/yasli/**.h",
		_WORKING_DIR.."/ext/yasli/yasli/**.cpp",
	},
	["Ext/yasli/PropertyTree"] =
	{
		_WORKING_DIR.."/ext/yasli/PropertyTree/**.h",
		_WORKING_DIR.."/ext/yasli/PropertyTree/**.cpp",
	},
}



-- json11


AddFilesToProject
{
	["Ext/json11"] =
	{
		_WORKING_DIR.."/ext/json11/**.hpp",
		_WORKING_DIR.."/ext/json11/**.cpp",
	},
}


-- box2d


AddFilesToProject
{
	["Ext/Box2D"] =
	{
		_WORKING_DIR.."/ext/Box2D/Box2D.h",
	},
	["Ext/Box2D/Collision"] =
	{
		_WORKING_DIR.."/ext/Box2D/Collision/b2BroadPhase.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2BroadPhase.h",
		_WORKING_DIR.."/ext/Box2D/Collision/b2CollideCircle.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2CollideEdge.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2CollidePolygon.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2Collision.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2Collision.h",
		_WORKING_DIR.."/ext/Box2D/Collision/b2Distance.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2Distance.h",
		_WORKING_DIR.."/ext/Box2D/Collision/b2DynamicTree.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2DynamicTree.h",
		_WORKING_DIR.."/ext/Box2D/Collision/b2TimeOfImpact.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/b2TimeOfImpact.h",
	},
	["Ext/Box2D/Collision/Shapes"] =
	{
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2ChainShape.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2ChainShape.h",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2CircleShape.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2CircleShape.h",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2EdgeShape.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2EdgeShape.h",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2PolygonShape.cpp",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2PolygonShape.h",
		_WORKING_DIR.."/ext/Box2D/Collision/Shapes/b2Shape.h",
	},
	["Ext/Box2D/Common"] =
	{
		_WORKING_DIR.."/ext/Box2D/Common/b2BlockAllocator.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2BlockAllocator.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2Draw.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2Draw.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2GrowableStack.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2Math.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2Math.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2Settings.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2Settings.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2StackAllocator.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2StackAllocator.h",
		_WORKING_DIR.."/ext/Box2D/Common/b2Timer.cpp",
		_WORKING_DIR.."/ext/Box2D/Common/b2Timer.h",
	},
	["Ext/Box2D/Dynamics"] =
	{
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Body.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Body.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2ContactManager.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2ContactManager.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Fixture.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Fixture.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Island.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2Island.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2TimeStep.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2World.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2World.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2WorldCallbacks.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/b2WorldCallbacks.h",
	},
	["Ext/Dynamics/Contacts"] =
	{
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2CircleContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2CircleContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2Contact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2Contact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ContactSolver.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2ContactSolver.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2PolygonContact.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Contacts/b2PolygonContact.h",
	},
	["Ext/Dynamics/Joints"] =
	{
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2DistanceJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2DistanceJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2FrictionJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2FrictionJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2GearJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2GearJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2Joint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2Joint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2MotorJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2MotorJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2MouseJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2MouseJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2PrismaticJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2PulleyJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2PulleyJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2RevoluteJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2RopeJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2RopeJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2WeldJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2WeldJoint.h",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2WheelJoint.cpp",
		_WORKING_DIR.."/ext/Box2D/Dynamics/Joints/b2WheelJoint.h",
	},
	["Ext/Box2D/Rope"] =
	{
		_WORKING_DIR.."/ext/Box2D/Rope/b2Rope.cpp",
		_WORKING_DIR.."/ext/Box2D/Rope/b2Rope.h",
	},
}
