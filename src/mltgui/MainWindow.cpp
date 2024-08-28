#include <QApplication>
#include <QScreen>

#include "MainWindow.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings()
{
	restoreSettings();

	table = new QTableView(this);
	model = new MLTModel(&mlt, this);
	table->setModel(model);

	setCentralWidget(table);
}

void MainWindow::closeEvent(QCloseEvent* event) {
	Q_UNUSED(event)
	saveSettings();
}

void MainWindow::restoreSettings() {
	if (!restoreGeometry(settings.value("MainWindow/Geometry").toByteArray())) {
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void MainWindow::saveSettings() {
	settings.setValue("MainWindow/Geometry", saveGeometry());
}

