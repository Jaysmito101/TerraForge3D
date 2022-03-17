project "GLSLOptimizer"
	kind "StaticLib"
	language "C++"
	staticruntime "on"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/**.h",
		"src/**.h",
		"src/**.hpp",
		"src/**.cpp",
		"src/**.c"
	}

	includedirs{
		"include",
		"src",
		"src/getopt",
		"src/mesa",
		"src/util",
		"src/glsl"
	}
	
	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"


	filter "configurations:Release"
		runtime "Release"
		optimize "Full"	
