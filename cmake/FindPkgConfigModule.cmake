find_package(PkgConfig)

function(find_pkgconfig_module prefix alias module)
	pkg_check_modules(${prefix} IMPORTED_TARGET GLOBAL ${module})
	if (${prefix}_FOUND)
		add_library(${alias} ALIAS PkgConfig::${prefix})
	endif()
endfunction()
