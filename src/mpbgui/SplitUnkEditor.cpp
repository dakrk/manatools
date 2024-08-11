#include "SplitUnkEditor.hpp"

SplitUnkEditor::SplitUnkEditor(QWidget* parent) :
	QDialog(parent)
{
	init();
}

SplitUnkEditor::SplitUnkEditor(const Split& split, QWidget* parent) :
	QDialog(parent),
	split_(split)
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
		label->setText(QString("0x%1").arg(value, 0, 16));
	});
}

void SplitUnkEditor::loadSplitData() {
	ui.spinUnk1->setValue(split_.unk1);
	ui.spinUnk2->setValue(split_.unk2);
	ui.spinUnk3->setValue(split_.unk3);
	ui.spinUnk4->setValue(split_.unk4);
}

void SplitUnkEditor::setSplitData() {
	split_.unk1 = ui.spinUnk1->value();
	split_.unk2 = ui.spinUnk2->value();
	split_.unk3 = ui.spinUnk3->value();
	split_.unk4 = ui.spinUnk4->value();
}
