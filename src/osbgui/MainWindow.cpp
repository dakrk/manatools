#include <QApplication>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/HorizontalLineItemDropStyle.hpp>
#include <guicommon/utils.hpp>

#include "MainWindow.hpp"
#include "ProgramEditor.hpp"
#include "osbgui.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings(),
	tonePlayer(44100, this)
{
	ui.setupUi(this);
	ui.statusbar->hide();
	restoreSettings();

	model = new OSBModel(&bank);

	ui.tblPrograms->setModel(model);
	ui.tblPrograms->setDragEnabled(true);
	ui.tblPrograms->setDragDropMode(QAbstractItemView::InternalMove);
	ui.tblPrograms->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.tblPrograms->setSelectionMode(QAbstractItemView::ExtendedSelection);
	ui.tblPrograms->setStyle(new HorizontalLineItemDropStyle(ui.tblPrograms->style()));
	ui.tblPrograms->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui.tblPrograms->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	setCurrentFile();
	resetTableLayout();

	ui.actionNew->setShortcut(QKeySequence::New);
	ui.actionOpen->setShortcut(QKeySequence::Open);
	ui.actionSave->setShortcut(QKeySequence::Save);
	ui.actionSaveAs->setShortcut(QKeySequence::SaveAs);
	ui.actionQuit->setShortcut(QKeySequence::Quit);

	ui.actionDelete->setShortcut(QKeySequence::Delete);
	ui.actionSelectAll->setShortcut(QKeySequence::SelectAll);

	ui.menuRecentFiles->menuAction()->setVisible(false);

	ui.btnPlayProgram->setCheckable(true);

	connect(ui.actionNew, &QAction::triggered, this, &MainWindow::newFile);
	connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::open);
	connect(ui.actionSave, &QAction::triggered, this, &MainWindow::save);
	connect(ui.actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
	connect(ui.actionQuit, &QAction::triggered, this, &QApplication::quit);

	connect(ui.actionDelete, &QAction::triggered, this, &MainWindow::delProgram);
	connect(ui.actionSelectAll, &QAction::triggered, this, &MainWindow::selectAll);

	connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::about);
	connect(ui.actionAboutQt, &QAction::triggered, this, [this]() { QMessageBox::aboutQt(this); });

	connect(ui.tblPrograms->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
	        [this](const QModelIndex &current, const QModelIndex &previous) {
		Q_UNUSED(previous);
		if (current.isValid()) {
			tonePlayer.stop();
			tonePlayer.setTone(bank.programs[current.row()].tone);
		}
	});

	connect(model, &QAbstractTableModel::dataChanged, this,
	        [this](const QModelIndex& tl, const QModelIndex& br, const QList<int>& roles) {
		Q_UNUSED(tl);
		Q_UNUSED(br);
		if (roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.isEmpty()) {
			setWindowModified(true);
		}
	});

	connect(ui.tblPrograms, &QTableView::activated, this, [this](const QModelIndex& index) {
		if (index.isValid() && index.column() == 1) {
			editProgram();
		}
	});

	connect(ui.btnAddProgram, &QPushButton::clicked, this, [this]() {
		insertItemRowHere(ui.tblPrograms);
	});

	connect(ui.btnDelProgram, &QPushButton::clicked, this, &MainWindow::delProgram);

	connect(ui.btnPlayProgram, &QPushButton::clicked, this, [this](bool checked) {
		checked ? tonePlayer.play() : tonePlayer.stop();
	});

	connect(ui.btnPlayProgram, &QPushButton::toggled, this, [this](bool checked) {
		ui.btnPlayProgram->setIcon(getPlaybackIcon(checked));
	});

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnPlayProgram->setChecked(tonePlayer.isPlaying());
	});

	connect(ui.btnEditProgram, &QPushButton::clicked, this, &MainWindow::editProgram);
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank = manatools::osb::load(path.toStdWString(), ui.actionGuessToneSize->isChecked());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Open One Shot bank"), tr("Failed to load bank file: %1").arg(err.what()));
		return false;
	}

	cursor.restore();

	setCurrentFile(path);
	reloadTable();

	if (bank.version != 2) {
		QMessageBox::warning(
			this,
			tr("Unsupported file version"),
			tr("Loaded bank is of an untested version. There may be inaccuracies. (Expected 2, got %1)")
				.arg(bank.version)
		);
	}

	return true;
}

bool MainWindow::saveFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Save One Shot bank"), tr("Failed to save bank file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	return true;
}

void MainWindow::newFile() {
	if (maybeSave()) {
		bank = {};
		setCurrentFile();
		reloadTable();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open One Shot bank"),
			getOutPath(curFile, true),
			tr("One Shot bank (*.osb);;All files (*.*)")
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
		tr("Save One Shot bank"),
		getOutPath(curFile),
		tr("One Shot bank (*.osb);;All files (*.*)")
	);

	if (path.isEmpty())
		return false;

	return saveFile(path);
}

void MainWindow::delProgram() {
	removeSelectedViewItems(ui.tblPrograms);
}

void MainWindow::selectAll() {
	ui.tblPrograms->selectAll();
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

void MainWindow::editProgram() {
	auto curIdx = ui.tblPrograms->selectionModel()->currentIndex();
	if (!curIdx.isValid())
		return;

	auto& program = bank.programs[curIdx.row()];

	ProgramEditor editor(program, this);
	editor.setCurFile(curFile);
	editor.setPath(curIdx.row());

	if (editor.exec() == QDialog::Accepted) {
		program = std::move(editor.program);
		emitRowChanged(model, curIdx.row());
	}
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

void MainWindow::emitRowChanged(QAbstractItemModel* model, int row) {
	auto topLeft = model->index(row, 0);
	auto bottomRight = model->index(row, model->columnCount());
	emit model->dataChanged(topLeft, bottomRight, { Qt::DisplayRole, Qt::EditRole });
}

void MainWindow::resetTableLayout() {
	ui.tblPrograms->horizontalHeader()->resizeSection(0, QHeaderView::ResizeToContents);
	ui.tblPrograms->horizontalHeader()->resizeSection(1, QHeaderView::ResizeToContents);

	if (ui.tblPrograms->columnWidth(0) < 64)
		ui.tblPrograms->setColumnWidth(0, 64);
	if (ui.tblPrograms->columnWidth(1) < 64)
		ui.tblPrograms->setColumnWidth(1, 64);
}

void MainWindow::reloadTable() {
	model->setBank(&bank);
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

	const auto btn = QMessageBox::warning(
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
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}

	ui.actionGuessToneSize->setChecked(settings.value("MainWindow/GuessToneSize", true).toBool());
}

void MainWindow::saveSettings() {
	settings.setValue("MainWindow/Geometry", saveGeometry());
	settings.setValue("MainWindow/GuessToneSize", ui.actionGuessToneSize->isChecked());
}
