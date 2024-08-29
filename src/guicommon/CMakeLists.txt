set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(guicommon
	AmpEnvelopeWidget.cpp
	FilterEnvelopeWidget.cpp
	CursorOverride.hpp
	UIntValidator.cpp
	utils.cpp
)

target_link_libraries(guicommon PUBLIC
	manatools::manatools
	Qt6::Widgets
)

set_target_properties(guicommon PROPERTIES
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

manatools_target(guicommon)

add_library(manatools::guicommon ALIAS guicommon)
