set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(mpbgui
	main.cpp
	MainWindow.ui
	MainWindow.cpp
	BankPropertiesDialog.ui
	BankPropertiesDialog.cpp
	ProgramsModel.cpp
	LayersModel.cpp
	SplitsModel.cpp
	LayerEditor.ui
	LayerEditor.cpp
	SplitEditor.ui
	SplitEditor.cpp
	SplitMiscEditor.ui
	SplitMiscEditor.cpp
	VelCurveEditor.ui
	VelCurveEditor.cpp
	VelCurvesModel.cpp
	VelCurveWidget.cpp
)

target_link_libraries(mpbgui PRIVATE
	manatools::manatools
	manatools::guicommon
	Qt6::Widgets
)

set_target_properties(mpbgui PROPERTIES
	WIN32_EXECUTABLE ON
	MACOSX_BUNDLE ON
)

manatools_target(mpbgui)
