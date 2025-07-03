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

#include "quill/Frontend.h"
#include "quill/Backend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"


int main(int argc, char* argv[])
{

	quill::Backend::start();
	quill::Logger* logger = quill::Frontend::create_or_get_logger(
		"root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

	LOG_INFO(logger, "Hello from {}!", "Quill");
	
	HINSTANCE dxcLibModule{ LoadLibrary(L"../../../vendor/dxc/bin/dxcompiler.dll") };

	if (dxcLibModule)
	{
		LOG_CRITICAL(logger, "dxcompiler.dll couldn't be loaded");
		return 0;
	}

	DxcCreateInstanceProc DxcCreateInstance{ (DxcCreateInstanceProc)GetProcAddress(dxcLibModule, "DxcCreateInstance") };
	FreeLibrary(dxcLibModule);


	return 0;
}

