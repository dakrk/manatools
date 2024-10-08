#include <QApplication>
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
		}
	});

	QPushButton* btnAdd = new QPushButton(QIcon::fromTheme("list-add"), "");
	QPushButton* btnDel = new QPushButton(QIcon::fromTheme("list-remove"), "");

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
		levelSliders[i] = sliderLevel;

		QLabel* lblLevel = new QLabel();

		QSlider* sliderPan = new QSlider(Qt::Horizontal);
		sliderPan->setMinimum(-15);
		sliderPan->setMaximum(15);
		panSliders[i] = sliderPan;

		QLabel* lblPan = new QLabel();

		auto row = mixerLayout->rowCount();

		mixerLayout->addWidget(new QLabel(tr("Channel %1").arg(i)), row, 0);
		mixerLayout->addWidget(sliderLevel, row, 1);
		mixerLayout->addWidget(lblLevel, row, 2);
		mixerLayout->addWidget(sliderPan, row, 3);
		mixerLayout->addWidget(lblPan, row, 4);

		connect(sliderLevel, &QAbstractSlider::valueChanged, this, [this, lblLevel](int value) {
			lblLevel->setNum(value);
			setWindowModified(true);
		});

		connect(sliderPan, &QAbstractSlider::valueChanged, this, [this, lblPan](int value) {
			lblPan->setNum(value);
			setWindowModified(true);
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

	setCurrentFile(path);
	model->setBank(&bank);
	return true;
}

bool MainWindow::saveFile(const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	try {
		bank.save(path.toStdWString());
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(this, tr("Save FX output bank file"), tr("Failed to save FX output bank file: %1").arg(err.what()));
		return false;
	}

	setCurrentFile(path);
	return true;
}

void MainWindow::newFile() {
	if (maybeSave()) {
		bank = {};
		setCurrentFile();
		model->setBank(&bank);
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
