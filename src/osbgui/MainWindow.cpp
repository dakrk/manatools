#include <QApplication>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <guicommon/CSV.hpp>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/HorizontalLineItemDropStyle.hpp>
#include <guicommon/tone.hpp>
#include <guicommon/utils.hpp>

#include "MainWindow.hpp"
#include "ProgramEditor.hpp"
#include "osbgui.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings(),
	tonePlayer(this)
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
	connect(ui.actionBankVersion, &QAction::triggered, this, &MainWindow::versionDialog);

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
		if (roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.isEmpty()) {
			setWindowModified(true);

			// get the thing to reload tone when current program has been modified
			auto curIdx = ui.tblPrograms->currentIndex();
			if (curIdx.isValid() && std::cmp_less_equal(tl.row(), curIdx.row()) &&
			    (!br.isValid() || std::cmp_greater_equal(br.row(), curIdx.row())))
			{
				tonePlayer.setTone(bank.programs[curIdx.row()].tone);
			}
		}
	});

	connect(model, &QAbstractTableModel::rowsInserted, this, [this]() { setWindowModified(true); });
	connect(model, &QAbstractTableModel::rowsMoved,    this, [this]() { setWindowModified(true); });
	connect(model, &QAbstractTableModel::rowsRemoved,  this, [this]() { setWindowModified(true); });

	connect(ui.tblPrograms, &QTableView::activated, this, [this](const QModelIndex& index) {
		if (index.isValid() && index.column() == 1) {
			editProgram();
		}
	});

	connect(ui.btnPlayProgram, &QPushButton::clicked, this, [this](bool checked) {
		checked ? tonePlayer.play() : tonePlayer.stop();
	});

	connect(ui.btnPlayProgram, &QPushButton::toggled, this, [this](bool checked) {
		ui.btnPlayProgram->setIcon(getPlaybackIcon(checked));
	});

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnPlayProgram->setChecked(tonePlayer.isPlaying());
	});

	connect(ui.btnAddProgram, &QPushButton::clicked, this, [this]() {
		insertItemRowHere(ui.tblPrograms);
	});

	connect(ui.btnDelProgram, &QPushButton::clicked, this, &MainWindow::delProgram);
	connect(ui.btnToneImport, &QPushButton::clicked, this, &MainWindow::importTone);
	connect(ui.btnToneExport, &QPushButton::clicked, this, &MainWindow::exportTone);
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

	if (!loadMapFile(path % ".csv")) {
		loadMapFile(getOutPath(path, true) % "/manatools_osb_map.csv");
	}

	saveMappings.reset();
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
	if (bank.version != 2) {
		QMessageBox::warning(
			this,
			tr("Unsupported file version"),
			tr("Bank is being saved as an untested version. There may be inaccuracies. (Expected 2, got %1)")
				.arg(bank.version)
		);
	}

	bool doSaveMappings = saveMappingsDialog();
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Save One Shot bank"), tr("Failed to save bank file: %1").arg(err.what()));
		return false;
	}

	if (doSaveMappings && !saveMapFile(path % ".csv")) {
		QMessageBox::warning(this, tr("Save map file"), tr("Failed to save map file."));
	}

	setCurrentFile(path);
	return true;
}

bool MainWindow::loadMapFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	CSV csv;
	QTextStream stream(&file);
	csv.read(stream);

	for (qsizetype i = 1; i < csv.rows.size(); i++) {
		const auto& cols = csv.rows[i];
		if (cols.size() < 2) {
			QMessageBox::warning(
				this,
				tr("Load map file"),
				tr("Map file is invalid, has less than 2 columns.").arg(cols[0])
			);
			return false;
		}

		bool ok;
		ulong index = cols[0].toULong(&ok);

		if (!ok || index + 1 > bank.programs.size()) {
			QMessageBox::warning(
				this,
				tr("Load map file"),
				tr("Map file contains invalid/out of bounds index: %1").arg(cols[0])
			);
			continue;
		}

		bank.programs[index].userData = std::move(cols[1]);
	}

	return true;
}

bool MainWindow::saveMapFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	CSV csv;
	QTextStream stream(&file);

	csv.rows.emplaceBack(QStringList { "path", "name" });

	for (size_t i = 0; i < bank.programs.size(); i++) {
		const QString* name = std::any_cast<QString>(&bank.programs[i].userData);
		if (name && !name->isEmpty()) {
			csv.rows.emplaceBack(QStringList { QString::number(i), *name });	
		}
	}

	csv.write(stream);
	return true;
}

void MainWindow::newFile() {
	if (maybeSave()) {
		bank = {};
		saveMappings.reset();
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

void MainWindow::versionDialog() {
	bool ok;
	auto verStr = QInputDialog::getText(
		this,
		tr("Set bank version"),
		tr("Version:"),
		QLineEdit::Normal,
		formatHex(bank.version),
		&ok
	);

	if (ok && !verStr.isEmpty()) {
		u32 newVer = verStr.toUInt(&ok, 0);
		if (ok && newVer != bank.version) {
			bank.version = newVer;
			setWindowModified(true);
		}
	}
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

bool MainWindow::importTone() {
	auto curIdx = ui.tblPrograms->currentIndex();
	if (!curIdx.isValid())
		return false;

	auto& program = bank.programs[curIdx.row()];
	auto metadata = tone::Metadata::fromOSB(program);

	bool success = tone::importDialog(program.tone, &metadata, getOutPath(curFile, true), this);
	if (success) {
		metadata.toOSB(program);
		emitRowChanged(model, curIdx.row());
	}

	return success;
}

bool MainWindow::exportTone() {
	const auto selRows = ui.tblPrograms->selectionModel()->selectedRows();
	bool failed = false;

	for (const auto& idx : selRows) {
		const auto& program = bank.programs[idx.row()];
		auto metadata = tone::Metadata::fromOSB(program);

		failed |= !tone::exportDialog(
			program.tone,
			&metadata,
			getOutPath(curFile, true),
			QFileInfo(curFile).baseName(),
			QString::number(idx.row()),
			this
		);
	}

	return !failed;
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

void MainWindow::reloadTable() {
	model->setBank(&bank);
}

bool MainWindow::programNameSet() const {
	for (const auto& program : bank.programs) {
		const QString* name = std::any_cast<QString>(&program.userData);
		if (name && !name->isEmpty()) {
			return true;
		}
	}
	return false;
}

bool MainWindow::saveMappingsDialog() {
	if (saveMappings.has_value()) {
		return *saveMappings;
	} else if (programNameSet()) {
		QMessageBox mb(
			QMessageBox::Question,
			tr("Save map file"),
			tr("Program names were set in this bank. Would you like to save the mapping file with the bank?"),
			QMessageBox::Yes | QMessageBox::No,
			this
		);

		QCheckBox cb(tr("Don't ask me again this session"), &mb);
		mb.setCheckBox(&cb);

		bool ret = mb.exec() == QMessageBox::Yes;
		if (cb.isChecked()) {
			saveMappings = ret;
		}

		return ret;
	}

	return false;
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
