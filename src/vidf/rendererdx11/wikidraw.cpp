#include "pch.h"
#include "wikidraw.h"
#include "shaders.h"
#include "pipeline.h"

namespace vidf { namespace dx11
{



	WikiDraw::WikiDraw(RenderDevicePtr _renderDevice, ShaderManager* _shaderManager)
		: renderDevice(_renderDevice)
	{
		uint32 white = ~0;
		Texture2DDesc texDesc{};
		texDesc.width = 1;
		texDesc.heigh = 1;
		texDesc.mipLevels = 1;
		texDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		texDesc.name = "WikiDraw/DefaultTexture";
		texDesc.dataPtr = &white;
		texDesc.dataSize = sizeof(uint32);
		defaultTexture = Texture2D::Create(renderDevice, texDesc);

		D3D11_SAMPLER_DESC ssDesc{};
		ssDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		ssDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		ssDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		ssDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		ssDesc.MinLOD = -std::numeric_limits<float>::max();
		ssDesc.MaxLOD = std::numeric_limits<float>::max();
		ssDesc.BorderColor[0] = ssDesc.BorderColor[1] = ssDesc.BorderColor[2] = ssDesc.BorderColor[3] = 1.0f;
		renderDevice->GetDevice()->CreateSamplerState(&ssDesc, &defaultSampler.Get());

		vertices.reserve(128);

		vertexShader = _shaderManager->CompileShaderFile("data/shaders/wikidraw.hlsl", "vsMain", ShaderType::VertexShader);
		pixelShader = _shaderManager->CompileShaderFile("data/shaders/wikidraw.hlsl", "psMain", ShaderType::PixelShader);

		std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
		D3D11_INPUT_ELEMENT_DESC desc{};
		desc.SemanticName = "POSITION";
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.AlignedByteOffset = offsetof(Vertex, vertex);
		elements.push_back(desc);
		desc.SemanticName = "TEXCOORD";
		desc.Format = DXGI_FORMAT_R32G32_FLOAT;
		desc.AlignedByteOffset = offsetof(Vertex, tc);
		elements.push_back(desc);
		desc.SemanticName = "COLOR";
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.AlignedByteOffset = offsetof(Vertex, color);
		elements.push_back(desc);

		PSOs.resize(NumPsos);
		GraphicsPSODesc PSODesc;
		PSODesc.geometryDesc = elements;
		PSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
		PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		PSODesc.depthStencil.DepthEnable = false;
		PSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		PSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
		PSODesc.rasterizer.FrontCounterClockwise = true;
		PSODesc.blend.RenderTarget[0].BlendEnable = true;
		PSODesc.blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		PSODesc.blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		PSODesc.blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		PSODesc.blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		PSODesc.blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		PSODesc.blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		PSODesc.vertexShader = vertexShader;
		PSODesc.pixelShader = pixelShader;
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		PSOs[PointsPso] = GraphicsPSO::Create(renderDevice, PSODesc);
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		PSOs[LinesPso] = GraphicsPSO::Create(renderDevice, PSODesc);
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		PSOs[TrianglesPso] = GraphicsPSO::Create(renderDevice, PSODesc);

		ConstantBufferDesc viewCBDesc(sizeof(CBuffer), "cBuffer");
		cBuffer = ConstantBuffer::Create(renderDevice, viewCBDesc);

		ResetSRV();
	}



	void WikiDraw::Begin(WikiDraw::StreamType type)
	{
		assert(type != Undefined);
		assert(!streaming);
		assert(!viewProjTMs.empty());
		assert(!worldTMs.empty());
		uint psoIdx;
		switch (type)
		{
		case vidf::dx11::WikiDraw::Points:    psoIdx = PointsPso; break;
		case vidf::dx11::WikiDraw::Lines:     psoIdx = LinesPso; break;
		case vidf::dx11::WikiDraw::Triangles: psoIdx = TrianglesPso; break;
		case vidf::dx11::WikiDraw::Quads:     psoIdx = TrianglesPso; break;
		}
		if (curBatch.psoIdx != psoIdx)
			RestartBatch();
		curBatch.psoIdx = psoIdx;
		curBatch.type = type;
		streaming = true;
	}



	void WikiDraw::End()
	{
		assert(streaming);
		assert(primitive.empty());
		streaming = false;
	}



	void WikiDraw::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
	{
		curVertex.color = (a << 24) | (b << 16) | (g << 8) | r;
	}



	void WikiDraw::SetTexCoord(Vector2f tc)
	{
		curVertex.tc = tc;
	}



	void WikiDraw::PushVertex(Vector3f vertex)
	{
		curVertex.vertex = vertex;
		PushVertex(curVertex);
	}



	void WikiDraw::PushVertex(Vertex vertex)
	{
		assert(streaming);

		curVertex = vertex;
		primitive.push_back(vertex);

		switch (curBatch.type)
		{
		case vidf::dx11::WikiDraw::Points:
			vertices.push_back(curVertex);
			primitive.clear();
			curBatch.numVertices++;
			break;
		case vidf::dx11::WikiDraw::Lines:
			if (primitive.size() == 2)
			{
				vertices.insert(vertices.end(), primitive.begin(), primitive.end());
				primitive.clear();
				curBatch.numVertices += 2;
			}
			break;
		case vidf::dx11::WikiDraw::Triangles:
			if (primitive.size() == 3)
			{
				vertices.insert(vertices.end(), primitive.begin(), primitive.end());
				primitive.clear();
				curBatch.numVertices += 3;
			}
			break;
		case vidf::dx11::WikiDraw::Quads:
			if (primitive.size() == 4)
			{
				vertices.push_back(primitive[0]);
				vertices.push_back(primitive[1]);
				vertices.push_back(primitive[2]);
				vertices.push_back(primitive[0]);
				vertices.push_back(primitive[2]);
				vertices.push_back(primitive[3]);
				primitive.clear();
				curBatch.numVertices += 6;
			}
			break;
		}
	}



	void WikiDraw::PushProjViewTM(const Matrix44f& tm)
	{
		assert(!streaming);
		if (curBatch.psoIdx != -1)
			RestartBatch();
		curBatch.projViewTMIdx = viewProjTMs.size();
		curBatch.psoIdx = -1;
		viewProjTMs.push_back(tm);
	}



	void WikiDraw::PushWorldTM(const Matrix44f& tm)
	{
		assert(!streaming);
		if (curBatch.psoIdx != -1)
			RestartBatch();
		curBatch.worldTMIdx = worldTMs.size();
		curBatch.psoIdx = -1;
		worldTMs.push_back(tm);
	}



	void WikiDraw::PushSRV(PD3D11ShaderResourceView srv, PD3D11SamplerState sampler)
	{
		assert(!streaming);
		if (curBatch.psoIdx != -1)
			RestartBatch();
		curBatch.srvIdx = srvs.size();
		curBatch.psoIdx = -1;
		srvs.push_back(srv);
		samplers.push_back(sampler);
	}



	void WikiDraw::PushSRV(PD3D11ShaderResourceView srv)
	{
		PushSRV(srv, defaultSampler);
	}



	void WikiDraw::ResetSRV()
	{
		PushSRV(defaultTexture.srv, defaultSampler);
	}



	void WikiDraw::Flush(CommandBuffer* commandBuffer)
	{
		assert(!streaming);
		assert(!viewProjTMs.empty());
		assert(!worldTMs.empty());
		assert(!worldTMs.empty());
		assert(!srvs.empty());

		if (curBatch.numVertices != 0)
			RestartBatch();
				
		uint lastWorldTM = -1;
		uint lastProjViewTM = -1;
		UpdateVertexBuffer(commandBuffer);
		commandBuffer->SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
		commandBuffer->SetConstantBuffer(0, cBuffer.buffer);
		for (auto& batch : batches)
		{
			if (lastProjViewTM != batch.projViewTMIdx || lastWorldTM != batch.worldTMIdx)
				UpdateCBuffer(commandBuffer, batch);

			PD3D11SamplerState ss = samplers[batch.srvIdx];
			commandBuffer->SetGraphicsPSO(PSOs[batch.psoIdx]);
			commandBuffer->SetSRV(0, srvs[batch.srvIdx]);
			commandBuffer->GetContext()->PSSetSamplers(0, 1, &ss.Get());
			commandBuffer->Draw(batch.numVertices, batch.firstVertex);

			lastWorldTM = batch.worldTMIdx;
			lastProjViewTM = batch.projViewTMIdx;
		}

		vertices.clear();
		batches.clear();
		viewProjTMs.clear();
		worldTMs.clear();
		srvs.clear();
		samplers.clear();
		curBatch = Batch();
		curVertex = Vertex();
		ResetSRV();
	}



	void WikiDraw::RestartBatch()
	{
		if (curBatch.numVertices != 0)
			batches.push_back(curBatch);
		curBatch.type = Undefined;
		curBatch.firstVertex = vertices.size();
		curBatch.numVertices = 0;
		streaming = false;
	}



	void WikiDraw::UpdateCBuffer(CommandBuffer* commandBuffer, const Batch& batch)
	{
		CBuffer cBufferData;
		cBufferData.viewProjTM = viewProjTMs[batch.projViewTMIdx];
		cBufferData.worldTM = worldTMs[batch.worldTMIdx];
		cBuffer.Update(commandBuffer->GetContext(), cBufferData);
	}



	void WikiDraw::UpdateVertexBuffer(CommandBuffer* commandBuffer)
	{
		if (gpuVertexCount != vertices.capacity())
		{
			VertexBufferDesc vertexBufferDesc(uint(sizeof(Vertex)), vertices.capacity(), "WikiGeomVB");
			vertexBufferDesc.dynamic = true;
			vertexBuffer = VertexBuffer::Create(commandBuffer->GetRenderDevice(), vertexBufferDesc);
			gpuVertexCount = vertices.capacity();
		}
		vertexBuffer.Update(commandBuffer->GetContext(), vertices.data(), vertices.size() * sizeof(Vertex));
	}




	WikiText::WikiText(RenderDevicePtr renderDevice)
	{
		HFONT font;

		const uint size = 256;
		font = CreateFont(
			size, 0, 0, 0, FW_BOLD, false, false, false,
			ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH,
			TEXT("Courier New"));

		HDC screenDC = ::GetDC(::GetDesktopWindow());
		HDC bitmapDC = ::CreateCompatibleDC(screenDC);
		SelectObject(bitmapDC, font);

		int width = 2048;
		int height = size;

		WCHAR text[numChars];
		for (int i = 0; i < numChars; ++i)
			text[i] = WCHAR(i + firstChar);
		INT widths[numChars];
		GetCharWidth32(bitmapDC, firstChar, firstChar + numChars - 1, widths);
		int x = 0;
		int y = 0;
		scale = 1.0f;
		for (int i = 0; i < numChars; ++i)
		{
			if (x + widths[i] > width)
			{
				x = 0;
				y += size;
			}
			if (y + size > height)
			{
				scale *= 2.0f;
				height *= 2;
			}
			x += widths[i];
		}

		BITMAPINFO canvas;
		ZeroMemory(&canvas, sizeof(canvas));
		canvas.bmiHeader.biSize = sizeof(canvas.bmiHeader);
		canvas.bmiHeader.biWidth = width;
		canvas.bmiHeader.biHeight = height;
		canvas.bmiHeader.biPlanes = 1;
		canvas.bmiHeader.biBitCount = 32;
		HBITMAP compBitmap = CreateDIBSection(bitmapDC, &canvas, DIB_RGB_COLORS, 0, 0, 0);
		SelectObject(bitmapDC, compBitmap);

		RECT rect;
		ZeroMemory(&rect, sizeof(rect));
		rect.right = width;
		rect.bottom = height;
		COLORREF backgroundCl = RGB(0, 0, 0);
		COLORREF foregroundCl = RGB(255, 255, 255);
		HBRUSH background = ::CreateSolidBrush(backgroundCl);
		FillRect(bitmapDC, &rect, background);
		SetTextColor(bitmapDC, foregroundCl);
		SetBkMode(bitmapDC, TRANSPARENT);

		x = 0;
		int chars = 0;
		WCHAR curChar = firstChar;
		for (int i = 0; i < numChars; ++i)
		{
			if (x + widths[i] > width)
			{
				DrawTextW(bitmapDC, text, chars, &rect, DT_LEFT | DT_NOCLIP);
				rect.left = 0;
				rect.top += size;
				chars = 0;
				x = 0;
			}
			rects[i].min.x = x / float(width);
			rects[i].min.y = 1.0f - (rect.top / float(height));
			rects[i].max.x = (x + widths[i]) / float(width);
			rects[i].max.y = 1.0f - ((rect.top + size) / float(height));
			text[chars++] = curChar++;
			x += widths[i];
		}
		DrawTextW(bitmapDC, text, chars, &rect, DT_LEFT | DT_NOCLIP);

		BITMAPINFOHEADER bi;
		ZeroMemory(&bi, sizeof(bi));
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		bi.biHeight = height;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = BI_RGB;
		LONG inBufferSz = width * height * 4;
		LONG outBufferSz = width * height * 4;
		LPVOID inBuffer = operator new(inBufferSz);
		LPVOID outBuffer = operator new(outBufferSz);
		GetDIBits(
			bitmapDC, compBitmap,
			0, (UINT)height,
			inBuffer,
			(BITMAPINFO*)&bi, DIB_RGB_COLORS);
		for (int i = 0; i < width * height; ++i)
		{
			unsigned char* inPtr = ((unsigned char*)inBuffer) + i * 4;
			unsigned char* outPtr = ((unsigned char*)outBuffer) + i * 4;
			*(outPtr + 0) = 0xff;
			*(outPtr + 1) = 0xff;
			*(outPtr + 2) = 0xff;
			*(outPtr + 3) = *(inPtr + 0);
		}

		// TODO - reduce/compress texture
		Texture2DDesc desc;
		desc.width = width;
		desc.heigh = height;
		desc.mipLevels = 1;
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.name = "WikiText/TextAtlas";
		desc.dataPtr = outBuffer;
		desc.dataSize = outBufferSz;
		texture = Texture2D::Create(renderDevice, desc);

		operator delete(inBuffer);
		operator delete(outBuffer);
	}

	

	void WikiText::OutputText(WikiDraw& wikiDraw, const Vector2f& position, const float size, const char* text, ...) const
	{
		assert(text != nullptr);

		const float aspect = 1.0f;	// TODO

		const int bufferSize = 256;
		char buffer[bufferSize];
		va_list ap;
		va_start(ap, text);
		vsprintf_s<bufferSize>(buffer, text, ap);
		va_end(ap);

		wikiDraw.PushSRV(texture.srv);

		Vector2f p = Vector2f(position.x, position.y);
		size_t strLen = std::strlen(text);
		wikiDraw.Begin(WikiDraw::Quads);
		for (int i = 0; i < strLen; ++i)
		{
			int charIdx = buffer[i] - firstChar;

			if (charIdx < 0 || charIdx >= numChars)
			{
				if (text[i] == '\n')
				{
					p.x = position.x;
					p.y += size;
				}
			}
			else
			{
				Rectf rect = rects[charIdx];
				Vector2f sz = Vector2f((rect.max.x - rect.min.x) * aspect, rect.max.y - rect.min.y) * scale * size;
			//	if (flip)
			//		Swap(rect.min.y, rect.max.y);

				wikiDraw.SetTexCoord(Vector2f(rect.min.x, rect.max.y));
				wikiDraw.PushVertex(Vector3f(p.x, p.y, 0.0f));

				wikiDraw.SetTexCoord(Vector2f(rect.max.x, rect.max.y));
				wikiDraw.PushVertex(Vector3f(p.x + sz.x, p.y, 0.0f));

				wikiDraw.SetTexCoord(Vector2f(rect.max.x, rect.min.y));
				wikiDraw.PushVertex(Vector3f(p.x + sz.x, p.y - sz.y, 0.0f));

				wikiDraw.SetTexCoord(Vector2f(rect.min.x, rect.min.y));
				wikiDraw.PushVertex(Vector3f(p.x, p.y - sz.y, 0.0f));

				p.x += sz.x;
			}
		}
		wikiDraw.End();
	}



} }
