set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# maybe I should just forfeit to vcpkg at this point, this is going to be awful
# (nvm that'd mean I'd have to static link stuff, argh)
option(USE_HOST_PORTAUDIO "Prefer host PortAudio library over submodule" ON)
option(USE_HOST_SNDFILE "Prefer host sndfile library over submodule" ON)

# ugh... there's no way to just completely disable building the examples
set(SF2CUTE_INSTALL_EXAMPLES OFF)
add_subdirectory(sf2cute)

######## PortAudio ########
if(USE_HOST_PORTAUDIO)
	find_pkgconfig_module(PortAudio PortAudio::portaudio portaudio-2.0)
endif()

if(NOT PortAudio_FOUND)
	message(STATUS "Using PortAudio submodule")
	add_subdirectory(portaudio)
	add_library(PortAudio::portaudio ALIAS PortAudio)
endif()

######## sndfile ########
if(USE_HOST_SNDFILE)
	find_pkgconfig_module(SndFile SndFile::sndfile sndfile)
endif()

if(NOT SndFile_FOUND)
	message(STATUS "Using libsndfile submodule")
	set(BUILD_PROGRAMS OFF)
	set(BUILD_EXAMPLES OFF)
	set(BUILD_TESTING OFF)
	add_subdirectory(libsndfile)
endif()
