#include <guicommon/utils.hpp>
#include "SplitUnkEditor.hpp"

SplitUnkEditor::SplitUnkEditor(QWidget* parent) :
	QDialog(parent)
{
	init();
}

SplitUnkEditor::SplitUnkEditor(const Split& split, QWidget* parent) :
	QDialog(parent),
	split(split)
{
	init();
}

void SplitUnkEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());

	connect(this, &QDialog::accepted, this, &SplitUnkEditor::setSplitData);

	connectSpinnerHexLabel(ui.spinUnk1, ui.lblUnk1Hex);
	connectSpinnerHexLabel(ui.spinUnk2, ui.lblUnk2Hex);
	connectSpinnerHexLabel(ui.spinUnk3, ui.lblUnk3Hex);
	connectSpinnerHexLabel(ui.spinUnk4, ui.lblUnk4Hex);

	loadSplitData();
}

void SplitUnkEditor::connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label) {
	connect(spinner, &QSpinBox::valueChanged, this, [label](int value) {
		label->setText(formatHex(value));
	});
}

void SplitUnkEditor::loadSplitData() {
	ui.spinUnk1->setValue(split.unk1);
	ui.spinUnk2->setValue(split.unk2);
	ui.spinUnk3->setValue(split.unk3);
	ui.spinUnk4->setValue(split.unk4);

	ui.checkUnkFlag8->setChecked(split.unkFlags & (1 << 7));
	ui.checkUnkFlag7->setChecked(split.unkFlags & (1 << 6));
	ui.checkUnkFlag6->setChecked(split.unkFlags & (1 << 5));
	ui.checkUnkFlag5->setChecked(split.unkFlags & (1 << 4));
	ui.checkUnkFlag4->setChecked(split.unkFlags & (1 << 3));
	ui.checkUnkFlag3->setChecked(split.unkFlags & (1 << 2));
}

void SplitUnkEditor::setSplitData() {
	split.unk1 = ui.spinUnk1->value();
	split.unk2 = ui.spinUnk2->value();
	split.unk3 = ui.spinUnk3->value();
	split.unk4 = ui.spinUnk4->value();

	split.unkFlags = 0;
	split.unkFlags |= ui.checkUnkFlag8->isChecked() ? (1 << 7) : 0;
	split.unkFlags |= ui.checkUnkFlag7->isChecked() ? (1 << 6) : 0;
	split.unkFlags |= ui.checkUnkFlag6->isChecked() ? (1 << 5) : 0;
	split.unkFlags |= ui.checkUnkFlag5->isChecked() ? (1 << 4) : 0;
	split.unkFlags |= ui.checkUnkFlag4->isChecked() ? (1 << 3) : 0;
	split.unkFlags |= ui.checkUnkFlag3->isChecked() ? (1 << 2) : 0;
}
