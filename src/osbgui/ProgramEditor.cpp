#include <QCloseEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QScreen>
#include <guicommon/tone.hpp>
#include <guicommon/utils.hpp>

#include "ProgramEditor.hpp"
#include "ProgramMiscEditor.hpp"

ProgramEditor::ProgramEditor(QWidget* parent) :
	QDialog(parent),
	settings(),
	tonePlayer(this)
{
	init();
}

ProgramEditor::ProgramEditor(const Program& program, QWidget* parent) :
	QDialog(parent),
	program(program),
	settings(),
	tonePlayer(this)
{
	init();
}

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

void ProgramEditor::init() {
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

	connect(ui.actionImportTone, &QAction::triggered, this, &ProgramEditor::importTone);
	connect(ui.actionExportTone, &QAction::triggered, this, &ProgramEditor::exportTone);
	connect(ui.actionConvertToADPCM, &QAction::triggered, this, &ProgramEditor::convertToADPCM);
	connect(ui.btnEditMisc, &QPushButton::clicked, this, &ProgramEditor::editUnknownProps);

	connect(this, &QDialog::accepted, this, &ProgramEditor::setProgramData);
	connect(this, &QDialog::finished, this, &ProgramEditor::saveSettings);

	ui.btnTonePlay->setCheckable(true);

	addLFOWaveItems(ui.comboLFOPitchWave);
	addLFOWaveItems(ui.comboLFOAmpWave);

	loadProgramData();
}

void ProgramEditor::addLFOWaveItems(QComboBox* box) {
	box->addItem(tr("Saw"), static_cast<u8>(LFOWaveType::Saw));
	box->addItem(tr("Square"), static_cast<u8>(LFOWaveType::Square));
	box->addItem(tr("Triangle"), static_cast<u8>(LFOWaveType::Triangle));
	box->addItem(tr("Noise"), static_cast<u8>(LFOWaveType::Noise));
}

bool ProgramEditor::setLFOWaveType(QComboBox* box, LFOWaveType type) {
	int index = box->findData(static_cast<u8>(type));
	if (index == -1) {
		QMessageBox::warning(
			this,
			tr("Invalid data"),
			tr("Invalid LFO wave type encountered in program. (Got %1)")
				.arg(static_cast<u8>(type))
		);
		return false;
	}

	box->setCurrentIndex(index);
	return true;
}

void ProgramEditor::loadToneData() {
	tonePlayer.setTone(program.tone);
	ui.lblToneInfo->setText(
		tr("%n samples, %1", "", program.tone.samples())
			.arg(manatools::tone::formatName(program.tone.format))
	);
}

void ProgramEditor::loadProgramData() {
	// In order of the Program struct in manatools/osb.hpp
	ui.checkLoopOn->setChecked(program.loop);
	ui.spinLoopStart->setValue(program.loopStart);
	ui.spinLoopEnd->setValue(program.loopEnd);

	ui.spinAmpAttack->setValue(program.amp.attackRate);
	ui.spinAmpDecay1->setValue(program.amp.decayRate1);
	ui.spinAmpDecay2->setValue(program.amp.decayRate2);
	ui.spinAmpRelease->setValue(program.amp.releaseRate);
	ui.spinAmpDecayLvl->setValue(program.amp.decayLevel);
	ui.spinAmpKeyRateScaling->setValue(program.amp.keyRateScaling);
	ui.ampEnvelope->amp = program.amp;

	ui.checkLFOSync->setChecked(program.lfo.sync);
	ui.spinLFOAmpDepth->setValue(program.lfo.ampDepth);
	setLFOWaveType(ui.comboLFOAmpWave, program.lfo.ampWave);
	ui.spinLFOPitchDepth->setValue(program.lfo.pitchDepth);
	setLFOWaveType(ui.comboLFOPitchWave, program.lfo.pitchWave);
	ui.spinLFOFreq->setValue(program.lfo.frequency);

	// ProgramFX struct unhandled

	ui.spinPanPot->setValue(program.panPot);
	ui.spinDirectLevel->setValue(program.directLevel);

	ui.spinOscLvl->setValue(program.oscillatorLevel);

	ui.checkFilterOn->setChecked(program.filter.on);
	ui.checkFilterVOFF->setChecked(program.filter.voff);
	ui.spinFilterQ->setValue(program.filter.resonance);
	ui.spinFilterStartLvl->setValue(program.filter.startLevel);
	ui.spinFilterAttackLvl->setValue(program.filter.attackLevel);
	ui.spinFilterDecayLvl1->setValue(program.filter.decayLevel1);
	ui.spinFilterDecayLvl2->setValue(program.filter.decayLevel2);
	ui.spinFilterReleaseLvl->setValue(program.filter.releaseLevel);
	ui.spinFilterDecayRate1->setValue(program.filter.decayRate1);
	ui.spinFilterAttackRate->setValue(program.filter.attackRate);
	ui.spinFilterReleaseRate->setValue(program.filter.releaseRate);
	ui.spinFilterDecayRate2->setValue(program.filter.decayRate2);
	ui.filterEnvelope->filter = program.filter;

	/**
	 * Would like to indicate that this value isn't exact, like I do in
	 * mpbgui's LayerEditor, but there's no space
	 */
	ui.spinLoopTime->setValue(program.loopTime * 4);
	ui.spinBaseNote->setValue(program.baseNote);

	loadToneData();
}

void ProgramEditor::setProgramData() {
	// In order of the Program struct in manatools/osb.hpp
	program.loop = ui.checkLoopOn->isChecked();
	program.loopStart = ui.spinLoopStart->value();
	program.loopEnd = ui.spinLoopEnd->value();

	program.amp.attackRate = ui.spinAmpAttack->value();
	program.amp.decayRate1 = ui.spinAmpDecay1->value();
	program.amp.decayRate2 = ui.spinAmpDecay2->value();
	program.amp.releaseRate = ui.spinAmpRelease->value();
	program.amp.decayLevel = ui.spinAmpDecayLvl->value();
	program.amp.keyRateScaling = ui.spinAmpKeyRateScaling->value();

	program.lfo.sync = ui.checkLFOSync->isChecked();
	program.lfo.ampDepth = ui.spinLFOAmpDepth->value();
	program.lfo.ampWave = ui.comboLFOAmpWave->currentData().value<LFOWaveType>();
	program.lfo.pitchDepth = ui.spinLFOPitchDepth->value();
	program.lfo.pitchWave = ui.comboLFOPitchWave->currentData().value<LFOWaveType>();
	program.lfo.frequency = ui.spinLFOFreq->value();

	// ProgramFX struct unhandled

	program.panPot = ui.spinPanPot->value();
	program.directLevel = ui.spinDirectLevel->value();

	program.oscillatorLevel = ui.spinOscLvl->value();

	program.filter.on = ui.checkFilterOn->isChecked();
	program.filter.voff = ui.checkFilterVOFF->isChecked();
	program.filter.resonance = ui.spinFilterQ->value();
	program.filter.startLevel = ui.spinFilterStartLvl->value();
	program.filter.attackLevel = ui.spinFilterAttackLvl->value();
	program.filter.decayLevel1 = ui.spinFilterDecayLvl1->value();
	program.filter.decayLevel2 = ui.spinFilterDecayLvl2->value();
	program.filter.releaseLevel = ui.spinFilterReleaseLvl->value();
	program.filter.decayRate1 = ui.spinFilterDecayRate1->value();
	program.filter.attackRate = ui.spinFilterAttackRate->value();
	program.filter.releaseRate = ui.spinFilterReleaseRate->value();
	program.filter.decayRate2 = ui.spinFilterDecayRate2->value();

	program.loopTime = ui.spinLoopTime->value() / 4;
	program.baseNote = ui.spinBaseNote->value();
}

void ProgramEditor::setCurFile(const QString& in) {
	curFile = in;
}

void ProgramEditor::setPath(size_t programIdx) {
	this->programIdx = programIdx;
	setWindowTitle(tr("Edit program [%1]").arg(programIdx));
}

bool ProgramEditor::importTone() {
	auto metadata = tone::Metadata::fromOSB(program);
	bool success = tone::importDialog(program.tone, &metadata, getOutPath(curFile, true), this);
	if (success) {
		metadata.toOSB(program);
		loadProgramData();
	}
	return success;
}

bool ProgramEditor::exportTone() {
	QString tonePath;
	if (programIdx.has_value()) {
		tonePath = QString::number(*programIdx);
	}

	auto metadata = tone::Metadata::fromOSB(program);
	return tone::exportDialog(
		program.tone,
		&metadata,
		getOutPath(curFile, true),
		QFileInfo(curFile).baseName(),
		tonePath,
		this
	);
}

void ProgramEditor::convertToADPCM() {
	tone::convertToADPCM(program.tone, this);
	loadToneData();
}

void ProgramEditor::editUnknownProps() {
	ProgramMiscEditor editor(program, this);
	if (editor.exec() == QDialog::Accepted) {
		program = std::move(editor.program);
	}
}

void ProgramEditor::closeEvent(QCloseEvent* event) {
	saveSettings();
	event->accept();
}

void ProgramEditor::dragEnterEvent(QDragEnterEvent* event) {
	maybeDropEvent(event);
}

void ProgramEditor::dropEvent(QDropEvent* event) {
	const QString path = maybeDropEvent(event);
	if (!path.isEmpty()) {
		auto metadata = tone::Metadata::fromOSB(program);
		if (tone::importFile(program.tone, &metadata, path, this)) {
			metadata.toOSB(program);
			loadProgramData();
		}
	}
}

QString ProgramEditor::maybeDropEvent(QDropEvent* event) {
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

void ProgramEditor::restoreSettings() {
	if (!restoreGeometry(settings.value("ProgramEditor/Geometry").toByteArray())) {
		move(QApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
	}
}

void ProgramEditor::saveSettings() {
	settings.setValue("ProgramEditor/Geometry", saveGeometry());
}
