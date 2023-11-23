-- premake.lua
	workspace ("Untitled Game")
	configurations ({ 
		"Debug", "Release"})
	platforms {"Win64"}
	
	project ("game")
	kind ("ConsoleApp")
	language ("C++")
	cppdialect ("C++20")
	-- Build Executable
	targetdir ("build/%{cfg.buildcfg}/%{cfg.architecture}")
	-- Intermediate Files
	objdir("bin/%{cfg.buildcfg}/%{cfg.architecture}")
	
	files ({ "**.hpp", "**.cpp", "**.h", "**.c" })
	links ({"SDL2.lib","SDL2main.lib"})
	includedirs ({"extern/includes","includes"})
	libdirs ({"extern/lib/"})
	
	filter ("configurations:Debug")
	defines ({ "DEBUG" })
	symbols ("On")
		filter ("configurations:Release")
		defines ({ "NDEBUG" })
		optimize ("On")

		filter  ("platforms:x64") 
		system ("Windows")
		architecture ("x86_64")