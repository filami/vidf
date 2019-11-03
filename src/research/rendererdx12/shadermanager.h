#pragma once

#include <dxcapi.h>

namespace vidf::dx12
{


	class ShaderManager;
	class Shader;
	typedef shared_ptr<Shader>     ShaderPtr;
	typedef Pointer<IDxcBlob>      PDxcBlob;
	typedef Pointer<IDxcLibrary>   PDxcLibrary;
	typedef Pointer<IDxcValidator> PDxcValidator;
	typedef Pointer<IDxcCompiler2> PDxcCompiler2;



	enum class ShaderType
	{
		Unknown,
		VertexShader,
		HullShader,
		DomainShader,
		GeometryShader,
		PixelShader,
		ComputeShader,
		Library,
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
		bool       IsValid() const { return state != ShaderState::Undefined; }
		ShaderType GetShaderType() const { return type; }
		void*      GetBufferPointer() /* const */ { return dxil ? const_cast<IDxcBlob*>(dxil.Get())->GetBufferPointer() : dxbc->GetBufferPointer(); }
		uint       GetBufferSize() /* const */ { return dxil ? const_cast<IDxcBlob*>(dxil.Get())->GetBufferSize() : dxbc->GetBufferSize(); }

	private:
		void Compile(ShaderManager* shaderManager);

	private:
		friend class ShaderManager;
		PDxcBlob    dxil;
		Pointer<ID3DBlob> dxbc;
		string      filePath;
		string      entryPoint;
		ShaderType  type = ShaderType::Unknown;
		ShaderState state = ShaderState::Undefined;
	};



	class ShaderManager
	{
	public:
		ShaderManager();

		ShaderPtr CompileShaderFile(const char* filePath, const char* entryPoint, ShaderType shaderType);

	private:
		friend class Shader;
		vector<ShaderPtr> shaders;
		PDxcLibrary   library;
		PDxcValidator validator;
		PDxcCompiler2 compiler;
	};


}
