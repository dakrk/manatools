#include <guicommon/utils.hpp>
#include "SplitMiscEditor.hpp"

SplitMiscEditor::SplitMiscEditor(QWidget* parent) :
	QDialog(parent)
{
	init();
}

SplitMiscEditor::SplitMiscEditor(const Split& split, QWidget* parent) :
	QDialog(parent),
	split(split)
{
	init();
}

void SplitMiscEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());

	connect(this, &QDialog::accepted, this, &SplitMiscEditor::setSplitData);

	connectSpinnerHexLabel(ui.spinUnk1, ui.lblUnk1Hex);
	connectSpinnerHexLabel(ui.spinUnk2, ui.lblUnk2Hex);
	connectSpinnerHexLabel(ui.spinUnk3, ui.lblUnk3Hex);

	loadSplitData();
}

void SplitMiscEditor::connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label) {
	connect(spinner, &QSpinBox::valueChanged, this, [label](int value) {
		label->setText(formatHex(value));
	});
}

void SplitMiscEditor::loadSplitData() {
	ui.checkFlagKYONEX->setChecked(split.unkFlags & (1 << 7));
	ui.checkFlagKYONB->setChecked(split.unkFlags & (1 << 6));
	ui.checkFlagSSCTL->setChecked(split.unkFlags & (1 << 2));

	ui.checkEnvLPSLNK->setChecked(split.amp.LPSLNK);

	ui.spinPitchFNS->setValue(split.pitch.FNS);
	ui.spinPitchOCT->setValue(split.pitch.OCT);

	ui.spinUnk1->setValue(split.unk1);
	ui.spinUnk2->setValue(split.unk2);
	ui.spinUnk3->setValue(split.unk3);
}

void SplitMiscEditor::setSplitData() {
	split.unkFlags = 0;
	split.unkFlags |= ui.checkFlagKYONEX->isChecked() ? (1 << 7) : 0;
	split.unkFlags |= ui.checkFlagKYONB->isChecked() ? (1 << 6) : 0;
	split.unkFlags |= ui.checkFlagSSCTL->isChecked() ? (1 << 2) : 0;

	split.amp.LPSLNK = ui.checkEnvLPSLNK->isChecked();

	split.pitch.FNS = ui.spinPitchFNS->value();
	split.pitch.OCT = ui.spinPitchOCT->value();

	split.unk1 = ui.spinUnk1->value();
	split.unk2 = ui.spinUnk2->value();
	split.unk3 = ui.spinUnk3->value();
}
