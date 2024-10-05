#include <guicommon/utils.hpp>
#include "ProgramMiscEditor.hpp"

ProgramMiscEditor::ProgramMiscEditor(QWidget* parent) :
	QDialog(parent)
{
	init();
}

ProgramMiscEditor::ProgramMiscEditor(const Program& program, QWidget* parent) :
	QDialog(parent),
	program(program)
{
	init();
}

void ProgramMiscEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());

	connect(this, &QDialog::accepted, this, &ProgramMiscEditor::setProgramData);

	connectSpinnerHexLabel(ui.spinUnk1, ui.lblUnk1Hex);

	loadProgramData();
}

void ProgramMiscEditor::connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label) {
	connect(spinner, &QSpinBox::valueChanged, this, [label](int value) {
		label->setText(formatHex(value));
	});
}

void ProgramMiscEditor::loadProgramData() {
	ui.checkFlagKYONEX->setChecked(program.unkFlags & (1 << 7));
	ui.checkFlagKYONB->setChecked(program.unkFlags & (1 << 6));
	ui.checkFlagSSCTL->setChecked(program.unkFlags & (1 << 2));

	ui.checkEnvLPSLNK->setChecked(program.amp.LPSLNK);

	ui.spinPitchFNS->setValue(program.pitch.FNS);
	ui.spinPitchOCT->setValue(program.pitch.OCT);

	ui.spinUnk1->setValue(program.unk1);
}

void ProgramMiscEditor::setProgramData() {
	program.unkFlags = 0;
	program.unkFlags |= ui.checkFlagKYONEX->isChecked() ? (1 << 7) : 0;
	program.unkFlags |= ui.checkFlagKYONB->isChecked() ? (1 << 6) : 0;
	program.unkFlags |= ui.checkFlagSSCTL->isChecked() ? (1 << 2) : 0;

	program.amp.LPSLNK = ui.checkEnvLPSLNK->isChecked();

	program.pitch.FNS = ui.spinPitchFNS->value();
	program.pitch.OCT = ui.spinPitchOCT->value();

	program.unk1 = ui.spinUnk1->value();
}
