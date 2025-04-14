workspace "RenderingEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Shipping"
	}


	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	

	startproject "RadiantEngine"


	group "Engine"
		include "RadiantEngine/re_premake5.lua"
