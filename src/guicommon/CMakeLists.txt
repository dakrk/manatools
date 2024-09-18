set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(guicommon
	AmpEnvelopeWidget.cpp
	AudioSystem.cpp
	ChannelSelectDialog.cpp
	CursorOverride.hpp
	FilterEnvelopeWidget.cpp
	FourCCDelegate.cpp
	HorizontalLineItemDropStyle.cpp
	InstDataDialog.cpp
	PianoKeyboardWidget.cpp
	tone.cpp
	TonePlayer.cpp
	UIntValidator.cpp
	utils.cpp
)

target_link_libraries(guicommon PUBLIC
	manatools::manatools
	Qt6::Widgets
	PortAudio::portaudio
	SndFile::sndfile
)

target_compile_definitions(guicommon PRIVATE
	GUICOMMON_LIBRARY
)

manatools_target(guicommon)

add_library(manatools::guicommon ALIAS guicommon)
