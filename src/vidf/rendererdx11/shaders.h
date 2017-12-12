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


	enum class ShaderState
	{
		Undefined,
		Ready,
		Error,
	};



	class Shader
	{
	public:
		~Shader();
		ID3D11VertexShader*  GetVertexShader();
		ID3D11PixelShader*   GetPixelShader();
		ID3D11ComputeShader* GetComputeShader();
		PD3DBlob             GetByteCode() { return byteCode; }

		void Compile(RenderDevicePtr renderDevice);

		bool       IsValid() const       { return state != ShaderState::Undefined; }
		ShaderType GetShaderType() const { return type; }

	private:
		friend class ShaderManager;
		std::string        filePath;
		std::string        entryPoint;
		ID3D11DeviceChild* shader = nullptr;
		ShaderType         type = ShaderType::Unknown;
		ShaderState        state = ShaderState::Undefined;
		PD3DBlob           byteCode;
	};



	class ShaderManager
	{
	public:
		ShaderManager(RenderDevicePtr _renderDevice);

		ShaderPtr CompileShaderFile(const char* filePath, const char* entryPoint, ShaderType shaderType);
		void      RecompileShaders();

	private:
		RenderDevicePtr        renderDevice;
		std::vector<ShaderPtr> shaders;
	};



} }
