#pragma once

#include "common.h"

namespace vidf { namespace dx11
{


	enum class ShaderType
	{
		Unknown,
		VertexShader,
		HullShader,
		DomainShader,
		GeometryShader,
		PixelShader,
		ComputeShader,
	};


	class Shader
	{
	public:
		~Shader();
		ID3D11VertexShader* GetVertexShader();
		ID3D11PixelShader*  GetPixelShader();
		ID3DBlob*           GetByteCode() { return byteCode; }

	private:
		friend class ShaderManager;
		ID3D11DeviceChild* shader = nullptr;
		ShaderType         type = ShaderType::Unknown;
		PD3DBlob           byteCode;
	};



	class ShaderManager
	{
	public:
		ShaderManager(RenderDevicePtr _renderDevice);

		ShaderPtr CompileShaderFile(const char* filePath, const char* entryPoint, ShaderType shaderType);

	private:
		RenderDevicePtr renderDevice;
	};



} }
