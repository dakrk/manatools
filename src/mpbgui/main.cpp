#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <manatools/version.hpp>
#include <guicommon/AudioSystem.hpp>

#include "MainWindow.hpp"
#include "mpbgui.hpp"

int main(int argc, char** argv) {
	AudioSystem audioSystem;
	QApplication app(argc, argv);
	app.setApplicationName("mpbgui");
	app.setApplicationDisplayName("mpbgui");
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
	cmdline.addPositionalArgument("file", cmdline.tr("Path to MPB or MDB file to open."));
	cmdline.process(app);

	const QStringList args = cmdline.positionalArguments();
	const QString filePath = args.size() ? args.at(0) : QString();

	MainWindow mainWindow;
	mainWindow.show();
	
	if (filePath.size())
		mainWindow.loadFile(filePath);

	return app.exec();
}
