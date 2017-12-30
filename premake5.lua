solution "td"
   location "generated"
   configurations { "Debug", "Release", "Ship" }
   platforms {"x64"}

project "td"
   kind "ConsoleApp"
   language "C++"
   targetdir "generated/bin/%{cfg.buildcfg}"
   includedirs { "src" }
   files { "main.cpp", "premake.lua" }
   characterset "MBCS"

   filter { "system:macosx", "language:C++" }
     buildoptions "-std=c++11 -stdlib=libc++"
     linkoptions "-stdlib=libc++"

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
  
   filter "configurations:Ship"
      defines { "NDEBUG", "NO_ASSERT" }
      optimize "Speed"
      flags { "LinkTimeOptimization", "NoRuntimeChecks", "StaticRuntime" }
