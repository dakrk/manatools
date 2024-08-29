#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <guicommon/AudioSystem.hpp>

//#include "MainWindow.hpp"
#include "osbgui.hpp"

int main(int argc, char** argv) {
	AudioSystem audioSystem;
	QApplication app(argc, argv);
	app.setApplicationName("osbgui");
	app.setApplicationDisplayName("osbgui");
	app.setApplicationVersion("0.1.0");
	app.setOrganizationName("DarkOK");
	app.setOrganizationDomain("darkok.xyz");

#ifdef _WIN32
	// Allows for dark theme
	app.setStyle(QStyleFactory::create("Fusion"));
#endif

	QCommandLineParser cmdline;
	cmdline.setApplicationDescription(cmdline.tr(APP_DESCRIPTION));
	cmdline.addHelpOption();
	cmdline.addVersionOption();
	cmdline.addPositionalArgument("file", cmdline.tr("Path to OSB file to open."));
	cmdline.process(app);

	const QStringList args = cmdline.positionalArguments();
	const QString filePath = args.size() ? args.at(0) : QString();

/*	MainWindow mainWindow;
	mainWindow.show();
	
	if (filePath.size())
		mainWindow.loadFile(filePath);
*/

	return app.exec();
}
