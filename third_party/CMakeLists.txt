set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# ugh... there's no way to just completely disable building the examples
set(SF2CUTE_INSTALL_EXAMPLES OFF)
add_subdirectory(sf2cute)

option(USE_HOST_PORTAUDIO "Prefer host PortAudio library over submodule" ON)

find_package(PkgConfig)

# need to take extra care so the submodule and system library use same name,
# and so build falls back to submodule if system library is missing.
if(USE_HOST_PORTAUDIO)
	pkg_check_modules(PortAudio IMPORTED_TARGET GLOBAL portaudio-2.0)
	if (PortAudio_FOUND)
		add_library(PortAudio::portaudio ALIAS PkgConfig::PortAudio)
	endif()
endif()

if(NOT PortAudio_FOUND)
	message(STATUS "Using PortAudio submodule")
	add_subdirectory(portaudio)
	add_library(PortAudio::portaudio ALIAS PortAudio)
endif()
