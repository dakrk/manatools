set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(fobgui
	main.cpp
	MainWindow.cpp
)

target_link_libraries(fobgui PRIVATE
	manatools::manatools
	manatools::guicommon
	Qt6::Widgets
)

set_target_properties(fobgui PROPERTIES
	WIN32_EXECUTABLE ON
)

manatools_target(fobgui)
