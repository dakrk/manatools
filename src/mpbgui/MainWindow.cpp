#include <QCloseEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QMessageBox>
#include <QMimeData>
#include <QScreen>
#include <QStyle>
#include <QTimer>
#include <stdexcept>
#include <manatools/sf2.hpp>
#include <guicommon/CSV.hpp>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/HorizontalLineItemDropStyle.hpp>
#include <guicommon/tone.hpp>
#include <guicommon/TonePlayer.hpp>
#include <guicommon/utils.hpp>

#include "MainWindow.hpp"
#include "BankPropertiesDialog.hpp"
#include "ProgramsModel.hpp"
#include "LayersModel.hpp"
#include "SplitsModel.hpp"
#include "LayerEditor.hpp"
#include "SplitEditor.hpp"
#include "VelCurveEditor.hpp"
#include "mpbgui.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings(),
	programIdx(0),
	layerIdx(0),
	splitIdx(0),
	tonePlayer(22050, this)
{
	ui.setupUi(this);
	ui.statusbar->hide();
	restoreSettings();

	// Fix program list at a sensible size across resizes
	ui.splitProgramList->setSizes({ 190, 810 });
	ui.splitProgramList->setStretchFactor(1, 1);

	ui.splitProgramPanes->setStretchFactor(0, 1);
	ui.splitProgramPanes->setStretchFactor(1, 6);
	ui.splitProgramPanes->setCollapsible(0, false);

	ui.splitSplitSelect->setStretchFactor(0, 2);
	ui.splitSplitSelect->setStretchFactor(1, 4);

	// shouldn't memleak 'cos qt should handle this
	programsModel = new ProgramsModel(&bank, this);
	layersModel = new LayersModel(&bank, programIdx, this);
	splitsModel = new SplitsModel(&bank, programIdx, layerIdx, this);

	ui.tblPrograms->setModel(programsModel);
	ui.tblLayers->setModel(layersModel);
	ui.tblSplits->setModel(splitsModel);

	ui.tblPrograms->setDragEnabled(true);
	ui.tblPrograms->setDragDropMode(QAbstractItemView::InternalMove);
	ui.tblPrograms->setStyle(new HorizontalLineItemDropStyle(ui.tblPrograms->style()));

	setCommonTableProps(ui.tblPrograms);
	setCommonTableProps(ui.tblLayers);
	setCommonTableProps(ui.tblSplits);

	setCurrentFile();
	resetTableLayout();

	connectTableMutations(programsModel);
	connectTableMutations(layersModel);
	connectTableMutations(splitsModel);

	ui.actionNew->setShortcut(QKeySequence::New);
	ui.actionOpen->setShortcut(QKeySequence::Open);
	ui.actionSave->setShortcut(QKeySequence::Save);
	ui.actionSaveAs->setShortcut(QKeySequence::SaveAs);
	ui.actionClose->setShortcut(QKeySequence::Close);
	ui.actionQuit->setShortcut(QKeySequence::Quit);

	ui.menuRecentFiles->menuAction()->setVisible(false); // TODO: Implement recent files menu
	ui.actionClose->setVisible(false);                   // TODO: Multiple windows?

	ui.actionUndo->setShortcut(QKeySequence::Undo);
	ui.actionRedo->setShortcut(QKeySequence::Redo);
	ui.actionCut->setShortcut(QKeySequence::Cut);
	ui.actionCopy->setShortcut(QKeySequence::Copy);
	ui.actionPaste->setShortcut(QKeySequence::Paste);
	ui.actionDelete->setShortcut(QKeySequence::Delete);

	// TODO: Implement edit operations
	ui.actionUndo->setVisible(false);
	ui.actionRedo->setVisible(false);
	ui.actionCut->setVisible(false);
	ui.actionCopy->setVisible(false);
	ui.actionPaste->setVisible(false);
	ui.actionDelete->setVisible(false);

	ui.btnSplitPlay->setCheckable(true);

	bank.velocities.push_back(manatools::mpb::Velocity::defaultCurve());

	connect(ui.actionNew, &QAction::triggered, this, &MainWindow::newFile);
	connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::open);
	connect(ui.actionSave, &QAction::triggered, this, &MainWindow::save);
	connect(ui.actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
	connect(ui.actionSoundFont2, &QAction::triggered, this, &MainWindow::exportSF2);
	connect(ui.actionQuit, &QAction::triggered, this, &QApplication::quit);

	connect(ui.actionProperties, &QAction::triggered, this, &MainWindow::editBankProperties);
	connect(ui.actionVelocities, &QAction::triggered, this, &MainWindow::editVelocities);

	connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::about);
	connect(ui.actionAboutQt, &QAction::triggered, this, [this]() { QMessageBox::aboutQt(this); });

	#define CONNECT_BTN_ADD(button, table) \
		connect(button, &QPushButton::clicked, this, [this]() { \
			insertItemRowHere(table); \
		});

	#define CONNECT_BTN_DEL(button, table) \
		connect(button, &QPushButton::clicked, this, [this]() { \
			removeSelectedViewItems(table); \
		});

	/**
	 * Have to use a QTimer and persistent index here, as currentChanged is emitted during the
	 * operation, meaning the index provided immediately after removal is unsuitable for use
	 */
	#define CONNECT_ROW_CHANGED(view, callback) \
		connect(view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, \
		        [&](const QModelIndex &current, const QModelIndex &previous) { \
		        	Q_UNUSED(previous); \
		        	QPersistentModelIndex idx(current); \
		        	QTimer::singleShot(0, this, [this, idx]() { \
		        		callback(idx.isValid() ? idx.row() : 0); \
		        	}); \
		        });

	CONNECT_BTN_ADD(ui.btnProgramAdd, ui.tblPrograms);
	CONNECT_BTN_DEL(ui.btnProgramDel, ui.tblPrograms);
	CONNECT_BTN_ADD(ui.btnSplitAdd, ui.tblSplits);
	CONNECT_BTN_DEL(ui.btnSplitDel, ui.tblSplits);

	/**
	 * The layers table has a fixed number of slots and as such using insertRow and removeRow
	 * doesn't properly fit our needs
	 */
	connect(ui.btnLayerAdd, &QPushButton::clicked, this, [this]() {
		QModelIndex cur = ui.tblLayers->currentIndex();
		if (cur.isValid())
			layersModel->addLayer(cur.row());
	});

	connect(ui.btnLayerDel, &QPushButton::clicked, this, [this]() {
		QModelIndex cur = ui.tblLayers->currentIndex();
		if (cur.isValid())
			layersModel->removeLayer(cur.row());
	});

	CONNECT_ROW_CHANGED(ui.tblPrograms, setProgram);
	CONNECT_ROW_CHANGED(ui.tblLayers, setLayer);
	CONNECT_ROW_CHANGED(ui.tblSplits, setSplit);

	#undef CONNECT_BTN_ADD
	#undef CONNECT_BTN_DEL
	#undef CONNECT_ROW_CHANGED

	// Moving a row doesn't fire currentRowChanged, despite the current row having actually changed
	connect(programsModel, &QAbstractItemModel::rowsMoved, this, [this]() {
		QModelIndex cur = ui.tblPrograms->currentIndex();
		if (cur.isValid())
			setProgram(cur.row());
	});

	connect(splitsModel, &QAbstractTableModel::dataChanged, this,
	        [this](const QModelIndex& tl, const QModelIndex& br, const QList<int>& roles)
	{
		// tad bit of a hack so tone gets reloaded after split changes
		if (std::cmp_less_equal(tl.row(), splitIdx) &&
		    (!br.isValid() || std::cmp_greater_equal(br.row(), splitIdx)) &&
		    (roles.contains(Qt::DisplayRole) || roles.isEmpty())) 
		{
			setSplit(splitIdx);
		}
	});

	connect(ui.tblLayers, &QTableView::activated, this, [this](const QModelIndex& index) {
		if (index.isValid() && index.column() == 0)
			editLayer();
	});

	connect(ui.tblSplits, &QTableView::activated, this, [this](const QModelIndex& index) {
		if (index.isValid() && index.column() == 0)
			editSplit();
	});

	connect(ui.btnLayerEdit, &QPushButton::clicked, this, &MainWindow::editLayer);
	connect(ui.btnSplitEdit, &QPushButton::clicked, this, &MainWindow::editSplit);
	connect(ui.btnToneImport, &QPushButton::clicked, this, &MainWindow::importTone);
	connect(ui.btnToneExport, &QPushButton::clicked, this, &MainWindow::exportTone);

	connect(ui.btnSplitPlay, &QPushButton::clicked, this, [this](bool checked) {
		checked ? tonePlayer.play() : tonePlayer.stop();
	});

	connect(ui.btnSplitPlay, &QPushButton::toggled, this, [this](bool checked) {
		ui.btnSplitPlay->setIcon(getPlaybackIcon(checked));
	});

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnSplitPlay->setChecked(tonePlayer.isPlaying());
	});
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank = manatools::mpb::load(path.toStdWString(), ui.actionGuessToneSize->isChecked());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Open MIDI program/drum bank"), tr("Failed to load bank file: %1").arg(err.what()));
		return false;
	}

	cursor.restore();

	if (!loadMapFile(path % ".csv")) {
		loadMapFile(getOutPath(path, true) % "/manatools_mpb_map.csv");
	}

	saveMappings.reset();
	setCurrentFile(path);
	reloadTables();

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
		QMessageBox::warning(this, tr("Save MIDI program/drum bank"), tr("Failed to save bank file: %1").arg(err.what()));
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

	// skip the first row, typically column names
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
		manatools::mpb::Program* program;

		if (!ok || !(program = bank.program(index))) {
			QMessageBox::warning(
				this,
				tr("Load map file"),
				tr("Map file contains invalid/out of bounds index: %1").arg(cols[0])
			);
			continue;
		}

		program->userData = std::move(cols[1]);
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

bool MainWindow::exportSF2File(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	// TODO: sf2cute doesn't support using wstring or filesystem::path
	try {
		auto sf2 = manatools::sf2::fromMPB(bank, QFileInfo(curFile).baseName().toStdString());
		sf2.Write(path.toStdString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Export as SoundFont 2"), tr("Failed to export as SoundFont 2: %1").arg(err.what()));
		return false;
	}

	return true;
}

void MainWindow::setProgram(size_t idx) {
	programIdx = idx;
	layersModel->setPath(programIdx);
	ui.tblLayers->setCurrentIndex(layersModel->index(0, 0));
}

void MainWindow::setLayer(size_t idx) {
	layerIdx = idx;
	splitsModel->setPath(programIdx, layerIdx);
	ui.tblSplits->setCurrentIndex(splitsModel->index(0, 0));
}

void MainWindow::setSplit(size_t idx) {
	splitIdx = idx;
	const auto* split = bank.split(programIdx, layerIdx, splitIdx);
	tonePlayer.stop();
	tonePlayer.setTone(split ? split->tone : manatools::tone::Tone());
}

void MainWindow::newFile() {
	if (maybeSave()) {
		bank = {};
		bank.velocities.push_back(manatools::mpb::Velocity::defaultCurve());
		saveMappings.reset();
		setCurrentFile();
		reloadTables();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open MIDI program/drum bank"),
			getOutPath(curFile, true),
			tr("MIDI program/drum bank (*.mpb *.mdb);;All files (*.*)")
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
	const QStringList filters = {
		tr("MIDI program bank (*.mpb)"),
		tr("MIDI drum bank (*.mdb)"),
		tr("All files (*.*)")
	};

	QString selectedFilter = filters[bank.drum ? 1 : 0];

	const QString path = QFileDialog::getSaveFileName(
		this,
		tr("Save MIDI program/drum bank"),
		getOutPath(curFile),
		filters.join(";;"),
		&selectedFilter
	);

	if (path.isEmpty())
		return false;

	if (selectedFilter == filters[0])
		bank.drum = false;
	else if (selectedFilter == filters[1])
		bank.drum = true;

	return saveFile(path);
}

bool MainWindow::exportSF2() {
	QMessageBox::warning(
		this,
		tr("Export as SoundFont 2"),
		tr(
			"<p>SF2 conversion is currently incomplete and inaccurate, and may never be accurate due to "
			"fundamental incompatibilities between the formats.</p>"
			"<p>Envelope, LFO, velocity, drum mode, layer delay and bend range data will not be preserved.</p>"
		)
	);

	const QString path = QFileDialog::getSaveFileName(
		this,
		tr("Export as SoundFont 2"),
		getOutPath(curFile, false, "sf2"),
		tr("SoundFont 2 (*.sf2)")
	);

	if (path.isEmpty())
		return false;

	return exportSF2File(path);
}

bool MainWindow::importTone() {
	auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	auto metadata = tone::Metadata::fromMPB(*split);
	bool success = tone::importDialog(split->tone, &metadata, getOutPath(curFile, true), this);
	if (success) {
		metadata.toMPB(*split);
		emitRowChanged(splitsModel, splitIdx);
	}

	return success;
}

bool MainWindow::exportTone() {
	auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	auto metadata = tone::Metadata::fromMPB(*split);
	auto tonePath = QString("%1-%2-%3").arg(programIdx).arg(layerIdx).arg(splitIdx);
	return tone::exportDialog(
		split->tone,
		&metadata,
		getOutPath(curFile, true),
		QFileInfo(curFile).baseName(),
		tonePath,
		this
	);
}

void MainWindow::editBankProperties() {
	BankPropertiesDialog dlg(&bank, this);
	if (dlg.exec() == QDialog::Accepted) {
		setWindowModified(true);
	}
}

void MainWindow::editVelocities() {
	VelCurveEditor editor(bank.velocities, this);

	if (editor.exec() == QDialog::Accepted) {
		bank.velocities = std::move(editor.velocities);
		setWindowModified(true);
	}
}

void MainWindow::editLayer() {
	auto* layer = bank.layer(programIdx, layerIdx);
	if (!layer)
		return;

	LayerEditor editor(layer, this);
	editor.setPath(programIdx, layerIdx);

	if (editor.exec() == QDialog::Accepted) {
		emitRowChanged(layersModel, layerIdx);
	}
}

void MainWindow::editSplit() {
	auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return;

	SplitEditor editor(*split, bank.velocities, this);
	editor.setCurFile(curFile);
	editor.setPath(programIdx, layerIdx, splitIdx);

	if (editor.exec() == QDialog::Accepted) {
		/**
		 * unsafe, but this is a modal, so no other bank operations that may have invalidated this
		 * pointer should have happened (hopefully)
		 */
		*split = std::move(editor.split);
		bank.velocities = std::move(editor.velocities);
		// TODO: don't change windowModified if split/velocity data hasn't actually changed
		emitRowChanged(splitsModel, splitIdx);
	}
}

void MainWindow::about() {
	QMessageBox::about(
		this,
		tr("About mpbgui"),
		tr(
			"<h3>About mpbgui</h3>"
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

void MainWindow::setCommonTableProps(QTableView* table) {
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void MainWindow::emitRowChanged(QAbstractItemModel* model, int row) {
	auto topLeft = model->index(row, 0);
	auto bottomRight = model->index(row, model->columnCount());
	emit model->dataChanged(topLeft, bottomRight, { Qt::DisplayRole, Qt::EditRole });
}

void MainWindow::connectTableMutations(QAbstractTableModel* model) {
	connect(model, &QAbstractTableModel::dataChanged, this,
	        [this](const QModelIndex& tl, const QModelIndex& br, const QList<int>& roles) {
		Q_UNUSED(tl);
		Q_UNUSED(br);
		if (roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.isEmpty()) {
			setWindowModified(true);
		}
	});

	connect(model, &QAbstractTableModel::rowsInserted, this, [this]() { setWindowModified(true); });
	connect(model, &QAbstractTableModel::rowsMoved,    this, [this]() { setWindowModified(true); });
	connect(model, &QAbstractTableModel::rowsRemoved,  this, [this]() { setWindowModified(true); });
}

void MainWindow::resetTableLayout() {
	ui.tblPrograms->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui.tblPrograms->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.tblLayers->resizeColumnsToContents();
	ui.tblSplits->resizeColumnsToContents();

	if (ui.tblLayers->columnWidth(0) < 48)
		ui.tblLayers->setColumnWidth(0, 48);
	if (ui.tblSplits->columnWidth(0) < 48)
		ui.tblSplits->setColumnWidth(0, 48);
}

void MainWindow::reloadTables() {
	programsModel->setBank(&bank);
	layersModel->setBank(&bank);
	splitsModel->setBank(&bank);
	ui.tblPrograms->setCurrentIndex(programsModel->index(0, 0));
	ui.tblLayers->setCurrentIndex(layersModel->index(0, 0));
	ui.tblSplits->setCurrentIndex(splitsModel->index(0, 0));
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
		setWindowFilePath(QDir::home().filePath("untitled.mpb"));
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
