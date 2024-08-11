#include <QApplication>
#include <QStyleFactory>

#include "MainWindow.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("mltgui");
	app.setApplicationDisplayName("mltgui");
	app.setOrganizationName("DarkOK");
	app.setOrganizationDomain("darkok.xyz");

#ifdef _WIN32
	// Allows for dark theme
	app.setStyle(QStyleFactory::create("Fusion"));
#endif

	MainWindow mainWindow;
	mainWindow.show();

	return app.exec();
}