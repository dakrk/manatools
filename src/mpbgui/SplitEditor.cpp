#include <QCloseEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QScreen>
#include <utility>
#include <guicommon/tone.hpp>
#include <guicommon/utils.hpp>

#include "SplitEditor.hpp"
#include "SplitUnkEditor.hpp"
#include "VelCurveEditor.hpp"

SplitEditor::SplitEditor(QWidget* parent) :
	QDialog(parent),
	settings(),
	tonePlayer(22050, this)
{
	init();
}

SplitEditor::SplitEditor(const Split& split, QWidget* parent) :
	QDialog(parent),
	split(split),
	settings(),
	tonePlayer(22050, this)
{
	init();
}

SplitEditor::SplitEditor(const Split& split, const std::vector<Velocity>& velocities, QWidget* parent) :
	QDialog(parent),
	split(split),
	velocities(velocities),
	settings(),
	tonePlayer(22050, this)
{
	init();
}

// bleh. not so pleased with this and how I force an update
#define CONNECT_AMP_SPINBOX_VALUE(spinbox, out) \
	connect(spinbox, &QSpinBox::valueChanged, this, [this](int i) { \
		out = i; \
		ui.ampEnvelope->update(); \
	});

#define CONNECT_FILTER_SPINBOX_VALUE(spinbox, out) \
	connect(spinbox, &QSpinBox::valueChanged, this, [this](int i) { \
		out = i; \
		ui.filterEnvelope->update(); \
	});

void SplitEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());
	setAcceptDrops(true);

	restoreSettings();

	editToneMenu = new QMenu();
	editToneMenu->addAction(ui.actionImportTone);
	editToneMenu->addAction(ui.actionExportTone);
	editToneMenu->addAction(ui.actionConvertToADPCM);
	ui.toolbtnToneEdit->setMenu(editToneMenu);

	connect(ui.spinBaseNote, &QSpinBox::valueChanged, this, [this](int i) {
		ui.lblBaseNoteVal->setText(noteToString(i));
	});

	connect(ui.btnTonePlay, &QPushButton::clicked, this, [this](bool checked) {
		checked ? tonePlayer.play() : tonePlayer.stop();
	});

	connect(ui.btnTonePlay, &QPushButton::toggled, this, [this](bool checked) {
		ui.btnTonePlay->setIcon(getPlaybackIcon(checked));
	});

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnTonePlay->setChecked(tonePlayer.isPlaying());
	});

	CONNECT_AMP_SPINBOX_VALUE(ui.spinAmpAttack, ui.ampEnvelope->amp.attackRate);
	CONNECT_AMP_SPINBOX_VALUE(ui.spinAmpDecay1, ui.ampEnvelope->amp.decayRate1);
	CONNECT_AMP_SPINBOX_VALUE(ui.spinAmpDecayLvl, ui.ampEnvelope->amp.decayLevel);
	CONNECT_AMP_SPINBOX_VALUE(ui.spinAmpDecay2, ui.ampEnvelope->amp.decayRate2);
	CONNECT_AMP_SPINBOX_VALUE(ui.spinAmpRelease, ui.ampEnvelope->amp.releaseRate);

	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterStartLvl, ui.filterEnvelope->filter.startLevel);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterAttackRate, ui.filterEnvelope->filter.attackRate);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterAttackLvl, ui.filterEnvelope->filter.attackLevel);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterDecayRate1, ui.filterEnvelope->filter.decayRate1);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterDecayLvl1, ui.filterEnvelope->filter.decayLevel1);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterDecayRate2, ui.filterEnvelope->filter.decayRate2);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterDecayLvl2, ui.filterEnvelope->filter.decayLevel2);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterReleaseRate, ui.filterEnvelope->filter.releaseRate);
	CONNECT_FILTER_SPINBOX_VALUE(ui.spinFilterReleaseLvl, ui.filterEnvelope->filter.releaseLevel);

	connect(ui.btnVelCurveEdit, &QPushButton::clicked, this, &SplitEditor::editVelCurve);
	connect(ui.actionImportTone, &QAction::triggered, this, &SplitEditor::importTone);
	connect(ui.actionExportTone, &QAction::triggered, this, &SplitEditor::exportTone);
	connect(ui.actionConvertToADPCM, &QAction::triggered, this, &SplitEditor::convertToADPCM);
	connect(ui.btnEditUnk, &QPushButton::clicked, this, &SplitEditor::editUnknownProps);

	connect(this, &QDialog::accepted, this, &SplitEditor::setSplitData);
	connect(this, &QDialog::finished, this, &SplitEditor::saveSettings);

	ui.btnTonePlay->setCheckable(true);

	addVelCurveItems(ui.comboVelCurve);
	addLFOWaveItems(ui.comboLFOPitchWave);
	addLFOWaveItems(ui.comboLFOAmpWave);

	loadSplitData();
}

void SplitEditor::addVelCurveItems(QComboBox* box) {
	for (u32 i = 0; i < velocities.size(); i++) {
		box->addItem(tr("Curve %1").arg(i));
	}
}

bool SplitEditor::setVelCurve(QComboBox* box, u32 id) {
	// the combo box contains an incrementing list so index instead of custom data will do
	if (std::cmp_greater_equal(id, box->count())) {
		QMessageBox::warning(
			this,
			tr("Invalid data"),
			tr("Invalid velocity curve ID encountered in split. (Got %1, max is %2)")
				.arg(id)
				.arg(static_cast<intptr_t>(velocities.size()) - 1)
		);
		return false;
	}

	box->setCurrentIndex(id);
	return true;
}

void SplitEditor::addLFOWaveItems(QComboBox* box) {
	box->addItem(tr("Saw"), static_cast<u8>(LFOWaveType::Saw));
	box->addItem(tr("Square"), static_cast<u8>(LFOWaveType::Square));
	box->addItem(tr("Triangle"), static_cast<u8>(LFOWaveType::Triangle));
	box->addItem(tr("Noise"), static_cast<u8>(LFOWaveType::Noise));
}

bool SplitEditor::setLFOWaveType(QComboBox* box, LFOWaveType type) {
	int index = box->findData(static_cast<u8>(type));
	if (index == -1) {
		QMessageBox::warning(
			this,
			tr("Invalid data"),
			tr("Invalid LFO wave type encountered in split. (Got %1)")
				.arg(static_cast<u8>(type))
		);
		return false;
	}

	box->setCurrentIndex(index);
	return true;
}

void SplitEditor::loadToneData() {
	tonePlayer.setTone(split.tone);
	ui.lblToneInfo->setText(
		tr("%n samples, %1", "", split.tone.samples())
			.arg(manatools::tone::formatName(split.tone.format))
	);
}

void SplitEditor::loadSplitData() {
	// In order of the Split struct in manatools/mpb.hpp
	ui.checkLoopOn->setChecked(split.loop);
	ui.spinLoopStart->setValue(split.loopStart);
	ui.spinLoopEnd->setValue(split.loopEnd);

	ui.spinAmpAttack->setValue(split.amp.attackRate);
	ui.spinAmpDecay1->setValue(split.amp.decayRate1);
	ui.spinAmpDecay2->setValue(split.amp.decayRate2);
	ui.spinAmpRelease->setValue(split.amp.releaseRate);
	ui.spinAmpDecayLvl->setValue(split.amp.decayLevel);
	ui.spinAmpKeyRateScaling->setValue(split.amp.keyRateScaling);
	ui.ampEnvelope->amp = split.amp;

	ui.checkLFOSync->setChecked(split.lfoOn);
	ui.spinLFOAmpDepth->setValue(split.lfo.ampDepth);
	setLFOWaveType(ui.comboLFOAmpWave, split.lfo.ampWave);
	ui.spinLFOPitchDepth->setValue(split.lfo.pitchDepth);
	setLFOWaveType(ui.comboLFOPitchWave, split.lfo.pitchWave);
	ui.spinLFOFreq->setValue(split.lfo.frequency);

	// SplitFX struct unhandled

	ui.spinPanPot->setValue(split.panPot);
	ui.spinDirectLevel->setValue(split.directLevel);

	ui.spinOscLvl->setValue(split.oscillatorLevel);

	ui.checkFilterOn->setChecked(split.filterOn);
	ui.spinFilterQ->setValue(split.filter.resonance);
	ui.spinFilterStartLvl->setValue(split.filter.startLevel);
	ui.spinFilterAttackLvl->setValue(split.filter.attackLevel);
	ui.spinFilterDecayLvl1->setValue(split.filter.decayLevel1);
	ui.spinFilterDecayLvl2->setValue(split.filter.decayLevel2);
	ui.spinFilterReleaseLvl->setValue(split.filter.releaseLevel);
	ui.spinFilterDecayRate1->setValue(split.filter.decayRate1);
	ui.spinFilterAttackRate->setValue(split.filter.attackRate);
	ui.spinFilterReleaseRate->setValue(split.filter.releaseRate);
	ui.spinFilterDecayRate2->setValue(split.filter.decayRate2);
	ui.filterEnvelope->filter = split.filter;

	// Start & end note unhandled :( (Should really increase window size)
	ui.spinBaseNote->setValue(split.baseNote);
	ui.spinFineTune->setValue(split.fineTune);

	setVelCurve(ui.comboVelCurve, split.velocityCurveID);
	ui.spinVelLow->setValue(split.velocityLow);
	ui.spinVelHigh->setValue(split.velocityHigh);

	ui.checkDrumModeOn->setChecked(split.drumMode);
	ui.spinDrumGroupID->setValue(split.drumGroupID);

	loadToneData();
}

void SplitEditor::setSplitData() {
	// In order of the Split struct in manatools/mpb.hpp
	split.loop = ui.checkLoopOn->isChecked();
	split.loopStart = ui.spinLoopStart->value();
	split.loopEnd = ui.spinLoopEnd->value();

	split.amp.attackRate = ui.spinAmpAttack->value();
	split.amp.decayRate1 = ui.spinAmpDecay1->value();
	split.amp.decayRate2 = ui.spinAmpDecay2->value();
	split.amp.releaseRate = ui.spinAmpRelease->value();
	split.amp.decayLevel = ui.spinAmpDecayLvl->value();
	split.amp.keyRateScaling = ui.spinAmpKeyRateScaling->value();

	split.lfoOn = ui.checkLFOSync->isChecked();
	split.lfo.ampDepth = ui.spinLFOAmpDepth->value();
	split.lfo.ampWave = ui.comboLFOAmpWave->currentData().value<LFOWaveType>();
	split.lfo.pitchDepth = ui.spinLFOPitchDepth->value();
	split.lfo.pitchWave = ui.comboLFOPitchWave->currentData().value<LFOWaveType>();
	split.lfo.frequency = ui.spinLFOFreq->value();

	// SplitFX struct unhandled

	split.panPot = ui.spinPanPot->value();
	split.directLevel = ui.spinDirectLevel->value();

	split.oscillatorLevel = ui.spinOscLvl->value();

	split.filterOn = ui.checkFilterOn->isChecked();
	split.filter.resonance = ui.spinFilterQ->value();
	split.filter.startLevel = ui.spinFilterStartLvl->value();
	split.filter.attackLevel = ui.spinFilterAttackLvl->value();
	split.filter.decayLevel1 = ui.spinFilterDecayLvl1->value();
	split.filter.decayLevel2 = ui.spinFilterDecayLvl2->value();
	split.filter.releaseLevel = ui.spinFilterReleaseLvl->value();
	split.filter.decayRate1 = ui.spinFilterDecayRate1->value();
	split.filter.attackRate = ui.spinFilterAttackRate->value();
	split.filter.releaseRate = ui.spinFilterReleaseRate->value();
	split.filter.decayRate2 = ui.spinFilterDecayRate2->value();

	// Strt & end note unhandled
	split.baseNote = ui.spinBaseNote->value();
	split.fineTune = ui.spinFineTune->value();

	split.velocityCurveID = ui.comboVelCurve->currentIndex();
	split.velocityLow = ui.spinVelLow->value();
	split.velocityHigh = ui.spinVelHigh->value();

	split.drumMode = ui.checkDrumModeOn->isChecked();
	split.drumGroupID = ui.spinDrumGroupID->value();
}

void SplitEditor::setCurFile(const QString& in) {
	curFile = in;
}

void SplitEditor::setPath(size_t programIdx, size_t layerIdx, size_t splitIdx) {
	pathSet = true;
	programIdx_ = programIdx;
	layerIdx_ = layerIdx;
	splitIdx_ = splitIdx;

	setWindowTitle(
		tr("Edit split [%1:%2:%3]")
			.arg(programIdx)
			.arg(layerIdx)
			.arg(splitIdx)
	);
}

void SplitEditor::editVelCurve() {
	VelCurveEditor editor(velocities, this);
	if (editor.exec() == QDialog::Accepted) {
		velocities = editor.velocities;
	}
}

bool SplitEditor::importTone() {
	auto metadata = tone::Metadata::fromMPB(split);
	bool success = tone::importDialog(split.tone, &metadata, getOutPath(curFile, true), this);
	if (success) {
		loadSplitData();
	}
	return success;
}

bool SplitEditor::exportTone() {
	QString tonePath;
	if (pathSet) {
		tonePath = QString("%1-%2-%3").arg(programIdx_).arg(layerIdx_).arg(splitIdx_);
	}

	auto metadata = tone::Metadata::fromMPB(split);
	return tone::exportDialog(
		split.tone,
		&metadata,
		getOutPath(curFile, true),
		QFileInfo(curFile).baseName(),
		tonePath,
		this
	);
}

void SplitEditor::convertToADPCM() {
	tone::convertToADPCM(split.tone, this);
	loadToneData();
}

void SplitEditor::editUnknownProps() {
	SplitUnkEditor editor(split, this);
	if (editor.exec() == QDialog::Accepted) {
		split = std::move(editor.split);
	}
}

void SplitEditor::closeEvent(QCloseEvent* event) {
	saveSettings();
	event->accept();
}

void SplitEditor::dragEnterEvent(QDragEnterEvent* event) {
	maybeDropEvent(event);
}

void SplitEditor::dropEvent(QDropEvent* event) {
	const QString path = maybeDropEvent(event);
	if (!path.isEmpty()) {
		auto metadata = tone::Metadata::fromMPB(split);
		if (tone::importFile(split.tone, &metadata, path, this)) {
			loadSplitData();
		}
	}
}

QString SplitEditor::maybeDropEvent(QDropEvent* event) {
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

void SplitEditor::restoreSettings() {
	if (!restoreGeometry(settings.value("SplitEditor/Geometry").toByteArray())) {
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void SplitEditor::saveSettings() {
	settings.setValue("SplitEditor/Geometry", saveGeometry());
}
