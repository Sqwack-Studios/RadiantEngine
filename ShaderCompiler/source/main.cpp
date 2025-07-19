//  Filename: main 
//	Author:	Daniel														
//	Date: 03/07/2025 19:39:51		
//  Sqwack-Studios													

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <wrl/client.h>
#include "dxc/dxcapi.h"

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include <cstdint>
#include <fstream>
#include <string_view>
#include <iostream>

#include "quill/Frontend.h"
#include "quill/Backend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

using namespace Microsoft::WRL;

//TODO: HRESULTS + logging
//TODO: D3D12 Calls + logging 

template<typename T, int32_t N>
struct StackArray
{
	T Data[N];
	int32_t Num;

	static constexpr int32_t Capacity() { return N; }
};

template<size_t N>
using LocalString = StackArray<char, N>;

template<size_t N>
using WLocalString = StackArray<wchar_t, N>;


enum eShaderType : std::uint8_t {
	Vertex = 0,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Compute,
	Mesh,
	NUM
};

//vs, ps, ds, hs, gs, cs, ms, lib; 6_0 - 6_7
static constexpr const wchar_t* ShaderTypeToString(eShaderType type)
{
	const wchar_t* ret{L"Unknown shader type"};

	switch (type)
	{
	case Vertex:
	{
		ret = L"vs_6_7";
		break;
	}
	case Hull:
	{
		ret = L"hs_6_7";
		break;
	}
	case Domain:
	{
		ret = L"ds_6_7";
		break;
	}
	case Geometry:
	{
		ret = L"gs_6_7";
		break;
	}
	case Pixel:
	{
		ret = L"ps_6_7";
		break;
	}
	case Compute:
	{
		ret = L"cs_6_7";
		break;
	}
	case Mesh:
	{
		ret = L"ms_6_7";
		break;
	}
	default:
		break;
	}

	return ret;
}

using namespace std;

static constexpr int32_t PATH_MAX_BUFFER{ 128 };
static constexpr int32_t MAX_DEFINES{ 32 };
static constexpr int32_t DEFINES_MAX_BUFFER{ 64 };
static constexpr int32_t ENTRY_POINT_MAX_BUFFER{ 31 };

struct shaderEntry
{
	const char* path; // the path is relative to the source code assets/shaders folder.
	const wchar_t* entryPoint;
	eShaderType type;
};

static constexpr shaderEntry entries[]{
	shaderEntry{ .path = "basic.hlsl", .entryPoint = L"VSMain", .type = eShaderType::Vertex},
	shaderEntry{ .path = "basic.hlsl", .entryPoint = L"PSMain", .type = eShaderType::Pixel},
	shaderEntry{ .path = "pollaycojones/basicDentro.hlsl", .entryPoint = L"VSMain", .type = eShaderType::Vertex }
};

static constexpr std::int32_t NUM_SHADER_ENTRIES{ sizeof(entries) / sizeof(shaderEntry) };

#ifdef PROJECT_COMPILE

static constexpr const wchar_t* dxLibPath{ L"../../../vendor/dxc/bin/dxcompiler.dll" };
static constexpr const wchar_t* shadersFolderPathW{ L"../../../assets/shaders/" };
static constexpr const char* shadersFolderPath{ "../../../assets/shaders/" };


#else

static constexpr const wchar_t* dxLibPath{ L"vendor/dxc/bin/dxcompiler.dll" };
static constexpr const wchar_t* shadersFolderPathW{ L"assets/shaders/" };
static constexpr const char* shadersFolderPath{ "assets/shaders/" };


#endif

// Arguments:
/*
* -F     Output folder where all files will be saved. If a file is contained in a subfolder, then it will be written into -F + /path/to/subfolder/shader.bin
* -D     An array of defines that will be globally setup ( -D DEFINE1=a DEFINE2=b DEFINE3=c ...)
* -Od    Pass this to disable optimizations
* -Zs    Enable debug information. This will generate extra .pdb files
*/
int main(int argc, char* argv[])
{
	quill::PatternFormatterOptions loggerPattern{
		std::string{ "%(time) [%(thread_id)] %(log_level) "
					"%(message) "},
		std::string{"%H:%M:%S.%Qms"},
		quill::Timezone::LocalTime,
		false
	};

	quill::ConsoleSinkConfig::Colours sinkColours{};

	sinkColours.apply_default_colours();

	sinkColours.assign_colour_to_log_level(quill::LogLevel::TraceL3, quill::ConsoleSinkConfig::Colours::white);
	sinkColours.assign_colour_to_log_level(quill::LogLevel::TraceL2, quill::ConsoleSinkConfig::Colours::white);
	sinkColours.assign_colour_to_log_level(quill::LogLevel::TraceL1, quill::ConsoleSinkConfig::Colours::white);
	sinkColours.assign_colour_to_log_level(quill::LogLevel::Debug, quill::ConsoleSinkConfig::Colours::green);
	sinkColours.assign_colour_to_log_level(quill::LogLevel::Info, quill::ConsoleSinkConfig::Colours::cyan);

	quill::ConsoleSinkConfig csinkConfig{};
	csinkConfig.set_colours(sinkColours);
	

	quill::Backend::start();
	quill::Logger* logger = quill::Frontend::create_or_get_logger(
		"root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1", csinkConfig), loggerPattern);

	
	LOG_INFO(logger, "Welcome to the offline compiler!\nAvailable commands:\n"
		"-F : Output folder where all files will be saved. If a file is contained in a subfolder, then it will be written into -F + /path/to/subfolder/shadername.bin\n"
		"-D : An array of defines that will be globally setup ( -D DEFINE1=a DEFINE2=b DEFINE3=c ...)\n"
		"-Od : Pass this to disable optimizations\n"
		"-Zs : Enable debug information. This will generate extra .pdb files\n");

	enum compileFlags : std::uint8_t {
		F  = 0x1,
		D  = 0x2,
		Od = 0x4,
		Zs = 0x8
	};



	char outputFolder[PATH_MAX_BUFFER]{"\0"};

	WLocalString<DEFINES_MAX_BUFFER> defines[MAX_DEFINES];
	std::int32_t numDefines{};
	std::uint8_t flags{};

	//parse arguments
	for (std::int32_t i{1}; i < argc; ++i)
	{
		std::string_view arg{ argv[i] };
		
		//-F command has been found. Next string should be a path.
		if (arg.compare("-F") == 0)//equal
		{
			//verify we don't run out of bounds

			char ignoreMsg[]{ "command was issued but no folder was provided! Ignoring" };
			
			arg = ((i + 1) >= argc) || (argv[i + 1] == "-") ? ignoreMsg : argv[++i];
			

			std::int32_t bytesToCopy{ static_cast<int32_t>(arg.size()) > (PATH_MAX_BUFFER - 1) ? PATH_MAX_BUFFER - 1 : static_cast<int32_t>(arg.size()) };

			memcpy(outputFolder, arg.data(), bytesToCopy);
			outputFolder[bytesToCopy] = '\0';
			LOG_INFO(logger, "-F {}", outputFolder);

			flags |= compileFlags::F;

			continue;
		}

		//-D command has been found. Next arguments will be the defines until a new command is found or we reach the end.
		if (arg.compare("-D") == 0)
		{
		
			std::int32_t lastDefineIdx{ ++i };

			while (lastDefineIdx < argc && argv[lastDefineIdx][0] != '-') { //count how many defines we got; either we reach the end or we find another command
				++lastDefineIdx;
			}
			


			if (lastDefineIdx == argc || (lastDefineIdx - i) == 0)
			{

				LOG_INFO(logger, "-D command was issued but no defines were provided. Ignoring...");
				continue;
			}

			for (std::int32_t j{i}; j < lastDefineIdx; ++j)
			{
				std::string_view arg{ argv[j] };
				
				std::int32_t defineLength{ static_cast<int32_t>(arg.size()) };

				const bool overflows{ defineLength > (DEFINES_MAX_BUFFER - 1) };

				if (overflows)
				{
					LOG_INFO(logger, "{} is too long, the DEFINE limit size is {} characters accounting for the null terminator. Ignoring...", arg, DEFINES_MAX_BUFFER);
					continue;
				}

				LOG_INFO(logger, "{} registered as a global define", arg);

				size_t convertedChars;
				mbstowcs_s(&convertedChars, defines[numDefines].Data, arg.data(), DEFINES_MAX_BUFFER);
				defines[numDefines].Num = static_cast<int32_t>(convertedChars);

				numDefines++;

			}

			i += numDefines - 1; //we do this bc next iteration will add +1 to the i. I'm not sure this is the right approach to parse command lines
			flags |= compileFlags::D;

		}


		if (arg.compare("-Od"))
			flags |= compileFlags::Od;
		
		
		if (arg.compare("-Zs"))
			flags |= compileFlags::Zs;
	}


	
	HINSTANCE dxcLibModule{ LoadLibrary(dxLibPath) };

	if (!dxcLibModule)
	{
		LOG_CRITICAL(logger, "dxcompiler.dll couldn't be loaded");
		return 0;
	}

	DxcCreateInstanceProc DxcCreateInstance{ (DxcCreateInstanceProc)GetProcAddress(dxcLibModule, "DxcCreateInstance") };
	
	ComPtr<IDxcCompiler3> dxCompiler;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxCompiler));
	

	
// Arguments:
/*
* -F     Output folder where all files will be saved. If a file is contained in a subfolder, then it will be written into -F + /path/to/subfolder/shader.bin
* -D     An array of defines that will be globally setup ( -D DEFINE1=a DEFINE2=b DEFINE3=c ...)
* -Od    Pass this to disable optimizations
* -Zs    Enable debug information. This will generate extra .pdb files
*/

	/*
	L"basic.hlsl",							    // Optional shader source file name for error reporting and for PIX shader source view.
	L"-E", L"MainVS",							// Entry point.
	L"-T", L"vs_6_6",							// Target. vs, ps, ds, hs, gs, cs, ms, lib; 6_0 - 6_7
	//L"-Zs",									// Enable debug information (slim format)
	L"-Zi",
	L"-Zpc",									//Pack matrices in column - major order.
	L"-Zpr",									//Pack matrices in row - major order.
	L"-Od",
	//L"-D", L"MYDEFINE=1",						// A single define...
	L"-Fo", L"../../bin/shaders/basic.bin",     // Optional. Stored in the pdb.
	L"-Fd", L"../../bin/shaders/basic.pdb",     // The file name of the pdb. This must either be supplied or the autogenerated file name must be used.
	L"-Qstrip_debug",
	L"-Qstrip_priv",
	L"-Qstrip_reflect",							// Strip reflection into a separate blob.
	L"-Qstrip_rootsignature"

	*/
	
	static constexpr int32_t MAX_COMPILE_PARAMS{ 128 };
	static constexpr int32_t NUM_PERMANENT_PARAMETERS{ 4 };
	StackArray<LPCWSTR, MAX_COMPILE_PARAMS> compileParams;
	
	
	//let's fill first parameters that are not configurable.

	compileParams.Data[0] = L"-Qstrip_debug";
	compileParams.Data[1] = L"-Qstrip_priv";
	compileParams.Data[2] = L"-Qstrip_reflect";
	compileParams.Data[3] = L"-Qstrip_rootsignature";
	compileParams.Num = NUM_PERMANENT_PARAMETERS;

	
	//We have to issue one compilation + dump per loop.
	//We check for the same flags multiple times because some parameters are not know before hand.
	//It'd be nice to have a SoA with each shader entry parameter, and compute all needed parameters at once, then just point to the right memory for each 
	//compilation entry. This way we are always calling the same function for each operation, which is more efficient. Not needed at all but it's fun.

	for (int32_t i{}; i < NUM_SHADER_ENTRIES; ++i, compileParams.Num = NUM_PERMANENT_PARAMETERS)
	{
		const shaderEntry& entry{ entries[i] };
		
		//Get the entry point
		compileParams.Data[compileParams.Num++] = L"-E";
		compileParams.Data[compileParams.Num++] = entry.entryPoint;
		//Get the shader type
		compileParams.Data[compileParams.Num++] = L"-T";
		compileParams.Data[compileParams.Num++] = ShaderTypeToString(entry.type);

		//Handle name, path and debug path if applies
		//A bit obscure as the output binaries and debug files will be named the same
		//except for the file extension, so it's a bit hacky in the end hehe
		WLocalString<PATH_MAX_BUFFER> WidePath;
		int32_t nameOffset{};

		WLocalString<PATH_MAX_BUFFER * 2> OutputPath{ .Data = L"\0", .Num = 0 };
		WLocalString<PATH_MAX_BUFFER * 2> DebugPath{ OutputPath };


		//find the first backslash 
		{
			size_t origSize = strlen(entry.path) + 1;
			size_t nameSize;
			const char* start{ entry.path };
			const char* end{ start + origSize }; //point to null terminator
			const char* it{ end };
			while (it > start) //reverse iterate
			{
				char test{ *it };
				if (test == '\\' || test == '/')
				{
					++it;
					break;
				}
				--it;
			}

			nameSize = end - it;
			nameOffset = static_cast<int32_t>(it - start);

			{
				size_t convertedChars; //do something with this I guess?
				mbstowcs_s(&convertedChars, WidePath.Data, PATH_MAX_BUFFER, entry.path, _TRUNCATE);
				WidePath.Num = static_cast<int32_t>(convertedChars) - 1;
			}

			//concatenate -F path + shader.path (without the name)
			if (flags & compileFlags::F)
			{
				size_t convertedChars;
				mbstowcs_s(&convertedChars, OutputPath.Data, strlen(outputFolder) + 1, outputFolder, _TRUNCATE);
				OutputPath.Num = static_cast<int32_t>(convertedChars) - 1;
			}

			wcscat_s(OutputPath.Data, WidePath.Data);
			OutputPath.Num = OutputPath.Num + WidePath.Num;
			wcscpy(OutputPath.Data + OutputPath.Num - 4, L"bin");
			OutputPath.Num--;//let's do this trick because we override hlsl for bin, which only has 1 letter difference. Effectively we go from shadername.hlsl to shadername.bin

		}

		//Set the shader name
		compileParams.Data[compileParams.Num++] = WidePath.Data + nameOffset;
		//Output file
		compileParams.Data[compileParams.Num++] = L"-Fo";
		compileParams.Data[compileParams.Num++] = OutputPath.Data;
		//Check if we disable optimizations
		compileParams.Data[compileParams.Num++] = flags & compileFlags::Od ? L"-Od" : L"-O3";

		//Check if we dump debug files, and output them with the same name as the binary output but with another extension
		if (flags & compileFlags::Zs)
		{
			compileParams.Data[compileParams.Num++] = L"-Zs";

			wcscat_s(DebugPath.Data, OutputPath.Data);
			wcscpy(DebugPath.Data + OutputPath.Num - 3, L"pdb");
			DebugPath.Num = OutputPath.Num;
			compileParams.Data[compileParams.Num++] = L"-Fd";
			compileParams.Data[compileParams.Num++] = DebugPath.Data;
		}

		//Add global defines. For now, not supporting permutations.
		if (flags & compileFlags::D)
		{
			compileParams.Data[compileParams.Num++] = L"-D";

			for (int32_t i{}; i < numDefines; i++)
			{
				compileParams.Data[compileParams.Num++] = defines[i].Data;
			}
		}

		//Now we are ready to compile
		LocalString<PATH_MAX_BUFFER> sourceShaderPath;

		strcpy_s(sourceShaderPath.Data, shadersFolderPath);
		strncat_s(sourceShaderPath.Data, entry.path, _TRUNCATE);
		sourceShaderPath.Num = static_cast<uint32_t>(strlen(sourceShaderPath.Data));

		{
			LocalString<ENTRY_POINT_MAX_BUFFER + 1> tempEntryPoint;
			wcstombs(tempEntryPoint.Data, entry.entryPoint, ENTRY_POINT_MAX_BUFFER);

			LOG_INFO(logger, "Shader \"{}\" compilation started.Entry point: \"{}\"", entry.path, tempEntryPoint.Data);
		}

		std::ifstream file{ sourceShaderPath.Data,std::ios::binary | std::ios::ate | std::ios::in };
		char shaderBlob[81920]; //4KB
		if (!file.is_open())
		{
			LOG_WARNING(logger, "File couldn't be opened. Skipping compilation...");
			continue;
			
		}
		std::uint32_t size{ static_cast<std::uint32_t>(file.tellg()) };
		file.seekg(0, std::ios::beg);
		file.read(shaderBlob, size);
		file.close();

		const DxcBuffer dxcBuff{
		.Ptr = shaderBlob,
		.Size = size,
		.Encoding = DXC_CP_UTF8
		};

		ComPtr<IDxcResult> compileResult;
		dxCompiler->Compile(&dxcBuff, compileParams.Data, compileParams.Num, nullptr, IID_PPV_ARGS(&compileResult));

		HRESULT hrStatus;
		compileResult->GetStatus(&hrStatus);

		ComPtr<IDxcBlobUtf8> pErrors = nullptr;
		compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
		

		if (!SUCCEEDED(hrStatus))
		{
			LOG_WARNING(logger, "Compilation failed, dumping errors and skipping...\n{}", pErrors->GetStringPointer());
			continue;
		}


		if (pErrors->GetStringLength() > 0)
		{
			LOG_WARNING(logger, "Compilation succeeded, warnings have been generated\n{}",pErrors->GetStringPointer());
		}
		else
		{
			LOG_INFO(logger, "Compilation succeeded!");
		}

		ComPtr<IDxcBlob> outShader;

		compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outShader), nullptr);

		//Get the .pdb if applies
		//Create folder if doesn't exist
		//write binarie
		//write pdb if applies
	}

	
	
	
	
	


	
	


	//ComPtr<IDxcBlob> pShader = nullptr;
	//ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
	//compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
	//
	
	//
	//auto webo{ pErrors->GetStringPointer() };
	//auto webo2{ pShaderName->GetStringPointer() };
	//// Note that d3dcompiler would return null if no errors or warnings are present.
	//// IDxcCompiler3::Compile will always return an error buffer, but its length
	//// will be zero if there are no warnings or errors.
	//if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	//	LOG_INFO(logger, "Compiling errors: {}", webo);
	//
	//
	//compileResult->GetStatus(&hrStatus);
	//if (FAILED(hrStatus))
	//{
	//	LOG_CRITICAL(logger, "Compilation Failed\n");
	//	return 0;
	//}
	//
	//ComPtr<IDxcBlob> pPDB = nullptr;
	//ComPtr<IDxcBlobUtf16> pPDBName = nullptr;
	//compileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), &pPDBName);
	//{
	//	FILE* fp = NULL;
	//
	//	// Note that if you don't specify -Fd, a pdb name will be automatically generated.
	//	// Use this file name to save the pdb so that PIX can find it quickly.
	//	_wfopen_s(&fp, pPDBName->GetStringPointer(), L"wb");
	//	fwrite(pPDB->GetBufferPointer(), pPDB->GetBufferSize(), 1, fp);
	//	fclose(fp);
	//}
	//if (pShader != nullptr)
	//{
	//	FILE* fp = NULL;
	//
	//	_wfopen_s(&fp, pShaderName->GetStringPointer(), L"wb");
	//	fwrite(pShader->GetBufferPointer(), pShader->GetBufferSize(), 1, fp);
	//	fclose(fp);
	//
	//}
	//FreeLibrary(dxcLibModule);
	return 0;
}

