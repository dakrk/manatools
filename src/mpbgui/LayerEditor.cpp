#include "LayerEditor.hpp"

LayerEditor::LayerEditor(Layer* layer, QWidget* parent) :
	QDialog(parent),
	layer(layer)
{
	ui.setupUi(this);
	setFixedSize(size());

	connect(this, &QDialog::accepted, this, &LayerEditor::setData);

	connectSpinnerHexLabel(ui.spinUnk1, ui.lblUnk1Hex);
	connectSpinnerHexLabel(ui.spinUnk2, ui.lblUnk2Hex);

	loadData();
}

void LayerEditor::setPath(size_t programIdx, size_t layerIdx) {
	setWindowTitle(
		QString("%1 [%2:%3]")
			.arg(tr("Edit layer"))
			.arg(programIdx + 1)
			.arg(layerIdx + 1)
	);
}

void LayerEditor::connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label) {
	connect(spinner, &QSpinBox::valueChanged, this, [label](int value) {
		label->setText(QString("0x%1").arg(value, 0, 16));
	});
}

void LayerEditor::loadData() {
	ui.spinDelay->setValue(layer->delay);
	ui.spinUnk1->setValue(layer->unk1);
	ui.spinBendHigh->setValue(layer->bendRangeHigh);
	ui.spinBendLow->setValue(layer->bendRangeLow);
	ui.spinUnk2->setValue(layer->unk2);
}

void LayerEditor::setData() {
	layer->delay = ui.spinDelay->value();
	layer->unk1 = ui.spinUnk1->value();
	layer->bendRangeHigh = ui.spinBendHigh->value();
	layer->bendRangeLow = ui.spinBendLow->value();
	layer->unk2 = ui.spinUnk2->value();
}
