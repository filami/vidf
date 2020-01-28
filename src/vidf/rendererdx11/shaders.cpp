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



	ID3D11GeometryShader* Shader::GetGeometryShader()
	{
		assert(type == ShaderType::GeometryShader);
		return static_cast<ID3D11GeometryShader*>(shader);
	}



	ID3D11PixelShader* Shader::GetPixelShader()
	{
		assert(type == ShaderType::PixelShader);
		return static_cast<ID3D11PixelShader*>(shader);
	}



	ID3D11ComputeShader* Shader::GetComputeShader()
	{
		assert(type == ShaderType::ComputeShader);
		return static_cast<ID3D11ComputeShader*>(shader);
	}



	void Shader::Compile(RenderDevicePtr renderDevice)
	{
		std::wstring wPath = ToWString(filePath.c_str());
		char nameBuffer[512]{};
		sprintf_s(nameBuffer, "%s:%s", filePath, entryPoint.c_str());

		const char* target = nullptr;
		switch (type)
		{
		case ShaderType::VertexShader: target = "vs_5_0"; break;
		case ShaderType::GeometryShader: target = "gs_5_0"; break;
		case ShaderType::PixelShader: target = "ps_5_0"; break;
		case ShaderType::ComputeShader: target = "cs_5_0"; break;
		};

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 /* | D3DCOMPILE_WARNINGS_ARE_ERRORS */ | D3DCOMPILE_DEBUG /*| D3DCOMPILE_SKIP_OPTIMIZATION*/;
		
		VI_INFO(Format("Compiling shader \"%0\"\n", filePath));
		
		PD3DBlob output;
		PD3DBlob _byteCode;
		HRESULT hr = D3DCompileFromFile(
			wPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), target, flags, 0,
			&_byteCode.Get(), &output.Get());
		if (output)
		{
			if (hr == S_OK)
				VI_WARNING((const char*)output->GetBufferPointer())
			else
				VI_ERROR((const char*)output->GetBufferPointer())
		}
		if (hr != S_OK)
			return;

		switch (type)
		{
		case ShaderType::VertexShader:
		{
			ID3D11VertexShader* gpuShader{};
			renderDevice->GetDevice()->CreateVertexShader(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader = gpuShader;
			byteCode = _byteCode;
			break;
		}
		case ShaderType::GeometryShader:
		{
			ID3D11GeometryShader* gpuShader{};
			renderDevice->GetDevice()->CreateGeometryShader(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader = gpuShader;
			break;
		}
		case ShaderType::PixelShader:
		{
			ID3D11PixelShader* gpuShader{};
			renderDevice->GetDevice()->CreatePixelShader(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader = gpuShader;
			break;
		}
		case ShaderType::ComputeShader:
		{
			ID3D11ComputeShader* gpuShader{};
			renderDevice->GetDevice()->CreateComputeShader(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader = gpuShader;
			break;
		}
		};
		state = ShaderState::Ready;
		NameObject(shader, nameBuffer);

		ID3D11ShaderReflection* _reflection;
		D3DReflect(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&_reflection);
		reflection = _reflection;
	}



	ShaderManager::ShaderManager(RenderDevicePtr _renderDevice)
		: renderDevice(_renderDevice)
	{
	}



	ShaderPtr ShaderManager::CompileShaderFile(const char* path, const char* entryPoint, ShaderType shaderType)
	{
		ShaderPtr shader = std::make_shared<Shader>();
		shader->filePath = path;
		shader->entryPoint = entryPoint;
		shader->type = shaderType;

		shader->Compile(renderDevice);

		shaders.push_back(shader);

		return shader;
	}



	void ShaderManager::RecompileShaders()
	{
		for (auto& shader : shaders)
			shader->Compile(renderDevice);
	}



} }
