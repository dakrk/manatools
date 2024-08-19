#include <QScreen>
#include <QMessageBox>
#include <QCloseEvent>
#include <QThread>
#include <QMenu>
#include <utility>

#include "SplitEditor.hpp"
#include "SplitUnkEditor.hpp"
#include "tone.hpp"
#include "utilities.hpp"

SplitEditor::SplitEditor(QWidget* parent) :
	QDialog(parent),
	settings(),
	bank(nullptr),
	tonePlayer(22050, this)
{
	init();
}

SplitEditor::SplitEditor(const Split& split, Bank* bank, QWidget* parent) :
	QDialog(parent),
	settings(),
	split_(split),
	bank(bank),
	tonePlayer(22050, this)
{
	init();
}

void SplitEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());

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
		ui.btnTonePlay->setIcon(QIcon::fromTheme(checked ? "media-playback-stop" : "media-playback-start"));
	});

	connect(ui.actionImportTone, &QAction::triggered, this, &SplitEditor::importTone);
	connect(ui.actionExportTone, &QAction::triggered, this, &SplitEditor::exportTone);
	connect(ui.actionConvertToADPCM, &QAction::triggered, this, &SplitEditor::convertToADPCM);
	connect(ui.btnEditUnk, &QPushButton::clicked, this, &SplitEditor::editUnknownProps);

	connect(this, &QDialog::accepted, this, &SplitEditor::setSplitData);
	connect(this, &QDialog::finished, this, &SplitEditor::saveSettings);

	connect(&tonePlayer, &TonePlayer::playingChanged, this, [this]() {
		ui.btnTonePlay->setChecked(tonePlayer.isPlaying());
	});

	ui.btnTonePlay->setCheckable(true);

	addVelCurveItems(ui.comboVelCurve);
	ui.btnVelCurveEdit->setEnabled(false); // TODO: Velocity curve editor

	addLFOWaveItems(ui.comboLFOPitchWave);
	addLFOWaveItems(ui.comboLFOAmpWave);

	loadSplitData();
}

void SplitEditor::addVelCurveItems(QComboBox* box) {
	if (!bank)
		return;

	for (u32 i = 0; i < bank->velocities.size(); i++) {
		box->addItem(tr("Curve %1").arg(i + 1));
	}
}

bool SplitEditor::setVelCurve(QComboBox* box, u32 id) {
	if (!bank)
		return false;

	// the combo box contains an incrementing list so index instead of custom data will do
	if (std::cmp_greater_equal(id, box->count())) {
		QMessageBox::warning(
			this,
			tr("Invalid data"),
			tr("Invalid velocity curve ID encountered in split. (Got %1, max is %2)")
				.arg(id).arg(bank->velocities.size())
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
	tonePlayer.setTone(split_.tone);
	ui.lblToneInfo->setText(
		tr("%n samples, %1", "", split_.tone.samples())
			.arg(manatools::tone::formatName(split_.tone.format))
	);
}

void SplitEditor::loadSplitData() {
	// In order of the Split struct in manatools/mpb.hpp
	ui.checkLoopOn->setChecked(split_.loop);
	ui.spinLoopStart->setValue(split_.loopStart);
	ui.spinLoopEnd->setValue(split_.loopEnd);

	ui.spinAmpAttack->setValue(split_.amp.attackRate);
	ui.spinAmpDecay1->setValue(split_.amp.decayRate1);
	ui.spinAmpDecay2->setValue(split_.amp.decayRate2);
	ui.spinAmpRelease->setValue(split_.amp.releaseRate);
	ui.spinAmpDecayLvl->setValue(split_.amp.decayLevel);
	ui.spinAmpKeyRateScaling->setValue(split_.amp.keyRateScaling);

	ui.checkLFOSync->setChecked(split_.lfoOn);
	ui.spinLFOAmpDepth->setValue(split_.lfo.ampDepth);
	setLFOWaveType(ui.comboLFOAmpWave, split_.lfo.ampWave);
	ui.spinLFOPitchDepth->setValue(split_.lfo.pitchDepth);
	setLFOWaveType(ui.comboLFOPitchWave, split_.lfo.pitchWave);
	ui.spinLFOFreq->setValue(split_.lfo.frequency);

	// SplitFX struct unhandled

	ui.spinPanPot->setValue(split_.panPot);
	ui.spinDirectLevel->setValue(split_.directLevel);

	ui.spinOscLvl->setValue(split_.oscillatorLevel);

	ui.checkFilterOn->setChecked(split_.filterOn);
	ui.spinFilterQ->setValue(split_.filter.resonance);
	ui.spinFilterStartLvl->setValue(split_.filter.startLevel);
	ui.spinFilterAttackLvl->setValue(split_.filter.attackLevel);
	ui.spinFilterDecayLvl1->setValue(split_.filter.decayLevel1);
	ui.spinFilterDecayLvl2->setValue(split_.filter.decayLevel2);
	ui.spinFilterReleaseLvl->setValue(split_.filter.releaseLevel);
	ui.spinFilterDecayRate1->setValue(split_.filter.decayRate1);
	ui.spinFilterAttackRate->setValue(split_.filter.attackRate);
	ui.spinFilterReleaseRate->setValue(split_.filter.releaseRate);
	ui.spinFilterDecayRate2->setValue(split_.filter.decayRate2);

	// Start & end note unhandled :( (Should really increase window size)
	ui.spinBaseNote->setValue(split_.baseNote);
	ui.spinFineTune->setValue(split_.fineTune);

	setVelCurve(ui.comboVelCurve, split_.velocityCurveID);
	ui.spinVelLow->setValue(split_.velocityLow);
	ui.spinVelHigh->setValue(split_.velocityHigh);

	ui.checkDrumModeOn->setChecked(split_.drumMode);
	ui.spinDrumGroupID->setValue(split_.drumGroupID);

	loadToneData();
}

void SplitEditor::setSplitData() {
	// In order of the Split struct in manatools/mpb.hpp
	split_.loop = ui.checkLoopOn->isChecked();
	split_.loopStart = ui.spinLoopStart->value();
	split_.loopEnd = ui.spinLoopEnd->value();

	split_.amp.attackRate = ui.spinAmpAttack->value();
	split_.amp.decayRate1 = ui.spinAmpDecay1->value();
	split_.amp.decayRate2 = ui.spinAmpDecay2->value();
	split_.amp.releaseRate = ui.spinAmpRelease->value();
	split_.amp.decayLevel = ui.spinAmpDecayLvl->value();
	split_.amp.keyRateScaling = ui.spinAmpKeyRateScaling->value();

	split_.lfoOn = ui.checkLFOSync->isChecked();
	split_.lfo.ampDepth = ui.spinLFOAmpDepth->value();
	split_.lfo.ampWave = ui.comboLFOAmpWave->currentData().value<LFOWaveType>();
	split_.lfo.pitchDepth = ui.spinLFOPitchDepth->value();
	split_.lfo.pitchWave = ui.comboLFOPitchWave->currentData().value<LFOWaveType>();
	split_.lfo.frequency = ui.spinLFOFreq->value();

	// SplitFX struct unhandled

	split_.panPot = ui.spinPanPot->value();
	split_.directLevel = ui.spinDirectLevel->value();

	split_.oscillatorLevel = ui.spinOscLvl->value();

	split_.filterOn = ui.checkFilterOn->isChecked();
	split_.filter.resonance = ui.spinFilterQ->value();
	split_.filter.startLevel = ui.spinFilterStartLvl->value();
	split_.filter.attackLevel = ui.spinFilterAttackLvl->value();
	split_.filter.decayLevel1 = ui.spinFilterDecayLvl1->value();
	split_.filter.decayLevel2 = ui.spinFilterDecayLvl2->value();
	split_.filter.releaseLevel = ui.spinFilterReleaseLvl->value();
	split_.filter.decayRate1 = ui.spinFilterDecayRate1->value();
	split_.filter.attackRate = ui.spinFilterAttackRate->value();
	split_.filter.releaseRate = ui.spinFilterReleaseRate->value();
	split_.filter.decayRate2 = ui.spinFilterDecayRate2->value();

	// Start & end note unhandled
	split_.baseNote = ui.spinBaseNote->value();
	split_.fineTune = ui.spinFineTune->value();

	split_.velocityCurveID = ui.comboVelCurve->currentIndex();
	split_.velocityLow = ui.spinVelLow->value();
	split_.velocityHigh = ui.spinVelHigh->value();

	split_.drumMode = ui.checkDrumModeOn->isChecked();
	split_.drumGroupID = ui.spinDrumGroupID->value();
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
		QString("%1 [%2:%3:%4]")
			.arg(tr("Edit split"))
			.arg(programIdx + 1)
			.arg(layerIdx + 1)
			.arg(splitIdx + 1)
	);
}

bool SplitEditor::importTone() {
	bool success = tone::importDialog(this, split_, getOutPath(curFile, true));

	if (success)
		loadSplitData();

	return success;
}

bool SplitEditor::exportTone() {
	QString tonePath;
	
	if (pathSet)
		tonePath = QString("%1-%2-%3").arg(programIdx_ + 1).arg(layerIdx_ + 1).arg(splitIdx_ + 1);

	return tone::exportDialog(this, split_, getOutPath(curFile, true), QFileInfo(curFile).baseName(), tonePath);
}

void SplitEditor::convertToADPCM() {
	loadToneData();
}

void SplitEditor::editUnknownProps() {
	SplitUnkEditor editor(split_, this);

	if (editor.exec() == QDialog::Accepted) {
		split_ = std::move(editor.split());
	}
}

void SplitEditor::closeEvent(QCloseEvent* event) {
	saveSettings();
	event->accept();
}

void SplitEditor::restoreSettings() {
	if (!restoreGeometry(settings.value("SplitEditor/Geometry").toByteArray())) {
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void SplitEditor::saveSettings() {
	settings.setValue("SplitEditor/Geometry", saveGeometry());
}
