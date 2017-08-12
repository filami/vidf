#include "pch.h"
#include "shaders.h"
#include "renderdevice.h"

namespace vidf { namespace dx11 {



	Shader::~Shader()
	{
		if (shader)
			shader->Release();
	}



	ID3D11VertexShader* Shader::GetVertexShader()
	{
		assert(type == ShaderType::VertexShader);
		return static_cast<ID3D11VertexShader*>(shader);
	}



	ID3D11PixelShader* Shader::GetPixelShader()
	{
		assert(type == ShaderType::PixelShader);
		return static_cast<ID3D11PixelShader*>(shader);
	}



	ShaderManager::ShaderManager(RenderDevicePtr _renderDevice)
		: renderDevice(_renderDevice)
	{
	}



	ShaderPtr ShaderManager::CompileShaderFile(const char* path, const char* entryPoint, ShaderType shaderType)
	{
		std::wstring wPath = ToWString(path);
		char nameBuffer[512]{};
		sprintf_s(nameBuffer, "%s:%s", path, entryPoint);

		const char* target = nullptr;
		switch (shaderType)
		{
		case ShaderType::VertexShader: target = "vs_5_0"; break;
		case ShaderType::PixelShader: target = "ps_5_0"; break;
		};

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS;

		std::cout << path << std::endl;
		
		PD3DBlob output;
		PD3DBlob byteCode;
		HRESULT hr = D3DCompileFromFile(
			wPath.c_str(), nullptr, nullptr, entryPoint, target, flags, 0,
			&byteCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
		if (hr != S_OK)
			return ShaderPtr();

		ShaderPtr shader = std::make_shared<Shader>();
		shader->type = shaderType;
		shader->byteCode = byteCode;

		switch (shaderType)
		{
		case ShaderType::VertexShader:
		{
			ID3D11VertexShader* gpuShader{};
			renderDevice->GetDevice()->CreateVertexShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader->shader = gpuShader;
			break;
		}
		case ShaderType::PixelShader:
		{
			ID3D11PixelShader* gpuShader{};
			renderDevice->GetDevice()->CreatePixelShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader->shader = gpuShader;
			break;
		}
		};
				
		NameObject(shader->shader, nameBuffer);

		std::cout << std::endl;

		return shader;
	}



} }
