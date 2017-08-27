#pragma once

#include "vidf/rendererdx11/resources.h"

namespace h2
{



	struct BspTexture
	{
		std::string           name;
		vidf::dx11::Texture2D gpuTexture;
		vidf::uint            width = 1;
		vidf::uint            heigth = 1;
		vidf::uint            flags = 0;
	};



	template<typename TStream>
	StreamResult StreamM8(TStream& stream, BspTexture& m8Data);



	template<typename TStream>
	vidf::dx11::Texture2D LoadTextureM8(TStream& stream, vidf::dx11::RenderDevicePtr renderDevice, const char* textureName);



}

#include "bsptextureimpl.h"
