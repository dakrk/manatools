#include <QApplication>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/utils.hpp>

#include "MainWindow.hpp"
#include "osbgui.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings()
{
	restoreSettings();

	setCurrentFile();
	resetTableLayout();
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		osb = manatools::osb::load(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to load bank file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	reloadTable();
	return true;
}

bool MainWindow::saveFile(const QString& path) {
/*	CursorOverride cursor(Qt::WaitCursor);

	try {
		osb.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to save bank file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path); */
	Q_UNUSED(path)
	return false;
}

void MainWindow::newFile() {
	if (maybeSave()) {
		osb = {};
		setCurrentFile();
		reloadTable();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open One Shot Bank"),
			getOutPath(curFile, true),
			tr("One Shot Bank (*.osb);;All files (*.*)")
		);

		if (!path.isEmpty()) {
			return loadFile(path);
		}
	}

	return false;
}

bool MainWindow::save() {
	if (curFile.isEmpty())
		return saveAs();

	return saveFile(curFile);
}

bool MainWindow::saveAs() {
	const QString path = QFileDialog::getSaveFileName(
		this,
		tr("Save One Shot Bank"),
		getOutPath(curFile),
		tr("One Shot Bank (*.osb);;All files (*.*)")
	);

	if (path.isEmpty())
		return false;

	return saveFile(path);
}

void MainWindow::about() {
	QMessageBox::about(
		this,
		tr("About osbgui"),
		tr(
			"<h3>About osbgui</h3>"
			"<small>Version %1</small>" // TODO: Show commit hash here
			"<p>%2</p>"
			"<p>This is part of manatools. For more information, visit <a href=\"%3\">%3</a>.</p>"
		)
		.arg(QApplication::applicationVersion())
		.arg(tr(APP_DESCRIPTION))
		.arg("https://github.com/dakrk/manatools")
	);
}

void MainWindow::closeEvent(QCloseEvent* event) {
	saveSettings();
	if (maybeSave()) {
		event->accept();
	} else {
		event->ignore();
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
	maybeDropEvent(event);
}

void MainWindow::dropEvent(QDropEvent* event) {
	const QString path = maybeDropEvent(event);
	if (!path.isEmpty() && maybeSave()) {
		loadFile(path);
	}
}

void MainWindow::resetTableLayout() {

}

void MainWindow::reloadTable() {
	
}

QString MainWindow::maybeDropEvent(QDropEvent* event) {
	const QMimeData* mimeData = event->mimeData();
	if (!mimeData->hasUrls())
		return {};

	const QList<QUrl> urls = mimeData->urls();
	if (urls.size() != 1)
		return {};

	const QString path = urls[0].toLocalFile();
	if (path.isEmpty() || !QFileInfo(path).isFile())
		return {};

	event->acceptProposedAction();
	return path;
}

bool MainWindow::maybeSave() {
	if (!isWindowModified())
		return true;

	const QMessageBox::StandardButton btn = QMessageBox::warning(
		this,
		tr("Save changes to file?"),
		tr("File has been modified. Would you like to save changes?"),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
		QMessageBox::Save
	);

	switch (btn) {
		case QMessageBox::Save:   return save();
		case QMessageBox::Cancel: return false;
		default:                  break;
	}

	return true;
}

void MainWindow::setCurrentFile(const QString& path) {
	curFile = path;

	if (!curFile.isEmpty()) {
		setWindowFilePath(curFile);
	} else {
		setWindowFilePath(QDir::home().filePath("untitled.osb"));
	}

	setWindowModified(false);
}

void MainWindow::restoreSettings() {
	if (!restoreGeometry(settings.value("MainWindow/Geometry").toByteArray())) {
		resize(600, 600);
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void MainWindow::saveSettings() {
	settings.setValue("MainWindow/Geometry", saveGeometry());
}
