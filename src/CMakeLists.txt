if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(
		-Wall
		-Wextra
	)
endif()

add_subdirectory(manatools)
add_subdirectory(guicommon)

add_subdirectory(mlttool)
add_subdirectory(mpbtool)
add_subdirectory(msbtool)
add_subdirectory(osbtool)

add_subdirectory(mltgui)
add_subdirectory(mpbgui)

install(
	TARGETS
		manatools

		mlttool
		mpbtool
		msbtool
		osbtool

		mltgui
		mpbgui

	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
