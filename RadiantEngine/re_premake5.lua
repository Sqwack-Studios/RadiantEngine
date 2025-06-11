project "RadiantEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	warnings "High"
	
	links 
	{
		"quill"
	}


	includedirs
	{
		"include",
		"source",
		"../vendor/quill/include"
	}


	files
	{
		"source/**.hpp",
		"source/**.cpp",
		"source/**.h",
		"source/**.c",
		"include/**.h",
		"include/**.hpp"
	}

	defines{"NOMINMAX"}

	filter "system:windows"
		systemversion "latest"
		staticruntime "on"
		flags {"MultiProcessorCompile"}


	filter "configurations:Debug"
			defines "RE_DEBUG"
			symbols "on"
			optimize "off"
			linktimeoptimization "off"
			

	filter "configurations:Release"
			defines "RE_RELEASE"
			symbols "on"
			optimize "on"
			linktimeoptimization "on"
			
			

	filter "configurations:Shipping"
			defines "RE_SHIPPING"
			symbols "off"
			optimize "full"
			linktimeoptimization "on"
			
			