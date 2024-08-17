#include <QScreen>
#include <QStyle>
#include <QKeySequence>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMimeData>
#include <stdexcept>
#include <manatools/sf2.hpp>
#include <manatools/wav.hpp>

#include "MainWindow.hpp"
#include "BankPropertiesDialog.hpp"
#include "ProgramsModel.hpp"
#include "LayersModel.hpp"
#include "SplitsModel.hpp"
#include "LayerEditor.hpp"
#include "SplitEditor.hpp"
#include "CursorOverride.hpp"
#include "TonePlayer.hpp"
#include "tone.hpp"

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	settings(),
	programIdx(0),
	layerIdx(0),
	splitIdx(0),
	tonePlayer(22050, this)
{
	ui.setupUi(this);

	restoreSettings();

	ui.statusbar->hide();

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

	bank.velocities.push_back(genDefVelCurve());

	connect(ui.actionNew, &QAction::triggered, this, &MainWindow::newFile);
	connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::open);
	connect(ui.actionSave, &QAction::triggered, this, &MainWindow::save);
	connect(ui.actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
	connect(ui.actionSoundFont2, &QAction::triggered, this, &MainWindow::exportSF2);
	connect(ui.actionQuit, &QAction::triggered, this, &QApplication::quit);

	connect(ui.actionProperties, &QAction::triggered, this, &MainWindow::editBankProperties);

	connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::about);
	connect(ui.actionAboutQt, &QAction::triggered, this, [this]() { QMessageBox::aboutQt(this); });

	// TODO: if no table current selection after add, then set it to new row
	#define CONNECT_BTN_ADD(button, table, callback) \
		connect(button, &QPushButton::clicked, this, [&]() { \
			QModelIndex cur = table->currentIndex(); \
			table->model()->insertRow(cur.isValid() ? cur.row() : 0); \
			callback(cur.row() + 1); \
		});

	// mmh
	#define CONNECT_BTN_DEL(button, table, callback) \
		connect(button, &QPushButton::clicked, this, [&]() { \
			QModelIndex cur = table->currentIndex(); \
			if (cur.isValid()) { \
				table->model()->removeRow(cur.row()); \
				if (cur.row() < table->model()->rowCount() - 1) \
					callback(cur.row()); \
				else \
					callback(cur.row() - 1); \
			} \
		});

	#define CONNECT_ROW_CHANGED(selectionModel, callback) \
		connect(selectionModel, &QItemSelectionModel::currentRowChanged, this, \
		        [&](const QModelIndex &current, const QModelIndex &previous) { \
		        	Q_UNUSED(previous); \
		        	if (current.isValid()) \
		        		callback(current.row()); \
		        });

	// explicitly setProgram because Ugh
	CONNECT_BTN_ADD(ui.btnProgramAdd, ui.tblPrograms, setProgram);
	CONNECT_BTN_DEL(ui.btnProgramDel, ui.tblPrograms, setProgram);
	CONNECT_BTN_ADD(ui.btnSplitAdd, ui.tblSplits, setSplit);
	CONNECT_BTN_DEL(ui.btnSplitDel, ui.tblSplits, setSplit);

	/**
	 * The layers table has a fixed number of slots and as such using insertRow
	 * and removeRow doesn't properly fit our needs
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

	CONNECT_ROW_CHANGED(ui.tblPrograms->selectionModel(), setProgram);
	CONNECT_ROW_CHANGED(ui.tblLayers->selectionModel(), setLayer);
	CONNECT_ROW_CHANGED(ui.tblSplits->selectionModel(), setSplit);

	#undef CONNECT_BTN_ADD
	#undef CONNECT_BTN_DEL
	#undef CONNECT_ROW_CHANGED

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
		ui.btnSplitPlay->setIcon(QIcon::fromTheme(checked ? "media-playback-stop" : "media-playback-start"));
	});

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnSplitPlay->setChecked(tonePlayer.isPlaying());
	});
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank = manatools::mpb::load(path.toStdString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to load bank file: %1").arg(err.what()));
		return false;
	}

	cursor.restore();

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

	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank.save(path.toStdString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to save bank file: %1").arg(err.what()));
		return false;
	}

	cursor.restore();
	setCurrentFile(path);

	return true;
}

bool MainWindow::exportSF2File(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		auto sf2 = manatools::sf2::fromMPB(bank, QFileInfo(curFile).baseName().toStdString());
		sf2.Write(path.toStdString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, "", tr("Failed to export as SoundFont 2: %1").arg(err.what()));
		return false;
	}

	return true;
}

/**
 * Kinda really hate how this is done but Qt gives me a headache trying to do
 * anything else so
 * (Like, layer's going to be set twice in this case because setProgram fetches it,
 * and so does the slot that's connected to currentChanged after setCurrentIndex.
 * If you select another layer row before a file is loaded, the selection also won't
 * be synced with layerIdx 0.)
 */
void MainWindow::setProgram(size_t newProgramIdx) {
	programIdx = newProgramIdx;
	layersModel->setProgram(programIdx);
	splitsModel->setProgram(programIdx);
	ui.tblLayers->setCurrentIndex(layersModel->index(layerIdx, 0));
}

void MainWindow::setLayer(size_t newLayerIdx) {
	layerIdx = newLayerIdx;
	splitsModel->setLayer(layerIdx);
	ui.tblSplits->setCurrentIndex(splitsModel->index(splitIdx, 0));
}

void MainWindow::setSplit(size_t newSplitIdx) {
	splitIdx = newSplitIdx;
	tonePlayer.stop();

	const auto* split = bank.split(programIdx, layerIdx, splitIdx);
	tonePlayer.setTone(split ? split->tone : manatools::tone::Tone());
}

void MainWindow::newFile() {
	if (maybeSave()) {
		bank = {};
		bank.velocities.push_back(genDefVelCurve());
		setCurrentFile();
		reloadTables();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open MIDI program/drum bank"),
			getOutPath(true),
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
		getOutPath(),
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
			"<p>Envelope, LFO, fine tune, velocity, drum mode, layer delay and bend range data will not be "
			"preserved.</p>"
		)
	);

	const QString path = QFileDialog::getSaveFileName(
		this,
		tr("Export as SoundFont 2"),
		getOutPath(false, "sf2"),
		tr("SoundFont 2 (*.sf2)")
	);

	if (path.isEmpty())
		return false;

	return exportSF2File(path);
}

bool MainWindow::importTone() {
	bool success = tone::importDialog(
		this, bank,
		programIdx, layerIdx, splitIdx,
		getOutPath(true)
	);

	if (success)
		emitRowChanged(splitsModel, splitIdx);

	return success;
}

bool MainWindow::exportTone() {
	return tone::exportDialog(
		this, bank,
		programIdx, layerIdx, splitIdx,
		getOutPath(true),
		QFileInfo(curFile).baseName()
	);
}

void MainWindow::editBankProperties() {
	BankPropertiesDialog dlg(&bank, this);

	if (dlg.exec() == QDialog::Accepted) {
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

	SplitEditor editor(*split, &bank, this);
	editor.setPath(programIdx, layerIdx, splitIdx);

	if (editor.exec() == QDialog::Accepted) {
		/**
		 * unsafe, but this is a modal, so no other bank operations that may have invalidated this
		 * pointer should have happened (hopefully)
		 */
		*split = std::move(editor.split());
		// TODO: don't change windowModified if split data hasn't actually changed
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
			"<p>GUI for editing and converting Sega MIDI Program Bank and MIDI Drum Bank files used by Dreamcast titles.</p>"
			"<p>This is part of manatools. For more information, visit <a href=\"%2\">%2</a>.</p>"
		)
		.arg(QApplication::applicationVersion())
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
		Q_UNUSED(roles);
		if (roles.contains(Qt::DisplayRole) || roles.isEmpty()) {
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
	setProgram(0);
	setLayer(0);
	setSplit(0);
}

QString MainWindow::maybeDropEvent(QDropEvent* event) {
	const QMimeData* mimeData = event->mimeData();

	if (!mimeData->hasUrls())
		return {};

	const QList<QUrl> urls = mimeData->urls();

	if (urls.size() != 1)
		return {};

	const QString path = urls[0].toLocalFile();

	if (path.isEmpty())
		return {};

	if (!QFileInfo(path).isFile())
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
		setWindowFilePath(QDir::home().filePath("untitled.mpb"));
	}

	setWindowModified(false);
}

QString MainWindow::getOutPath(bool dirOnly, const QString& newExtension) const {
	if (curFile.isEmpty())
		return QDir::homePath();

	if (dirOnly)
		return QFileInfo(curFile).path();

	if (newExtension.isEmpty())
		return curFile;

	QFileInfo info(curFile);
	return info.dir().filePath(info.baseName() += '.' + newExtension);
}

manatools::mpb::Velocity MainWindow::genDefVelCurve() {
	manatools::mpb::Velocity vel;

	for (uint i = 0; i < std::size(vel.data); i++) {
		vel.data[i] = i;
	}

	return vel;
}

void MainWindow::restoreSettings() {
	if (!restoreGeometry(settings.value("MainWindow/Geometry").toByteArray())) {
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void MainWindow::saveSettings() {
	settings.setValue("MainWindow/Geometry", saveGeometry());
}
