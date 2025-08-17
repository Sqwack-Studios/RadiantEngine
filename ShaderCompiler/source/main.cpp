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

#define global static //use this when declaring a global variable
#define persistent static //use this when declaring a variable with persistent memory locally in a function 
#define internal static //use this when declaring a function to be internally linked to the scope of the translation unit

template<typename T>
struct Span
{
	T* data;
	size_t num;
};


//only for POD types
//no bounds checking anywhere
template<typename T, int32_t N>
struct StackArray
{
	T data[N];
	size_t num;

	void add(T item)
	{
		data[num] = item;
		num++;
	}

	T& operator[](size_t i) { return data[i]; }
	const T& operator[](size_t i) const { return data[i]; }
};


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
	constexpr const wchar_t* lut[]{
		L"vs_6_7",
		L"hs_6_7",
		L"ds_6_7",
		L"gs_6_7",
		L"ps_6_7",
		L"cs_6_7",
		L"ms_6_7",
		L"Unknown shader type"

	};
	return lut[type];
}


template<typename T>
internal void replace_all(T* first, const T* last, const T cmp, const T replace)
{
	T* cursor{ first };
	for (; cursor != last; ++cursor)
	{
		if (*cursor == cmp)
		{
			*cursor = replace;
		}
	}
}

template<typename T>
internal T* find_last(T* first, T* last, T cmp)
{
	//Because we find the last, we iterate on reverse. So the first time we find a valid cmp, we bail
	T* cursor{ last };
	T* ret{ };
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


template<typename T>
struct str
{
	T* data;
	size_t num;
	size_t cap;

	operator Span<T>()
	{
		return Span<T>{.data = data, .num = num };
	}

	template<typename T>
	operator Span<const T>()
	{
		return Span<const T>{.data = data, .num = num }; 
	}
};

using String = str<char>;
using WString = str<wchar_t>;

internal void string_to_wide(WString& dst, Span<const char> src)
{
	size_t convertedChars; //do something with this I guess?
	mbstowcs_s(&convertedChars, dst.data, dst.cap, src.data, src.num);
	dst.num = convertedChars - 1;
}


internal void wide_to_string(String& dst, Span<const wchar_t> src)
{
	size_t convertedChars;
	wcstombs_s(&convertedChars, dst.data, dst.cap, src.data, src.num);
	dst.num = convertedChars - 1;
}

using namespace std;

static constexpr size_t PATH_MAX_BUFFER{ 128 };
static constexpr size_t MAX_DEFINES{ 32 };
static constexpr size_t DEFINES_MAX_BUFFER{ 64 };
static constexpr size_t ENTRY_POINT_MAX_BUFFER{ 32 };

static constexpr const wchar_t OUTPUT_EXTENSION[]{ L".cso" };
static constexpr size_t OUTPUT_EXTENSION_SIZE{ _countof(OUTPUT_EXTENSION) - 1 };
static constexpr const wchar_t DEBUG_EXTENSION[]{ L".pdb" };
static constexpr size_t DEBUG_EXTENSION_SIZE{ _countof(DEBUG_EXTENSION) - 1 };
static constexpr const char SHADER_EXTENSION[]{ ".hlsl" };
static constexpr size_t SHADER_EXTENSION_SIZE{ _countof(SHADER_EXTENSION) - 1 };

struct shaderEntry
{
	const char* path; // the path is relative to the source code assets/shaders folder.
	const wchar_t* entryPoint;
	eShaderType type;
};

static constexpr shaderEntry entries[]{
	shaderEntry{.path = "basicVS.hlsl", .entryPoint = L"VSMain", .type = eShaderType::Vertex},
	shaderEntry{.path = "basicPS.hlsl", .entryPoint = L"PSMain", .type = eShaderType::Pixel},
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
	quill::Logger* logger;
	quill::Backend::start();
	
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


		logger = quill::Frontend::create_or_get_logger(
			"root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1", csinkConfig), loggerPattern);
	}

	//Initialize all the buffers
	wchar_t executablePathBuffer[MAX_PATH]{ L"\0" };
	wchar_t definesBuffers[MAX_DEFINES][DEFINES_MAX_BUFFER];
	char outputFolderBuffer[PATH_MAX_BUFFER]{ "\0" };

	//Initialize the strings
	WString ExecutablePath{ .data = executablePathBuffer, .num = 0, .cap = MAX_PATH };
	ExecutablePath.num = GetModuleFileName(NULL, ExecutablePath.data, MAX_PATH);
	{//Trim the executable part and leave the absolute directory path
		wchar_t* end{ ExecutablePath.data + ExecutablePath.num };
		wchar_t* lastBackSlash{ find_last<wchar_t>(ExecutablePath.data, ExecutablePath.data + ExecutablePath.num, '\\') };
		*lastBackSlash = '\0';
		ExecutablePath.num -= end - lastBackSlash;
	}

	String outputFolder{ .data = outputFolderBuffer, .num = 0, .cap = PATH_MAX_BUFFER };
	WString defines[MAX_DEFINES];
	std::int32_t numDefines{};
	for (int32_t i{}; i < MAX_DEFINES; i++)
	{
		defines[i] = WString{ .data = definesBuffers[i], .num = 0, .cap = DEFINES_MAX_BUFFER };
	}

	LOG_INFO(logger, "Welcome to the offline compiler!\nAvailable commands:\n"
		"-F : Output folder where all files will be saved. If a file is contained in a subfolder, then it will be written into -F + /path/to/subfolder/shadername.bin\n"
		"-D : An array of defines that will be globally setup ( -D DEFINE1=a DEFINE2=b DEFINE3=c ...)\n"
		"-Od : Pass this to disable optimizations\n"
		"-Zs : Enable debug information. This will generate extra .pdb files\n");

	enum compileFlags : std::uint8_t {
		F = 0x1,
		D = 0x2,
		Od = 0x4,
		Zs = 0x8
	};

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
			

			size_t bytesToCopy{ static_cast<int32_t>(arg.size()) > (PATH_MAX_BUFFER - 1) ? PATH_MAX_BUFFER - 1 : static_cast<int32_t>(arg.size()) };

			memcpy(outputFolder.data, arg.data(), bytesToCopy);
			outputFolder.data[bytesToCopy] = '\0';
			LOG_INFO(logger, "-F {}", outputFolder.data);

			replace_all(outputFolder.data, outputFolder.data + bytesToCopy, '/', '\\');

			outputFolder.num = bytesToCopy;

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

			for (std::int32_t j{ i }; j < lastDefineIdx; ++j)
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

				mbstowcs(defines[numDefines].data, arg.data(), arg.size());
				defines[numDefines].num = defineLength;

				numDefines++;

			}

			i += numDefines - 1; //we do this bc next iteration will add +1 to the i. I'm not sure this is the right approach to parse command lines
			flags |= compileFlags::D;

		}


		if (arg.compare("-Od") == 0)
			flags |= compileFlags::Od;
		
		
		if (arg.compare("-Zs") == 0)
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

	compileParams.data[0] = L"-Qstrip_debug";
	compileParams.data[1] = L"-Qstrip_priv";
	compileParams.data[2] = L"-Qstrip_reflect";
	compileParams.data[3] = L"-Qstrip_rootsignature";
	compileParams.num = NUM_PERMANENT_PARAMETERS;

	
	//We have to issue one compilation + dump per loop.
	//We check for the same flags multiple times because some parameters are not know before hand.
	//It'd be nice to have a SoA with each shader entry parameter, and compute all needed parameters at once, then just point to the right memory for each 
	//compilation entry. This way we are always calling the same function for each operation, which is more efficient. Not needed at all but it's fun.

	for (int32_t i{}; i < NUM_SHADER_ENTRIES; ++i, compileParams.num = NUM_PERMANENT_PARAMETERS)
	{
		const shaderEntry& entry{ entries[i] };
		
		//Get the entry point
		compileParams.add(L"-E");
		compileParams.add(entry.entryPoint);
		//Get the shadadd(
		compileParams.add(L"-T");
		compileParams.add(ShaderTypeToString(entry.type));

		str<const char> entryPath{.data = entry.path, .num = strlen(entry.path), .cap = PATH_MAX_BUFFER };

		if (entryPath.num > (PATH_MAX_BUFFER - 1))//leave space for the null-terminator
		{
			size_t diff{ entryPath.num - PATH_MAX_BUFFER };
			LOG_WARNING(logger, "The relative path of the shader entry {} exceeds the limit size by {} characters. Skipping...", entry.path + diff, diff);
			continue;
		}

		//define all the wide string buffers and strings
		wchar_t widePathBuffer[PATH_MAX_BUFFER];
		wchar_t outputPathBuffer[PATH_MAX_BUFFER];
		wchar_t debugPathBuffer[PATH_MAX_BUFFER];

		WString widePath{.data = widePathBuffer, .num = 0, .cap = PATH_MAX_BUFFER};
		WString outputPath{ .data = outputPathBuffer, .num = 0, .cap = PATH_MAX_BUFFER };
		WString debugPath{ .data = debugPathBuffer, .num = 0, .cap = PATH_MAX_BUFFER };

		int32_t namePathOffset{};//defines the offset from the start of the path to the start of the filename
		int32_t nameSize{};//defines the size of the name excluding the extension (.hlsl, .hlsli, etc...)
		int32_t extensionSize{};//defines the size of the extension, including the dot "."
		
		{//find the first backslash, if it exists

			const char* start{ entryPath.data };
			const char* end{ start + entryPath.num };
			const char* lastBackslash{ find_last<const char>(start, end, '/') };

			lastBackslash = lastBackslash ? lastBackslash : find_last<const char>(start, end, '\\');
			lastBackslash = lastBackslash ? lastBackslash : start;

			const char* extensionLocation{ find_last<const char>(lastBackslash, end, '.') };

			namePathOffset = lastBackslash == start ? 0 : static_cast<int32_t>(lastBackslash - start) + 1;
			nameSize = static_cast<int32_t>(extensionLocation - lastBackslash);
			extensionSize = end - extensionLocation;
		}

		//Convert to wide
		{
			string_to_wide(widePath, entryPath);
		}
		replace_all(widePath.data, widePath.data + widePath.num, L'/', L'\\');


		//If user defined output path, convert it to wide
		if (flags & compileFlags::F)
		{
			string_to_wide(outputPath, outputFolder);
		}

		//concatenate -F path + shader.path (without the name)
		{//append the slash and change the trailing extension
			size_t slashLoc{ outputPath.num };
			outputPath.data[slashLoc] = L'\\';
			size_t trimmedExtensionNum{ widePath.num - SHADER_EXTENSION_SIZE };
			memcpy(outputPath.data + slashLoc + 1, widePath.data, sizeof(wchar_t) * trimmedExtensionNum);
			memcpy(outputPath.data + slashLoc + 1 + trimmedExtensionNum, OUTPUT_EXTENSION, sizeof(wchar_t) * OUTPUT_EXTENSION_SIZE);

			outputPath.num = outputPath.num + trimmedExtensionNum + 1 /*the slash*/ + OUTPUT_EXTENSION_SIZE;
			outputPath.data[outputPath.num] = '\0';
			
		}

		
		if (int32_t diff{ static_cast<int32_t>(ExecutablePath.num + outputPath.num + 1 - MAX_PATH) }; diff > 0)
		{
			char buff1[MAX_PATH];
			char buff2[MAX_PATH];
			String d1{ .data = buff1, .num = 0, .cap = MAX_PATH };
			String d2{ .data = buff2, .num = 0, .cap = MAX_PATH };
		
			wide_to_string(d1, ExecutablePath);
			wide_to_string(d2, outputPath);
		
			LOG_WARNING(logger, "The full working path {}, concatenated with the relative output directory {} exceeds the limit of {} characters by {}, skipping...", 
				d1.data, d2.data , MAX_PATH, diff);
			continue;
		}
		
		//Set the shader name
		compileParams.add(widePath.data + namePathOffset); 
		//Output file
		compileParams.add(L"-Fo");
		compileParams.add(outputPath.data);
		//Check if we disable optimizations
		compileParams.add(flags & compileFlags::Od ? L"-Od" : L"-O3");
		
		//Check if we dump debug files, and output them with the same name as the binary output but with another extension
		if (flags & compileFlags::Zs)
		{
			compileParams.add(L"-Zs");
			wcscpy(debugPath.data, outputPath.data);
			wcscpy(debugPath.data + outputPath.num - DEBUG_EXTENSION_SIZE, DEBUG_EXTENSION);
			debugPath.num = outputPath.num;
			compileParams.add(L"-Fd");
			compileParams.add(debugPath.data);
		}
		
		//Add global defines. For now, not supporting permutations.
		if (flags & compileFlags::D)
		{
			compileParams.add(L"-D");
		
			for (int32_t i{}; i < numDefines; i++)
			{
				compileParams.add(defines[i].data);
			}
		}
		
		//Now we are ready to compile
		wchar_t shaderSourcePathBuffer[PATH_MAX_BUFFER];
		WString sourceShaderPath{ .data = shaderSourcePathBuffer, .num = 0 };
		{
			constexpr size_t folderLen{ _countof(SHADERS_FOLDER_PATHW) - 1 };
			memcpy(sourceShaderPath.data, SHADERS_FOLDER_PATHW, sizeof(wchar_t) * (folderLen));//dont copy the trailing '\0'
			sourceShaderPath.data[folderLen] = L'/';
			memcpy(sourceShaderPath.data + (folderLen + 1u), widePath.data, sizeof(wchar_t) * widePath.num);
			size_t num{(folderLen + 1u) + widePath.num };
			sourceShaderPath.data[num] = L'\0';
			sourceShaderPath.num = static_cast<int32_t>(num);
		}
		
		{
			char entryPointBuffer[ENTRY_POINT_MAX_BUFFER + 1];
			String tempEntryPoint{ .data = entryPointBuffer, .num = 0, .cap = ENTRY_POINT_MAX_BUFFER + 1 };
			wide_to_string(tempEntryPoint, Span<const wchar_t>{.data = entry.entryPoint, .num = wcslen(entry.entryPoint)});
		
			LOG_INFO(logger, "Shader \"{}\" compilation started.Entry point: \"{}\"", entry.path, tempEntryPoint.data);
		}
		
		std::ifstream file{ sourceShaderPath.data,std::ios::binary | std::ios::ate | std::ios::in };
		char shaderBlob[81920]; //81KB
		if (!file.is_open())
		{
			LOG_WARNING(logger, "File couldn't be opened. Skipping compilation...");
			continue;
			
		}
		uint32_t size{ static_cast<std::uint32_t>(file.tellg()) };
		file.seekg(0, std::ios::beg);
		file.read(shaderBlob, size);
		file.close();
		
		const DxcBuffer dxcBuff{
		.Ptr = shaderBlob,
		.Size = size,
		.Encoding = DXC_CP_UTF8
		};
		
		ComPtr<IDxcResult> compileResult;
		dxCompiler->Compile(&dxcBuff, compileParams.data, compileParams.num, nullptr, IID_PPV_ARGS(&compileResult));
		
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
		
		
		
		wchar_t fullPathBuffer[MAX_PATH]{L"\0"};
		WString fullPath{.data = fullPathBuffer, .num = 0, .cap = MAX_PATH };
		wcsncat(fullPath.data, ExecutablePath.data, ExecutablePath.num);
		fullPath.num = ExecutablePath.num;
		wcsncat(fullPath.data, outputPath.data, outputPath.num - (OUTPUT_EXTENSION_SIZE + nameSize));
		fullPath.num += outputPath.num - (OUTPUT_EXTENSION_SIZE + nameSize);
		replace_all(fullPath.data, fullPath.data + fullPath.num, L'/', L'\\');
		
		int webo{ SHCreateDirectory(NULL, fullPath.data) };
		
		ComPtr<IDxcBlob> outShader;
		compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outShader), nullptr);

		wcsncat(fullPath.data, outputPath.data + outputPath.num - (nameSize + OUTPUT_EXTENSION_SIZE), nameSize + OUTPUT_EXTENSION_SIZE);
		fullPath.num += nameSize + OUTPUT_EXTENSION_SIZE;
		
		replace_all(fullPath.data, fullPath.data + fullPath.num, L'/', L'\\');
		
		if (outShader)
		{
			FILE* fp{};
	
			int result{ _wfopen_s(&fp, fullPath.data, L"wb") };
			fwrite(outShader->GetBufferPointer(), outShader->GetBufferSize(), 1, fp);
			fclose(fp);
			
		
		}

		if (flags & compileFlags::Zs)
		{
			ComPtr<IDxcBlob> outPdb{};
			compileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&outPdb), nullptr);

			if (outPdb)
			{
				FILE* fp{};

				wcscpy(fullPath.data + fullPath.num - DEBUG_EXTENSION_SIZE, DEBUG_EXTENSION);
				int result{ _wfopen_s(&fp, fullPath.data, L"wb") };
				fwrite(outPdb->GetBufferPointer(), outPdb->GetBufferSize(), 1, fp);
				fclose(fp);
			}

		}
	}

	return 0;
}

