@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

for %%a in (%*) do set "%%a=1"

if not "%msvc%"=="1" if not "%clang%"=="1" (
    set msvc=1
    echo Using MSVC as default compiler...
)

if not "%debug%"=="1" if not "%release%"=="1" (
    set debug=1
    echo Using debug options by default...
)

if "%msvc%"=="1" (
    set clang=0
    echo [MSVC compile]
)

if "%clang%"=="1" (
    set msvc=0
    echo [Clang compile]
)

if "%debug%"=="1" (
    set release=0
    set output_dir=bin\debug
    echo [Debug build]
)
if "%release%"=="1" (
    set debug=0
    set output_dir=bin\release
    echo [Release build]
)

if "%verbose%"=="1" (
    set verbose=1
)
::MSVC warnings suppressed:
::C4324 structure padded due to alignment requirements (alignas)
::C4530 no standard exception handling

if "%verbose%"=="1" (
	set cl_warnings=/W4 
) else (
	set cl_warnings=/W0
)

set cl_common=/I RadiantEngine\include /I vendor\quill\include /I vendor\D3D12MemoryAllocator\src\ /I vendor\D3D12MemoryAllocator\include\ /I vendor\OpenFBX\src /nologo /FC %cl_warnings% /MP /Gw /MT /GR- /EHs- /EHc- /wd4324 /wd4530 /arch:SSE4.2 /D UNICODE /D _UNICODE /D _HAS_EXCEPTIONS_=0 /std:c++20

if "%debug%"=="1" (
    set compile_opts=/Od /Ob2 /Zi /D DEBUG_MODE=1 %cl_common%
)
if "%release%"=="1" (
    set compile_opts=/O2 /D RELEASE_MODE=1 %cl_common%
)

set cl_link=/link d3d12.lib dxgi.lib dxguid.lib user32.lib /INCREMENTAL:NO /NOCOFFGRPINFO

if not exist "%output_dir%" (
    mkdir "%output_dir%"
)

set out_flag=/out:%output_dir%\game.exe
set run_compilation= cl %compile_opts% ClientApp\source\main.cpp %cl_link% %out_flag% 

echo [Output] %output_dir%\game.exe
echo [compilation start]

if "%dump%"=="1" (
    goto dump:
) else (
    goto normal_compilation:
)


:dump
	for /f %%i in ('powershell -ExecutionPolicy Bypass -NoProfile -Command "Get-Date -Format yyyyMMdd_HH_mm_ss"') do set time_stamp=%%i
	set dump_file=%output_dir%\build_log_%time_stamp%.txt
	echo [dumping build log into] %dump_file%
	powershell -NoProfile -ExecutionPolicy Bypass -Command "%run_compilation% | Tee-Object -FilePath  %dump_file%"
goto finish

:normal_compilation
%run_compilation%
goto finish


:finish


