cmake_minimum_required(VERSION 3.22)

project(manatools
	VERSION 0.3.0
	LANGUAGES C CXX
)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Windows doesn't have a concept like RPATH so executables cannot be ran from
# the build directory for testing unless everything's in the same folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# For the dumb method of building with Qt on Visual Studio
list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(CompilerFlags)
include(ProjectFuncs)
include(FindPkgConfigModule)

add_subdirectory(third_party)
add_subdirectory(src)

include(CPack)
set(CPACK_STRIP_FILES ON)
