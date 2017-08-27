#include "pch.h"
#include "stream.h"
#include "bsptexture.h"

namespace h2
{



	template<typename TStream>
	StreamResult StreamM8(TStream& stream, BspTexture& m8Data)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		const size_t fileOffset = stream.tellg();

		uint32 signature;
		Stream(stream, signature);
		if (signature != 0x02)
			return StreamResult::Fail;

		char name[32];
		Stream(stream, name);

		std::array<uint32, 16> values;
		Stream(stream, values.begin(), values.end());
		m8Data.width = values[0];
		Stream(stream, values.begin(), values.end());
		m8Data.heigth = values[0];

		stream.seekg(fileOffset + 1028);
		Stream(stream, m8Data.flags);

		return StreamResult::Ok;
	}



	template<typename TStream>
	vidf::dx11::Texture2D LoadTextureM8(TStream& stream, vidf::dx11::RenderDevicePtr renderDevice, const char* textureName)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		Texture2D texture;

		const size_t fileOffset = stream.tellg();

		uint32 signature;
		Stream(stream, signature);
		assert(signature == 0x02);

		char name[32];
		Stream(stream, name);

		const uint maxNumMips = 16;
		std::array<uint32, maxNumMips> widths;
		std::array<uint32, maxNumMips> heigths;
		std::array<uint32, maxNumMips> offsets;
		Stream(stream, widths.begin(), widths.end());
		Stream(stream, heigths.begin(), heigths.end());
		Stream(stream, offsets.begin(), offsets.end());

		uint numMips = 0;
		uint bufferSize = 0;
		for (uint i = 0; i < 16; ++i)
		{
			if (widths[i] == 0 || heigths[i] == 0)
				break;
			bufferSize += widths[i] * heigths[i];
			++numMips;
		}

		char nextFrame[32];
		Stream(stream, nextFrame);

		const uint paletteSize = 256 * 3;
		uint8 palette[paletteSize];
		Stream(stream, palette);

		std::vector<uint32> buffer;
		buffer.resize(bufferSize);
		Texture2DDesc desc;
		desc.width = widths[0];
		desc.heigh = heigths[0];
		desc.mipLevels = numMips;
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.name = textureName;

		std::vector<uint32>::iterator dstIt = buffer.begin();
		std::vector<uint8> inputBuffer;
		inputBuffer.resize(widths[0] * heigths[0]);
		for (uint i = 0; i < numMips; ++i)
		{
			uint bufferSz = widths[i] * heigths[i];
			stream.seekg(fileOffset + offsets[i]);
			// Stream(stream, inputBuffer.data(), bufferSz);
			stream.read((char*)inputBuffer.data(), bufferSz);
			for (uint j = 0; j < bufferSz; ++j)
			{
				const uint8 r = palette[inputBuffer[j] * 3 + 0];
				const uint8 g = palette[inputBuffer[j] * 3 + 1];
				const uint8 b = palette[inputBuffer[j] * 3 + 2];
				const uint32 color = (0xff << 24) | (b << 16) | (g << 8) | r;
				*dstIt = color;
				++dstIt;
			}
		}

		desc.dataPtr = buffer.data();
		desc.dataSize = bufferSize * sizeof(uint32);
		texture = Texture2D::Create(renderDevice, desc);

		return texture;
	}



}
