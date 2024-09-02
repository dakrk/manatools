#include <QApplication>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>
#include <manatools/io.hpp>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/utils.hpp>
#include <functional>
#include <utility>

#include "MainWindow.hpp"
#include "mltgui.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings()
{
	restoreSettings();

	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(QIcon::fromTheme("document-new"), tr("&New"), QKeySequence::New, this, &MainWindow::newFile);
	fileMenu->addAction(QIcon::fromTheme("document-open"), tr("&Open"), QKeySequence::Open, this, &MainWindow::open);
	fileMenu->addSeparator();
	fileMenu->addAction(QIcon::fromTheme("document-save"), tr("&Save"), QKeySequence::Save, this, &MainWindow::save);
	fileMenu->addAction(QIcon::fromTheme("document-save-as"), tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveAs);
	fileMenu->addSeparator();
	fileMenu->addAction(QIcon::fromTheme("application-exit"), tr("&Quit"), QKeySequence::Quit, this, &QApplication::quit);

	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(QIcon::fromTheme("help-about"), tr("&About"), this, &MainWindow::about);
	helpMenu->addAction(tr("About Qt"), this, [this]() { QMessageBox::aboutQt(this); });

	QMenu* unitTypeMenu = new QMenu(this);
	unitTypeMenu->addAction(tr("MIDI Sequence Bank [MSB]"), std::bind(&MainWindow::addUnit, this, "SMSB"));
	unitTypeMenu->addAction(tr("MIDI Program Bank [MPB]"), std::bind(&MainWindow::addUnit, this, "SMPB"));
	unitTypeMenu->addAction(tr("MIDI Drum Bank [MDB]"), std::bind(&MainWindow::addUnit, this, "SMDB"));
	unitTypeMenu->addAction(tr("One Shot Bank [OSB]"), std::bind(&MainWindow::addUnit, this, "SOSB"));
	unitTypeMenu->addAction(tr("FX Program Bank [FPB]"), std::bind(&MainWindow::addUnit, this, "SFPB"));
	unitTypeMenu->addAction(tr("FX Output Bank [FOB]"), std::bind(&MainWindow::addUnit, this, "SFOB"));
	unitTypeMenu->addAction(tr("FX Program Work [FPW]"), std::bind(&MainWindow::addUnit, this, "SFPW"));
	unitTypeMenu->addAction(tr("PCM Stream Ring Buffer [PSR]"), std::bind(&MainWindow::addUnit, this, "SPSR"));

	table = new QTableView(this);
	model = new MLTModel(&mlt, this);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setModel(model);

	setCurrentFile();
	resetTableLayout();

	QToolButton* toolbtnAddUnit = new QToolButton();
	toolbtnAddUnit->setIcon(QIcon::fromTheme("list-add"));
	toolbtnAddUnit->setPopupMode(QToolButton::InstantPopup);
	toolbtnAddUnit->setMenu(unitTypeMenu);

	QPushButton* btnDelUnit = new QPushButton(QIcon::fromTheme("list-remove"), "");
	QPushButton* btnImportUnitData = new QPushButton(QIcon::fromTheme("document-open"), "");
	QPushButton* btnExportUnitData = new QPushButton(QIcon::fromTheme("document-save-as"), "");

	connect(btnImportUnitData, &QPushButton::clicked, this, &MainWindow::importUnitDialog);
	connect(btnExportUnitData, &QPushButton::clicked, this, &MainWindow::exportUnitDialog);

	QHBoxLayout* btnLayout = new QHBoxLayout();
	btnLayout->addWidget(toolbtnAddUnit);
	btnLayout->addWidget(btnDelUnit);
	btnLayout->addStretch(1);
	btnLayout->addWidget(btnImportUnitData);
	btnLayout->addWidget(btnExportUnitData);

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(table);
	mainLayout->addLayout(btnLayout);

	QWidget* mainLayoutWidget = new QWidget();
	mainLayoutWidget->setLayout(mainLayout);
	setCentralWidget(mainLayoutWidget);
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		mlt = manatools::mlt::load(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to load multi-unit file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	reloadTable();
	return true;
}

bool MainWindow::saveFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		mlt.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to save multi-unit file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	return true;
}

void MainWindow::newFile() {
	if (maybeSave()) {
		mlt = {};
		setCurrentFile();
		reloadTable();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open Multi-Unit file"),
			getOutPath(curFile, true),
			tr("Multi-Unit file (*.mlt);;All files (*.*)")
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
		tr("Save Multi-Unit file"),
		getOutPath(curFile),
		tr("Multi-Unit file (*.mlt);;All files (*.*)")
	);

	if (path.isEmpty())
		return false;

	return saveFile(path);
}

void MainWindow::about() {
	QMessageBox::about(
		this,
		tr("About mltgui"),
		tr(
			"<h3>About mltgui</h3>"
			"<small>Version %1</small>" // TODO: Show commit hash here
			"<p>%2</p>"
			"<p>This is part of manatools. For more information, visit <a href=\"%3\">%3</a>.</p>"
		)
		.arg(QApplication::applicationVersion())
		.arg(tr(APP_DESCRIPTION))
		.arg("https://github.com/dakrk/manatools")
	);
}

void MainWindow::addUnit(const QString& fourCC) {
	assert(fourCC.size() == 4);
	QMessageBox::information(this, "test", fourCC);
}

bool MainWindow::importUnitDialog() {
	return false;
}

bool MainWindow::exportUnitDialog() {
	auto curIdx = table->selectionModel()->currentIndex();
	if (!curIdx.isValid() || std::cmp_greater_equal(curIdx.row(), mlt.units.size()))
		return false;

	const auto& unit = mlt.units[curIdx.row()];
	QString unitExt = unit.fourCC + 1;

	auto defPath = QDir(getOutPath(curFile, true)).filePath(
		QString("%1_u%2-b%3.%4")
			.arg(QFileInfo(curFile).baseName())
			.arg(curIdx.row())
			.arg(unit.bank)
			.arg(unitExt.toLower())
	);

	const QString path = QFileDialog::getSaveFileName(
		this,
		tr("Export unit"),
		defPath,
		tr("%1 file (*.%2);;All files (*.*)").arg(unitExt).arg(unitExt.toLower())
	);

	if (path.isEmpty())
		return false;

	return exportUnit(unit, path);
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

bool MainWindow::importUnit(manatools::mlt::Unit& unit, const QString& path) {
	Q_UNUSED(unit)
	Q_UNUSED(path)
	return false;
}

bool MainWindow::exportUnit(const manatools::mlt::Unit& unit, const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Selected unit has no data (no file offset), cannot export."));
		return false;
	}

	try {
		manatools::io::FileIO file(path.toStdWString(), "wb");
		file.writeVec(unit.data);
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to export unit: %1").arg(err.what()));
		return false;
	}
	
	return true;
}

void MainWindow::resetTableLayout() {
	table->horizontalHeader()->resizeSection(0, QHeaderView::ResizeToContents);
	table->horizontalHeader()->resizeSection(1, QHeaderView::ResizeToContents);

	if (table->columnWidth(0) < 64)
		table->setColumnWidth(0, 64);
	if (table->columnWidth(1) < 48)
		table->setColumnWidth(1, 48);
}

void MainWindow::reloadTable() {
	model->setMLT(&mlt);
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
		setWindowFilePath(QDir::home().filePath("untitled.mlt"));
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
