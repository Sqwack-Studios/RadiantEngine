
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

	include "vendor/quill/quill_premake5.lua"


	local quillDefines = runQuillCmake()

	defines(quillDefines)

	externalproject "quill"
			location "vendor/quill"
			uuid "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"
			kind "StaticLib"
			language "C++"



	include "RadiantEngine/re_premake5.lua"

	include "ClientApp/client_premake5.lua"

