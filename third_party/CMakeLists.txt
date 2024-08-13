set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# maybe I should just forfeit to vcpkg at this point, this is going to be awful
# (nvm that'd mean I'd have to static link stuff, argh)
option(USE_HOST_PORTAUDIO "Prefer host PortAudio library over submodule" ON)
option(USE_HOST_OGG "Prefer host Ogg library over submodule" ON)
option(USE_HOST_VORBIS "Prefer host Vorbis library over submodule" ON)
option(USE_HOST_FLAC "Prefer host FLAC library over submodule" ON)
option(USE_HOST_OPUS "Prefer host Opus library over submodule" ON)
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

######## Ogg ########
if(USE_HOST_OGG)
	find_pkgconfig_module(OGG Ogg::ogg ogg)
endif()

if(NOT OGG_FOUND)
	set(INSTALL_DOCS OFF)
	add_subdirectory(ogg)
endif()

######## Vorbis ########
if(USE_HOST_VORBIS)
	find_pkgconfig_module(Vorbis Vorbis::vorbis vorbis)
	find_pkgconfig_module(Vorbis_Enc Vorbis::vorbisenc vorbisenc)
endif()

if(NOT Vorbis_FOUND OR NOT Vorbis_Enc_FOUND)
	set(BUILD_TESTING OFF)
	add_subdirectory(vorbis)
endif()

######## FLAC ########
if(USE_HOST_FLAC)
	find_pkgconfig_module(FLAC FLAC::FLAC flac)
endif()

if(NOT FLAC_FOUND)
	set(BUILD_CXXLIBS OFF)
	set(BUILD_PROGRAMS OFF)
	set(BUILD_EXAMPLES OFF)
	set(BUILD_TESTING OFF)
	set(BUILD_DOCS OFF)
	set(INSTALL_MANPAGES OFF)
	# great, and you can't disable this "benchmark_residual" binary
	add_subdirectory(flac)
endif()

######## Opus ########
if(USE_HOST_OPUS)
	find_pkgconfig_module(OPUS Opus::opus opus)
endif()

if(NOT OPUS_FOUND)
	# TODO: See if you can make the Wcast-align warning shut up
	add_subdirectory(opus)
endif()

######## LAME ########
# still uses SVN and I don't even need MP3 encoding anyway so I CBA

######## mpg123 ########
# haha, this does too... at least there's an endorsed git mirror. TODO

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
