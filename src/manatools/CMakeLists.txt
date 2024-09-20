add_library(manatools
	fob.cpp
	io.cpp
	midi.cpp
	mlt.cpp
	mpb.cpp
	msb.cpp
	msd.cpp
	osb.cpp
	sf2.cpp
	tonedecoder.cpp
	yadpcm.cpp
)

configure_file(version.hpp.in version.hpp)

target_link_libraries(manatools PUBLIC
	sf2cute::sf2cute
)

set_target_properties(manatools PROPERTIES
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

manatools_target(manatools)

add_library(manatools::manatools ALIAS manatools)
