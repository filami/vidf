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



	void Shader::Compile(RenderDevicePtr renderDevice)
	{
		std::wstring wPath = ToWString(filePath.c_str());
		char nameBuffer[512]{};
		sprintf_s(nameBuffer, "%s:%s", filePath, entryPoint.c_str());

		const char* target = nullptr;
		switch (type)
		{
		case ShaderType::VertexShader: target = "vs_5_0"; break;
		case ShaderType::PixelShader: target = "ps_5_0"; break;
		};

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS;

		std::cout << filePath.c_str() << std::endl;

		PD3DBlob output;
		PD3DBlob _byteCode;
		HRESULT hr = D3DCompileFromFile(
			wPath.c_str(), nullptr, nullptr, entryPoint.c_str(), target, flags, 0,
			&_byteCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
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
		case ShaderType::PixelShader:
		{
			ID3D11PixelShader* gpuShader{};
			renderDevice->GetDevice()->CreatePixelShader(_byteCode->GetBufferPointer(), _byteCode->GetBufferSize(), nullptr, &gpuShader);
			shader = gpuShader;
			break;
		}
		};
		state = ShaderState::Ready;
		NameObject(shader, nameBuffer);
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
