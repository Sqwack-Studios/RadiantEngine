//  Filename: main 
//	Author:	Daniel														
//	Date: 03/07/2025 19:39:51		
//  Sqwack-Studios													

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <wrl/client.h>
#include "dxc/dxcapi.h"
#include <ShlObj_core.h>

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


//Length accounts for the number of characters to check, but must not be longer than the size of the string. This function
//doesn't check for the null terminator character
static void ReplaceAllCharOccurrences(char* string, int32_t length, const char cmp, const char replace)
{
	char* cursor{ string };
	char* end{ string + length };

	while (cursor < end)
	{
		char val{ *cursor };

		if (val == cmp)
		{
			*cursor = replace;
		}

		cursor++;
	}
}

template<typename T>
static void replace_all(T* first, const T* last, const T cmp, const T replace)
{
	T* cursor{ first };
	for(; cursor != last; ++cursor)
	{
		if (*cursor == cmp)
		{
			*cursor = replace;
		}
	}
}


template<typename T>
static T* find_last(const T* first, T* last, const T cmp)
{
	//Because we find the last, we iterate on reverse. So the first time we find a valid cmp, we bail
	T* cursor{ last };

	T* ret{};
	for (; cursor != first; cursor--)
	{
		if (*cursor == cmp)
		{
			ret = cursor;
			break;
		}
	}

	return ret;
}

using namespace std;

static constexpr int32_t PATH_MAX_BUFFER{ 128 };
static constexpr int32_t MAX_DEFINES{ 32 };
static constexpr int32_t DEFINES_MAX_BUFFER{ 64 };
static constexpr int32_t ENTRY_POINT_MAX_BUFFER{ 31 };
static constexpr const wchar_t OUTPUT_EXTENSION[]{ L"bin" };
static constexpr int32_t OUTPUT_EXTENSION_SIZE{ _countof(OUTPUT_EXTENSION) - 1 };
static constexpr const char SHADER_EXTENSION[]{ "hlsl" };
static constexpr int32_t SHADER_EXTENSION_SIZE{ _countof(SHADER_EXTENSION) - 1 };

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

static constexpr const wchar_t* DX_LIB_PATH{ L"../../../vendor/dxc/bin/dxcompiler.dll" };
static constexpr const wchar_t SHADERS_FOLDER_PATHW[]{ L"../../../assets/shaders" };
static constexpr const char SHADERS_FOLDER_PATH[]{ "../../../assets/shaders" };


#else

static constexpr const wchar_t* DX_LIB_PATH{ L"vendor/dxc/bin/dxcompiler.dll" };
static constexpr const wchar_t SHADERS_FOLDER_PATHW[]{ L"assets/shaders" };
static constexpr const char SHADERS_FOLDER_PATH[]{ "assets/shaders" };


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


	WLocalString<MAX_PATH> ExecutablePath;
	ExecutablePath.Num  = GetModuleFileName(NULL, ExecutablePath.Data, MAX_PATH);

	{//Trim the executable part and leave the absolute directory path
		wchar_t* end{ ExecutablePath.Data + ExecutablePath.Num };
		wchar_t* lastBackSlash{ find_last<wchar_t>(ExecutablePath.Data, ExecutablePath.Data + ExecutablePath.Num, '\\') };
		*lastBackSlash = '\0';
		ExecutablePath.Num -= end - lastBackSlash;
	}

	
	char outputFolder[PATH_MAX_BUFFER]{"\0"};
	int32_t outputFolderSize{};

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

			replace_all(outputFolder, outputFolder + bytesToCopy, '/', '\\');


			outputFolderSize = bytesToCopy;
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


	
	HINSTANCE dxcLibModule{ LoadLibrary(DX_LIB_PATH) };

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
		
		LocalString<PATH_MAX_BUFFER> entryPath;
		entryPath.Num = static_cast<int32_t>(strlen(entry.path));
		if (entryPath.Num > PATH_MAX_BUFFER)
		{
			int32_t diff{ entryPath.Num - PATH_MAX_BUFFER };
			LOG_WARNING(logger, "The relative path of the shader entry {} exceeds the limit size by {} characters. Skipping...", entry.path + diff, diff);
			continue;
		}
		strncpy(entryPath.Data, entry.path, PATH_MAX_BUFFER);

		WLocalString<PATH_MAX_BUFFER> widePath;
		WLocalString<PATH_MAX_BUFFER * 2> outputPath{ .Data = L"\0", .Num = 0 };
		WLocalString<PATH_MAX_BUFFER * 2> debugPath{ outputPath };
		int32_t namePathOffset{};
		int32_t nameSize{};

		//Get the entry point
		compileParams.Data[compileParams.Num++] = L"-E";
		compileParams.Data[compileParams.Num++] = entry.entryPoint;
		//Get the shader type
		compileParams.Data[compileParams.Num++] = L"-T";
		compileParams.Data[compileParams.Num++] = ShaderTypeToString(entry.type);

		
		{//find the first backslash, if it exists

			const char* start{ entryPath.Data };
			const char* end{ start + entryPath.Num };
			char* lastBackslash{ (char*)start };

			for (; lastBackslash != start; lastBackslash--)
			{
				char test{ *lastBackslash };
				if (test == '\\' || test == '/')
				{
					break;
				}
			}

			nameSize = static_cast<int32_t>(end - lastBackslash);
			namePathOffset = lastBackslash == start ? 0 : static_cast<int32_t>(lastBackslash - start) + 1;
		}


		{
			size_t convertedChars; //do something with this I guess?
			mbstowcs_s(&convertedChars, widePath.Data, PATH_MAX_BUFFER, entryPath.Data, _TRUNCATE);
			widePath.Num = static_cast<int32_t>(convertedChars) - 1;
		}

		if (flags & compileFlags::F)
		{
			size_t convertedChars;
			mbstowcs_s(&convertedChars, outputPath.Data, PATH_MAX_BUFFER * 2, outputFolder, _TRUNCATE);
			outputPath.Num = static_cast<int32_t>(convertedChars) - 1;
		}

		//concatenate -F path + shader.path (without the name)
		{//append the slash and change the trailing extension
			int32_t slashLoc{ outputPath.Num };
			outputPath.Data[slashLoc] = L'/';
			int32_t trimmedExtensionNum{ widePath.Num - SHADER_EXTENSION_SIZE };
			memcpy(outputPath.Data + slashLoc + 1, widePath.Data, sizeof(wchar_t) * trimmedExtensionNum);
			memcpy(outputPath.Data + slashLoc + 1 + trimmedExtensionNum, OUTPUT_EXTENSION, sizeof(wchar_t) * OUTPUT_EXTENSION_SIZE);

			outputPath.Num = outputPath.Num + trimmedExtensionNum + 1 /*the slash*/ + OUTPUT_EXTENSION_SIZE;
			outputPath.Data[outputPath.Num] = '\0';
			
		}

		
		if (int32_t diff{ ExecutablePath.Num + outputPath.Num + 1 - MAX_PATH }; diff > 0)
		{
			LocalString<MAX_PATH> d1;
			LocalString<MAX_PATH> d2;

			wcstombs(d1.Data, ExecutablePath.Data, MAX_PATH);
			wcstombs(d2.Data, outputPath.Data, MAX_PATH);

			LOG_WARNING(logger, "The full working path {}, concatenated with the relative output directory {} exceeds the limit of {} characters by {}, skipping...", 
				d1.Data, d2.Data , MAX_PATH, diff);
			continue;
		}


		//Set the shader name
		compileParams.Data[compileParams.Num++] = widePath.Data + namePathOffset;
		//Output file
		compileParams.Data[compileParams.Num++] = L"-Fo";
		compileParams.Data[compileParams.Num++] = outputPath.Data;
		//Check if we disable optimizations
		compileParams.Data[compileParams.Num++] = flags & compileFlags::Od ? L"-Od" : L"-O3";

		//Check if we dump debug files, and output them with the same name as the binary output but with another extension
		if (flags & compileFlags::Zs)
		{
			compileParams.Data[compileParams.Num++] = L"-Zs";

			wcscat_s(debugPath.Data, outputPath.Data);
			wcscpy(debugPath.Data + outputPath.Num - 3, L"pdb");
			debugPath.Num = outputPath.Num;
			compileParams.Data[compileParams.Num++] = L"-Fd";
			compileParams.Data[compileParams.Num++] = debugPath.Data;
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
		WLocalString<PATH_MAX_BUFFER> sourceShaderPath;
		{
			constexpr size_t folderLen{ _countof(SHADERS_FOLDER_PATHW) - 1 };
			memcpy(sourceShaderPath.Data, SHADERS_FOLDER_PATHW, sizeof(wchar_t) * (folderLen));//dont copy the trailing '\0'
			sourceShaderPath.Data[folderLen] = L'/';
			memcpy(sourceShaderPath.Data + (folderLen + 1u), widePath.Data + namePathOffset, sizeof(wchar_t) * nameSize);
			size_t num{(folderLen + 1u) + nameSize };
			sourceShaderPath.Data[num] = L'\0';
			sourceShaderPath.Num = static_cast<int32_t>(num);
		}

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

		WLocalString<MAX_PATH> fullPath{};

		wcsncat(fullPath.Data, ExecutablePath.Data, ExecutablePath.Num);
		fullPath.Num = ExecutablePath.Num;
		//wcsncat(fullPath.Data, outputPath.Data, concatOutputPathSize - 1);
		//fullPath.Num += concatOutputPathSize - 1;

		//TODO: Rework the paths so the always start with backslash but never finish with one
		//TODO: Get executable path, not working directory
		int webo{ SHCreateDirectory(NULL, fullPath.Data) };


		//Get the .pdb if applies
		//Create folder if doesn't exist
		//write binarie
		//write pdb if applies
	}

	

	//int webo{ SHCreateDirectory(NULL, L"polla/metida/en/mi/culeteeee") };


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


	return 0;
}

