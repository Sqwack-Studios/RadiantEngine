project "ClientApp "
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	warnings "High"

    includedirs
	{
		"../RadiantEngine/include",
		"../RadiantEngine/shaders",
		"../vendor/quill/include",
		"../vendor/D3D12MemoryAllocator/include"
	}


	files
	{
		"source/**.cpp",
		"../RadiantEngine/include/**.hpp",
		"../RadiantEngine/include/**.h",
		"../vendor/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
	}


	links
	{
		--"RadiantEngine",
		"quill",
		"dxgi",
		"d3d12",
		"dxguid",
		"../vendor/dxc/bin/dxil.dll",
		"../vendor/dxc/bin/dxcompiler.dll"
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
			
		
    
