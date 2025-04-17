workspace "RenderingEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Shipping"
	}


	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	

	startproject "ClientApp"


	group "Engine"
		include "RadiantEngine/re_premake5.lua"

	group "ClientApp"
		include "ClientApp/client_premake5.lua"