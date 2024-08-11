if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(
		-Wall
		-Wextra
	)
endif()

add_subdirectory(manatools)

add_subdirectory(mpbtool)
add_subdirectory(mlttool)
add_subdirectory(osbtool)

add_subdirectory(mpbgui)
add_subdirectory(mltgui)

install(TARGETS
	manatools

	mpbtool
	mlttool
	osbtool

	mpbgui
	mltgui
)
