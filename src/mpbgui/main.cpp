#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <portaudio.h>

#include "MainWindow.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("mpbgui");
	app.setApplicationDisplayName("mpbgui");
	app.setApplicationVersion("0.1.0");
	app.setOrganizationName("DarkOK");
	app.setOrganizationDomain("darkok.xyz");

#ifdef _WIN32
	// Allows for dark theme
	app.setStyle(QStyleFactory::create("Fusion"));
#endif

	QCommandLineParser cmdline;
	cmdline.setApplicationDescription(cmdline.tr("GUI for editing and converting Sega MIDI Program Bank and MIDI Drum Bank files used by Dreamcast titles."));
	cmdline.addHelpOption();
	cmdline.addVersionOption();
	cmdline.addPositionalArgument("file", cmdline.tr("Path to MPB or MDB file to open."));
	cmdline.process(app);

	PaError err = Pa_Initialize();
	if (err != paNoError) {
		QMessageBox::warning(nullptr, "Failed to initialize audio", Pa_GetErrorText(err));
	}

	const QStringList args = cmdline.positionalArguments();
	const QString filePath = args.size() ? args.at(0) : QString();

	MainWindow mainWindow;
	mainWindow.show();
	
	if (filePath.size())
		mainWindow.loadFile(filePath);

	auto result = app.exec();

	Pa_Terminate();
	return result;
}
