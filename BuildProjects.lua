solution "AndyVincentGMod"

	language "C++"
	location ( os.get() .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "lib/" .. os.get() .. "/" )
	includedirs {	"../gmodlua/include/",
					"../mysql/include/",
					"MySQLOO/includes/",
					"ThreadOO/includes/",
					"LuaOO/includes/"		 }

	if os.get() == "linux" then

		buildoptions{ "-fPIC" }
		linkoptions{ "-fPIC" }

	end
	
	configurations
	{ 
		"Release"
	}
	
	configuration "Release"
		defines { "NDEBUG" }
		flags{ "Optimize", "FloatFast" }

		if os.get() == "windows" then

			defines{ "WIN32" }

		elseif os.get() == "linux" then

			defines{ "LINUX" }

		end
	
	project "ThreadOO"
		defines { "GMMODULE" }
		files { "ThreadOO/source/**.*", "ThreadOO/includes/**.*" }
		kind "StaticLib"

	project "LuaOO"
		defines{ "GMMODULE" }
		files { "LuaOO/source/**.*", "LuaOO/includes/**.*" }
		kind "StaticLib"

	project "MySQLOO"
		defines{ "GMMODULE" }
		files{ "MySQLOO/source/**.*", "MySQLOO/includes/**.*" }
		kind "SharedLib"
		libdirs { "../mysql/lib/debug/", "../mysql/lib/opt/", "lib/" .. os.get() .. "/" }
		local libmysql = (os.get() == "linux" and "mysql" or "libmysql")
		links { libmysql, "ThreadOO", "LuaOO" }
		