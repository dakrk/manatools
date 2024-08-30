#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <manatools/version.hpp>

#include "MainWindow.hpp"
#include "mltgui.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("mltgui");
	app.setApplicationDisplayName("mltgui");
	app.setApplicationVersion(manatools::versionString);
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
	cmdline.addPositionalArgument("file", cmdline.tr("Path to MLT file to open."));
	cmdline.process(app);

	const QStringList args = cmdline.positionalArguments();
	const QString filePath = args.size() ? args.at(0) : QString();

	MainWindow mainWindow;
	mainWindow.show();
	
	if (filePath.size())
		mainWindow.loadFile(filePath);

	return app.exec();
}