#include <QApplication>
#include <QCheckBox>
#include <QDropEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <manatools/io.hpp>
#include <guicommon/CSV.hpp>
#include <guicommon/CursorOverride.hpp>
#include <guicommon/utils.hpp>

#include "MainWindow.hpp"
#include "fobgui.hpp"

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
	editMenu->addAction(QIcon::fromTheme("document-properties"), tr("&Version"), this, &MainWindow::versionDialog);

	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(QIcon::fromTheme("help-about"), tr("&About"), this, &MainWindow::about);
	helpMenu->addAction(tr("About Qt"), this, [this]() { QMessageBox::aboutQt(this); });

	setCurrentFile();

	list = new QListView();
	model = new FOBModel(&bank);
	list->setModel(model);

	connect(list->selectionModel(), &QItemSelectionModel::currentChanged, this,
	        [&](const QModelIndex& current, const QModelIndex& previous)
	{
		if (previous.isValid()) {
			saveMixerData(bank.mixers[previous.row()]);
		}

		if (current.isValid()) {
			loadMixerData(bank.mixers[current.row()]);
		} else {
			loadMixerData({});
		}

		for (uint i = 0; i < manatools::fob::CHANNELS; i++) {
			levelSliders[i]->setEnabled(current.isValid());
			panSliders[i]->setEnabled(current.isValid());
		}
	});

	QPushButton* btnAdd = new QPushButton(QIcon::fromTheme("list-add"), "");
	QPushButton* btnDel = new QPushButton(QIcon::fromTheme("list-remove"), "");

	connect(btnAdd, &QPushButton::clicked, this, [this]() {
		insertItemRowHere(list);
	});

	connect(btnDel, &QPushButton::clicked, this, [this]() {
		removeSelectedViewItems(list);
	});

	QHBoxLayout* listButtonsLayout = new QHBoxLayout();
	listButtonsLayout->addWidget(btnAdd);
	listButtonsLayout->addWidget(btnDel);
	listButtonsLayout->addStretch(1);

	QVBoxLayout* listLayout = new QVBoxLayout();
	listLayout->addWidget(list);
	listLayout->addLayout(listButtonsLayout);
	listLayout->setContentsMargins(0, 0, 0, 0);

	// dumb widget because Qt doesn't let you set the width of a layout itself
	QWidget* listLayoutContainer = new QWidget();
	listLayoutContainer->setLayout(listLayout);
	listLayoutContainer->setMaximumWidth(150);

	QFont headingFont = font();
	headingFont.setBold(true);

	QLabel* lblChannel = new QLabel(tr("Channel"));
	lblChannel->setAlignment(Qt::AlignHCenter);
	lblChannel->setFont(headingFont);

	QLabel* lblFXLevel = new QLabel(tr("FX Level"));
	lblFXLevel->setAlignment(Qt::AlignHCenter);
	lblFXLevel->setFont(headingFont);

	QLabel* lblFXPan = new QLabel(tr("FX Pan"));
	lblFXPan->setAlignment(Qt::AlignHCenter);
	lblFXPan->setFont(headingFont);

	QGridLayout* mixerLayout = new QGridLayout();
	mixerLayout->addWidget(lblChannel, 0, 0);
	mixerLayout->addWidget(lblFXLevel, 0, 1, 1, 1);
	mixerLayout->addWidget(lblFXPan, 0, 3, 1, 1);

	levelSliders.resize(manatools::fob::CHANNELS);
	panSliders.resize(manatools::fob::CHANNELS);

	for (uint i = 0; i < manatools::fob::CHANNELS; i++) {
		QSlider* sliderLevel = new QSlider(Qt::Horizontal);
		sliderLevel->setMaximum(15);
		sliderLevel->setEnabled(false);
		levelSliders[i] = sliderLevel;

		QLabel* lblLevel = new QLabel();
		lblLevel->setNum(0);

		QSlider* sliderPan = new QSlider(Qt::Horizontal);
		sliderPan->setMinimum(-15);
		sliderPan->setMaximum(15);
		sliderPan->setEnabled(false);
		panSliders[i] = sliderPan;

		QLabel* lblPan = new QLabel();
		lblPan->setNum(0);

		auto row = mixerLayout->rowCount();

		mixerLayout->addWidget(new QLabel(tr("Channel %1").arg(i)), row, 0);
		mixerLayout->addWidget(sliderLevel, row, 1);
		mixerLayout->addWidget(lblLevel, row, 2);
		mixerLayout->addWidget(sliderPan, row, 3);
		mixerLayout->addWidget(lblPan, row, 4);

		connect(sliderLevel, &QAbstractSlider::valueChanged, this, [lblLevel](int value) {
			lblLevel->setNum(value);
		});

		connect(sliderLevel, &QAbstractSlider::actionTriggered, this, [this](int action) {
			if (action == QAbstractSlider::SliderMove) {
				setWindowModified(true);
			}
		});

		connect(sliderPan, &QAbstractSlider::valueChanged, this, [lblPan](int value) {
			lblPan->setNum(value);
		});

		connect(sliderPan, &QAbstractSlider::actionTriggered, this, [this](int action) {
			if (action == QAbstractSlider::SliderMove) {
				setWindowModified(true);
			}
		});
	}

	mixerLayout->setRowStretch(mixerLayout->rowCount(), 1);
	mixerLayout->setColumnMinimumWidth(2, 30);
	mixerLayout->setColumnMinimumWidth(4, 30);

	QHBoxLayout* mainLayout = new QHBoxLayout();
	mainLayout->addWidget(listLayoutContainer);
	mainLayout->addLayout(mixerLayout);
	mainLayout->setStretch(1, 1);

	QWidget* mainLayoutWidget = new QWidget();
	mainLayoutWidget->setLayout(mainLayout);
	setCentralWidget(mainLayoutWidget);
}

bool MainWindow::loadFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank = manatools::fob::load(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Open FX output bank file"), tr("Failed to load FX output bank file: %1").arg(err.what()));
		return false;
	}

	cursor.restore();

	if (!loadMapFile(path % ".csv")) {
		loadMapFile(getOutPath(path, true) % "/manatools_fob_map.csv");
	}

	setCurrentFile(path);
	model->setBank(&bank);

	if (!bank.mixers.empty()) {
		list->setCurrentIndex(model->index(0, 0));
	} else {
		resetSliders();
	}

	saveMappings.reset();
	return true;
}

bool MainWindow::saveFile(const QString& path) {
	bool doSaveMappings = saveMappingsDialog();
	CursorOverride cursor(Qt::WaitCursor);

	auto index = list->currentIndex();
	if (index.isValid()) {
		saveMixerData(bank.mixers[index.row()]);
	}

	try {
		bank.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Save FX output bank file"), tr("Failed to save FX output bank file: %1").arg(err.what()));
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

		if (!ok || index + 1 > bank.mixers.size()) {
			QMessageBox::warning(
				this,
				tr("Load map file"),
				tr("Map file contains invalid/out of bounds index: %1").arg(cols[0])
			);
			continue;
		}

		bank.mixers[index].userData = std::move(cols[1]);
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

	for (size_t i = 0; i < bank.mixers.size(); i++) {
		const QString* name = std::any_cast<QString>(&bank.mixers[i].userData);
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
		setCurrentFile();
		model->setBank(&bank);
		resetSliders();
		saveMappings.reset();
	}
}

bool MainWindow::open() {
	if (maybeSave()) {
		const QString path = QFileDialog::getOpenFileName(
			this,
			tr("Open FX output bank file"),
			getOutPath(curFile, true),
			tr("FX output bank file (*.fob);;All files (*.*)")
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
		tr("Save FX output bank file"),
		getOutPath(curFile),
		tr("FX output bank file (*.fob);;All files (*.*)")
	);

	if (path.isEmpty())
		return false;

	return saveFile(path);
}

void MainWindow::about() {
	QMessageBox::about(
		this,
		tr("About fobgui"),
		tr(
			"<h3>About fobgui</h3>"
			"<small>Version %1</small>"
			"<p>%2</p>"
			"<p>This is part of manatools. For more information, visit <a href=\"%3\">%3</a>.</p>"
		)
		.arg(QApplication::applicationVersion())
		.arg(tr(APP_DESCRIPTION))
		.arg("https://github.com/dakrk/manatools")
	);
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

void MainWindow::loadMixerData(const manatools::fob::Mixer& mixer) {
	for (uint i = 0; i < manatools::fob::CHANNELS; i++) {
		levelSliders[i]->setValue(mixer.level[i]);
		panSliders[i]->setValue(mixer.pan[i]);
	}
}

void MainWindow::saveMixerData(manatools::fob::Mixer& mixer) {
	for (uint i = 0; i < manatools::fob::CHANNELS; i++) {
		mixer.level[i] = levelSliders[i]->value();
		mixer.pan[i] = panSliders[i]->value();
	}
}

void MainWindow::resetSliders() {
	for (uint i = 0; i < manatools::fob::CHANNELS; i++) {
		levelSliders[i]->setEnabled(false);
		levelSliders[i]->setValue(0);
		panSliders[i]->setEnabled(false);
		panSliders[i]->setValue(0);
	}	
}

bool MainWindow::mixerNameSet() const {
	for (const auto& mixer : bank.mixers) {
		const QString* name = std::any_cast<QString>(&mixer.userData);
		if (name && !name->isEmpty()) {
			return true;
		}
	}
	return false;
}

bool MainWindow::saveMappingsDialog() {
	if (saveMappings.has_value()) {
		return *saveMappings;
	} else if (mixerNameSet()) {
		QMessageBox mb(
			QMessageBox::Question,
			tr("Save map file"),
			tr("Mixer names were set in this bank. Would you like to save the mapping file with the bank?"),
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
		setWindowFilePath(QDir::home().filePath("untitled.fob"));
	}

	setWindowModified(false);
}

void MainWindow::restoreSettings() {
	if (!restoreGeometry(settings.value("MainWindow/Geometry").toByteArray())) {
		resize(480, 425);
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void MainWindow::saveSettings() {
	settings.setValue("MainWindow/Geometry", saveGeometry());
}
