project "ClientApp "
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	warnings "High"

    includedirs
	{
		"../RadiantEngine/includes",
	}


	files
	{
		"source/**.cpp",
		"../RadiantEngine/includes/**.hpp",
		"../RadiantEngine/includes/**.h"
	}


	links
	{
		"RadiantEngine"
	}

		filter "system:windows"
		systemversion "latest"
		staticruntime "on"
		flags {"MultiProcessorCompile"}


	filter "configurations:Debug"
			defines "APP_DEBUG"
			symbols "on"
			optimize "off"
			linktimeoptimization "off"
			

	filter "configurations:Release"
			defines "APP_RELEASE"
			symbols "on"
			optimize "on"
			linktimeoptimization "on"
			
			

	filter "configurations:Shipping"
			defines "APP_SHIPPING"
			symbols "off"
			optimize "full"
			linktimeoptimization "on"
			
		
    
