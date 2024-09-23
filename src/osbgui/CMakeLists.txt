set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(osbgui
	main.cpp
	MainWindow.ui
	MainWindow.cpp
)

target_link_libraries(osbgui PRIVATE
	manatools::manatools
	manatools::guicommon
	Qt6::Widgets
)

set_target_properties(osbgui PROPERTIES
	WIN32_EXECUTABLE ON
)

manatools_target(osbgui)
