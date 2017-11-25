solution "td"
   location "generated"
   configurations { "Debug", "Release", "Ship" }
   platforms {"x64"}

project "td"
   kind "ConsoleApp"
   language "C++"
   targetdir "generated/bin/%{cfg.buildcfg}"
   includedirs { "src" }
   files { "main.cpp" }
   characterset "MBCS"

   filter "configurations:Debug"
      defines { "DEBUG" }
      flags { "Symbols" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
  
   filter "configurations:Ship"
      defines { "NDEBUG", "NO_ASSERT" }
      optimize "Speed"
      flags { "LinkTimeOptimization", "NoRuntimeChecks", "StaticRuntime" }
