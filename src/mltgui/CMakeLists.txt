set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(mltgui
	main.cpp
	MainWindow.cpp
	MLTModel.cpp
)

target_link_libraries(mltgui PRIVATE
	manatools::manatools
	manatools::guicommon
	Qt6::Widgets
)

set_target_properties(mltgui PROPERTIES
	WIN32_EXECUTABLE ON
)

manatools_target(mltgui)
