#include <QApplication>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <manatools/io.hpp>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/FourCCDelegate.hpp>
#include <guicommon/HorizontalLineItemDropStyle.hpp>
#include <guicommon/utils.hpp>
#include <functional>

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

	QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(QIcon::fromTheme("edit-delete"), tr("&Delete"), QKeySequence::Delete, this, &MainWindow::delUnit);
	editMenu->addAction(QIcon::fromTheme("edit-select-all"), tr("Select &All"), QKeySequence::SelectAll, this, &MainWindow::selectAll);
	editMenu->addSeparator();
	editMenu->addAction(QIcon::fromTheme("document-properties"), tr("MLT &Version"), this, &MainWindow::versionDialog);

	QMenu* packMenu = editMenu->addMenu(tr("&Pack"));
	packMenu->addAction(tr("By &AICA Size"), std::bind(&MainWindow::packMLT, this, true));
	packMenu->addAction(tr("By &File Size"), std::bind(&MainWindow::packMLT, this, false));
	// editMenu->addAction(QIcon::fromTheme("document-properties"), tr("Preference&s"), this, [] { /* TODO */ });

	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(QIcon::fromTheme("help-about"), tr("&About"), this, &MainWindow::about);
	helpMenu->addAction(tr("About Qt"), this, [this]() { QMessageBox::aboutQt(this); });

	// Have to explicitly construct a manatools::FourCC because std::bind & Qt stuff doesn't like doing it implicitly
	QMenu* unitTypeMenu = new QMenu(this);
	unitTypeMenu->addAction(tr("MIDI Sequence Bank [MSB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SMSB")));
	unitTypeMenu->addAction(tr("MIDI Program Bank [MPB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SMPB")));
	unitTypeMenu->addAction(tr("MIDI Drum Bank [MDB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SMDB")));
	unitTypeMenu->addAction(tr("One Shot Bank [OSB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SOSB")));
	unitTypeMenu->addAction(tr("FX Program Bank [FPB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SFPB")));
	unitTypeMenu->addAction(tr("FX Output Bank [FOB]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SFOB")));
	unitTypeMenu->addAction(tr("FX Program Work [FPW]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SFPW")));
	unitTypeMenu->addAction(tr("PCM Stream Ring Buffer [PSR]"), std::bind(&MainWindow::addUnit, this, manatools::FourCC("SPSR")));

	table = new QTableView(this);
	model = new MLTModel(&mlt, this);
	ramStatus = new QLabel(this);
	curUnitStatus = new QLabel(this);

	table->setDragEnabled(true);
	table->setDragDropMode(QAbstractItemView::InternalMove);
	table->setItemDelegateForColumn(0, new FourCCDelegate(true, table));
	table->setModel(model);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	table->setStyle(new HorizontalLineItemDropStyle(table->style()));

	setCurrentFile();
	resetTableLayout();
	updateRAMStatus();

	connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
	        [this](const QItemSelection& selected, const QItemSelection& deselected) {
		Q_UNUSED(selected);
		Q_UNUSED(deselected);
		updateUnitStatus();
	});

	connect(model, &QAbstractTableModel::dataChanged, this,
	        [this](const QModelIndex& tl, const QModelIndex& br, const QList<int>& roles) {
		Q_UNUSED(tl);
		Q_UNUSED(br);
		if (roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.isEmpty()) {
			dataModified();
		}
	});

	/**
	 * perhaps remove these...? these are operations that in mltgui also change currentRow,
	 * therefore causing updateUnitStatus to be called twice
	 */
	connect(model, &QAbstractTableModel::rowsInserted, this, &MainWindow::dataModified);
	connect(model, &QAbstractTableModel::rowsMoved, this, &MainWindow::dataModified);
	connect(model, &QAbstractTableModel::rowsRemoved, this, &MainWindow::dataModified);

	QToolButton* toolbtnAddUnit = new QToolButton();
	toolbtnAddUnit->setIcon(QIcon::fromTheme("list-add"));
	toolbtnAddUnit->setPopupMode(QToolButton::InstantPopup);
	toolbtnAddUnit->setMenu(unitTypeMenu);

	QPushButton* btnDelUnit = new QPushButton(QIcon::fromTheme("list-remove"), "");
	QPushButton* btnClearUnitData = new QPushButton(QIcon::fromTheme("edit-clear"), "");
	QPushButton* btnImportUnitData = new QPushButton(QIcon::fromTheme("document-open"), "");
	QPushButton* btnExportUnitData = new QPushButton(QIcon::fromTheme("document-save-as"), "");

	connect(btnDelUnit, &QPushButton::clicked, this, &MainWindow::delUnit);
	connect(btnClearUnitData, &QPushButton::clicked, this, &MainWindow::clearUnitData);
	connect(btnImportUnitData, &QPushButton::clicked, this, &MainWindow::importUnitDialog);
	connect(btnExportUnitData, &QPushButton::clicked, this, &MainWindow::exportUnitDialog);

	QHBoxLayout* btnLayout = new QHBoxLayout();
	btnLayout->addWidget(toolbtnAddUnit);
	btnLayout->addWidget(btnDelUnit);
	btnLayout->addStretch(1);
	btnLayout->addWidget(ramStatus);
	btnLayout->addStretch(1);
	btnLayout->addWidget(btnClearUnitData);
	btnLayout->addWidget(btnImportUnitData);
	btnLayout->addWidget(btnExportUnitData);

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(table);
	mainLayout->addLayout(btnLayout);

	QWidget* mainLayoutWidget = new QWidget();
	mainLayoutWidget->setLayout(mainLayout);
	setCentralWidget(mainLayoutWidget);

	statusBar()->insertWidget(0, curUnitStatus);
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		mlt = manatools::mlt::load(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Open Multi-Unit file"), tr("Failed to load multi-unit file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	reloadTable();
	updateRAMStatus();
	updateUnitStatus();
	return true;
}

bool MainWindow::saveFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	if (mlt.aicaUsed() > manatools::mlt::AICA_MAX) {
		const auto btn = QMessageBox::warning(
			this,
			tr("Save Multi-Unit file"),
			tr("The current file exceeds the maximum amount of AICA RAM, continue saving?"),
			QMessageBox::Yes | QMessageBox::No
		);

		if (btn != QMessageBox::Yes) {
			return false;
		}	
	}

	try {
		mlt.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Save Multi-Unit file"), tr("Failed to save multi-unit file: %1").arg(err.what()));
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
		updateRAMStatus();
		updateUnitStatus();
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

void MainWindow::dataModified() {
	setWindowModified(true);
	mlt.adjust();
	updateRAMStatus();
	updateUnitStatus();
}

void MainWindow::selectAll() {
	table->selectAll();
}

void MainWindow::versionDialog() {
	bool ok;
	auto verStr = QInputDialog::getText(
		this,
		tr("Set MLT version"),
		tr("Version (0x0101 for version 1):"),
		QLineEdit::Normal,
		formatHex(mlt.version),
		&ok
	);

	if (ok && !verStr.isEmpty()) {
		u32 newVer = verStr.toUInt(&ok, 0);
		if (ok && newVer != mlt.version) {
			mlt.version = newVer;
			dataModified();
		}
	}
}

void MainWindow::packMLT(bool useAICASizes) {
	if (mlt.pack(useAICASizes)) {
		/**
		 * Getting all rows that actually changed is a bit ugh, but it's inevitable
		 * once the time comes to implement undo/redo
		 * Emitting dataChanged is also kind of wasteful, as then a redundant
		 * mlt.adjust() would be ran
		 */
		emit model->dataChanged(
			model->index(0, 0),
			model->index(model->rowCount(), model->columnCount()),
			{ Qt::DisplayRole, Qt::EditRole }
		);
	}
}

bool MainWindow::addUnit(const manatools::FourCC fourCC) {
	QModelIndex cur = table->currentIndex();
	int row, col;

	if (cur.isValid()) {
		row = cur.row() + 1;
		col = cur.column();
	} else {
		row = 0;
		col = 0;
	}

	if (model->insertUnits(row, 1, fourCC)) {
		table->setCurrentIndex(model->index(row, col));
		return true;
	}

	return false;
}

void MainWindow::delUnit() {
	return removeSelectedViewItems(table);
}

void MainWindow::clearUnitData() {
	auto selRows = table->selectionModel()->selectedRows();
	int tl = -1;
	int br = -1;

	for (auto& idx : selRows) {
		auto& unit = mlt.units[idx.row()];
		if (!unit.data.empty()) {
			if (idx.row() < tl)
				tl = idx.row();
			else if (idx.row() > br)
				br = idx.row();

			unit.data.clear();
		}
	}

	if (tl != -1 || br != -1) {
		emit model->dataChanged(
			model->index(tl, 0),
			model->index(br, model->columnCount()),
			{ Qt::DisplayRole, Qt::EditRole }
		);
	}
}

bool MainWindow::importUnitDialog() {
	auto curIdx = table->selectionModel()->currentIndex();
	if (!curIdx.isValid())
		return false;

	auto& unit = mlt.units[curIdx.row()];

	const QString path = QFileDialog::getOpenFileName(
		this,
		tr("Import unit"),
		getOutPath(curFile, true),
		tr("Supported files (*.fob *.fpb *.mdb *.mpb *.msb *.osb);;All files (*.*)")
	);

	if (path.isEmpty())
		return false;

	// emitting rowChanged should be the responsibility of importUnit, but urghhhhh
	bool ret = importUnit(unit, path);
	if (ret)
		emitRowChanged(model, curIdx.row());

	return ret;
}

bool MainWindow::exportUnitDialog() {
	const auto selRows = table->selectionModel()->selectedRows();

	const QString defDir = getOutPath(curFile, true);
	const QString fileName = QFileInfo(curFile).baseName();
	QDir outDir(defDir);
	bool failed = false;

	auto genOutPath = [&fileName, &outDir](const manatools::mlt::Unit& unit, const QString& ext, int row) {
		return outDir.filePath(
			QString("%1_%2-%3.%4")
				.arg(fileName.isEmpty() ? "untitled" : fileName)
				.arg(row)
				.arg(unit.bank)
				.arg(ext.toLower())
		);
	};

	if (selRows.size() > 1) {
		const QString dir = QFileDialog::getExistingDirectory(this, tr("Export units to directory"), defDir);
		if (dir.isEmpty())
			return false;

		outDir = dir;

		for (const auto& idx : selRows) {
			const auto& unit = mlt.units[idx.row()];
			const QString path = genOutPath(unit, unit.fourCC.data() + 1, idx.row());
			failed |= !exportUnit(unit, path);
		}
	} else if (selRows.size() > 0) {
		const int row = selRows[0].row();
		const auto& unit = mlt.units[row];
		const QString ext = unit.fourCC.data() + 1;
		const QString path = QFileDialog::getSaveFileName(
			this,
			tr("Export unit"),
			genOutPath(unit, ext, row),
			tr("%1 file (*.%2);;All files (*.*)").arg(ext).arg(ext.toLower())
		);

		if (path.isEmpty())
			return false;

		failed |= !exportUnit(unit, path);
	} else {
		failed = true;
	}

	return !failed;
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
	CursorOverride cursor(Qt::WaitCursor);

	if (!unit.shouldHaveData()) {
		const auto btn = QMessageBox::warning(
			this,
			tr("Import unit"),
			tr("Selected unit is of type that should not contain data. Continue importing?"),
			QMessageBox::Yes | QMessageBox::No
		);

		if (btn != QMessageBox::Yes) {
			return false;
		}
	}

	try {
		long fileSize = 0;
		manatools::io::FileIO file(path.toStdWString(), "rb");
		u32 nextOffset = mlt.aicaNextOffset(unit.aicaDataPtr);

		file.end();
		fileSize = file.tell();

		if (fileSize > (nextOffset - unit.aicaDataPtr)) {
			const auto btn = QMessageBox::warning(
				this,
				tr("Import unit"),
				tr(
					"The size of the imported file exceeds the space available in this memory location.\n"
					"Importing would require relocating units that come after, thus impacting things that "
					"may require a specific mapping in memory. Would you like to continue?"
				),
				QMessageBox::Yes | QMessageBox::No
			);

			if (btn != QMessageBox::Yes) {
				return false;
			}
		}

		// Should be padded 'n' all by a subsequent adjust call
		unit.aicaDataSize = fileSize;

		unit.data.resize(fileSize);
		file.jump(0);
		file.readVec(unit.data);
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Import unit"), tr("Failed to import unit: %1").arg(err.what()));
		return false;
	}

	return true;
}

bool MainWindow::exportUnit(const manatools::mlt::Unit& unit, const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	// This should be done the moment you click the button but, redundant checks much?
	if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
		cursor.restore();
		QMessageBox::warning(this, tr("Export unit"), tr("A selected unit has no data (no file offset), cannot export it."));
		return false;
	}

	try {
		manatools::io::FileIO file(path.toStdWString(), "wb");
		file.writeVec(unit.data);
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Export unit"), tr("Failed to export unit: %1").arg(err.what()));
		return false;
	}
	
	return true;
}

void MainWindow::emitRowChanged(QAbstractItemModel* model, int row) {
	auto topLeft = model->index(row, 0);
	auto bottomRight = model->index(row, model->columnCount());
	emit model->dataChanged(topLeft, bottomRight, { Qt::DisplayRole, Qt::EditRole });
}

void MainWindow::updateRAMStatus() {
	intptr_t avail = manatools::mlt::AICA_MAX - mlt.aicaUsed();
	ramStatus->setText(tr("%1 (%2) bytes available").arg(avail).arg(formatHex(avail)));

	if (avail <= 0)
		ramStatus->setStyleSheet("QLabel { color: red; }");
	else if (avail <= 131072) // AICA_MAX / 16
		ramStatus->setStyleSheet("QLabel { color: orange; }");
	else
		ramStatus->setStyleSheet("");
}

void MainWindow::updateUnitStatus() {
	const auto selRows = table->selectionModel()->selectedRows();
	QString str;

	if (selRows.size() > 1) {
		size_t totalSize = 0;

		// these indexes should not be invalid I hope...
		for (const auto& idx : selRows) {
			totalSize += mlt.units[idx.row()].aicaDataSize;
		}

		str = tr("%1 units selected, total size %2 bytes")
				.arg(selRows.size())
				.arg(totalSize);
	} else if (selRows.size() > 0) {
		const int row = selRows[0].row();
		const auto& unit = mlt.units[row];
		str = tr("Unit %1: %2 bytes, minimum end 0x%3")
				.arg(row)
				.arg(unit.aicaDataSize)
				.arg(unit.aicaDataPtr + unit.aicaDataSize, 0, 16);
	}

	curUnitStatus->setText(str);
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
