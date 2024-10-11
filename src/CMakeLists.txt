if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(
		-Wall
		-Wextra
	)
endif()

add_subdirectory(manatools)
add_subdirectory(guicommon)

add_subdirectory(findtool)
add_subdirectory(fobtool)
add_subdirectory(mlttool)
add_subdirectory(mpbtool)
add_subdirectory(msbtool)
add_subdirectory(osbtool)

add_subdirectory(fobgui)
add_subdirectory(mltgui)
add_subdirectory(mpbgui)
add_subdirectory(osbgui)

install(
	TARGETS
		manatools
		guicommon

		findtool
		fobtool
		mlttool
		mpbtool
		msbtool
		osbtool

		fobgui
		mltgui
		mpbgui
		osbgui

	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
