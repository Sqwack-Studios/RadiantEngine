project "ShaderCompiler"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	warnings "High"


    links
    {
        "quill",
    }

    includedirs
    {
        "include",
        "source",
        "../vendor/dxc/include",
        "../vendor/quill/include"
    }

    files
    {
        "source/**.hpp",
		"source/**.cpp",
		"source/**.h",
		"source/**.c",
		"include/**.h",
		"include/**.hpp",
    }

	defines{ "PROJECT_COMPILE"}

    filter "system:windows"
		systemversion "latest"
		staticruntime "on"
		flags {"MultiProcessorCompile"}


	filter "configurations:Debug"
			symbols "on"
			optimize "off"
			linktimeoptimization "off"
			

	filter "configurations:Release"
			symbols "on"
			optimize "on"
			linktimeoptimization "on"
			
			

	filter "configurations:Shipping"
			symbols "off"
			optimize "full"
			linktimeoptimization "on"