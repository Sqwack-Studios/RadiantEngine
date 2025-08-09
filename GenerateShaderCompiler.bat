@ECHO OFF
cl ShaderCompiler\source\main.cpp  /W3 /FeShaderCompiler.exe /I vendor\dxc\include /I vendor\quill\include /D UNICODE /D _UNICODE /std:c++20 /O2 /link Shell32.lib 