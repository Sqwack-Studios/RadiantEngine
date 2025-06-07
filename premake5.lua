workspace "RenderingEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Shipping"
	}

	startproject "ClientApp"

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	


	group "Logger"
		include "vendor/quill/quill_premake5.lua"


	group "Engine"
		include "RadiantEngine/re_premake5.lua"

	group "ClientApp"
		include "ClientApp/client_premake5.lua"

