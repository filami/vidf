#include "pch.h"
#include "shadermanager.h"
#include "common.h"

namespace vidf::dx12
{


	void PrintOperationResult(PDxcLibrary library, Pointer<IDxcOperationResult> result)
	{
		Pointer<IDxcBlobEncoding> buffer;
		Pointer<IDxcBlobEncoding> bufferUTF8;
		AssertHr(result->GetErrorBuffer(&buffer.Get()));
		library->GetBlobAsUtf8(buffer, &bufferUTF8.Get());
		if (bufferUTF8->GetBufferSize() != 0)
			cout << (const char*)bufferUTF8->GetBufferPointer() << endl;
	}



	bool IsOperationResultOk(Pointer<IDxcOperationResult> result)
	{
		HRESULT hr;
		AssertHr(result->GetStatus(&hr));
		return hr == S_OK;
	}



	void Shader::Compile(ShaderManager* shaderManager)
	{
		Assert(shaderManager != nullptr);

		// TODO - DXIL compiler bugged out with csSvgfAtrous
		if (type == ShaderType::Library)
		{
			wstring wPath = ToWString(filePath.c_str());
			wstring wEntryPoint = ToWString(entryPoint.c_str());
			dxil == nullptr;
			state = ShaderState::Error;	// set to error by default and only set to ready if code compiles

			VI_INFO(Format("DXC - Compiling shader \"%0\"\n", filePath));

			ifstream ifs{ filePath, ios::binary };
			if (!ifs)
			{
				VI_ERROR("File not found");
				return;
			}
			vector<char> program;
			ifs.seekg(0, ios::end);
			uint fileSz = uint(ifs.tellg());
			ifs.seekg(0, ios::beg);
			program.resize(fileSz);
			ifs.read(program.data(), fileSz);

			Pointer<IDxcBlobEncoding> source;
			shaderManager->library->CreateBlobWithEncodingFromPinned(program.data(), program.size(), 0, &source.Get());

			const wchar_t* target = nullptr;
			switch (type)
			{
			case ShaderType::VertexShader:   target = L"vs_6_0"; break;
			case ShaderType::GeometryShader: target = L"gs_6_0"; break;
			case ShaderType::PixelShader:    target = L"ps_6_0"; break;
			case ShaderType::ComputeShader:  target = L"cs_6_0"; break;
			case ShaderType::Library:        target = L"lib_6_3"; break;
			};

			LPCWSTR args[] = { /* L" /Zi  " , L" -Qembed_debug ", */ L" -HV 2017 -O4 -Zpr " };

			Pointer<IDxcOperationResult> compileResult;
			AssertHr(shaderManager->compiler->Compile(
				source,
				wPath.c_str(),
				wEntryPoint.empty() ? L"" : wEntryPoint.c_str(),
				target,
				// args, _countof(args),
				nullptr, 0,
				nullptr, 0,       // name/value defines and their count
				nullptr,          // handler for #include directives
				&compileResult.Get()));

			PrintOperationResult(shaderManager->library, compileResult);

			if (!IsOperationResultOk(compileResult))
				return;

			Pointer<IDxcOperationResult> validationResult;
			Pointer<IDxcBlob> shaderCode;
			AssertHr(compileResult->GetResult(&shaderCode.Get()));

			AssertHr(shaderManager->validator->Validate(shaderCode, DxcValidatorFlags_InPlaceEdit, &validationResult.Get()));
			PrintOperationResult(shaderManager->library, validationResult);

			if (!IsOperationResultOk(validationResult))
				return;

			dxil = shaderCode;

			state = ShaderState::Ready;
		}
		else
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

			VI_INFO(Format("FXC47 - Compiling shader \"%0\" %1 %2\n", filePath, entryPoint, target));

			Pointer<ID3DBlob> output;
			Pointer<ID3DBlob> _byteCode;
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

			dxbc = _byteCode;

			state = ShaderState::Ready;
		}
	}



	ShaderManager::ShaderManager()
	{
		HMODULE dxilModule = LoadLibraryA(".\\data\\dxil.dll");
		HMODULE dxcModule = LoadLibraryA(".\\data\\dxcompiler.dll");
		if (!dxcModule || !dxilModule)
			__debugbreak();
		auto dxcCreateInstance = (DxcCreateInstanceProc)(GetProcAddress(dxcModule, "DxcCreateInstance"));
		if (!dxcCreateInstance)
			__debugbreak();
		dxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library.Get()));
		dxcCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(&validator.Get()));
		dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler.Get()));
	}



	ShaderPtr ShaderManager::CompileShaderFile(const char* filePath, const char* entryPoint,  ShaderType shaderType)
	{
		ShaderPtr shader = std::make_shared<Shader>();
		shader->filePath = filePath;
		if (entryPoint)
			shader->entryPoint = entryPoint;
		shader->type = shaderType;

		shader->Compile(this);

		shaders.push_back(shader);

		return shader;
	}



}
