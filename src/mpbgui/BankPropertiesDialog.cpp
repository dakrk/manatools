#include <guicommon/utils.hpp>
#include "BankPropertiesDialog.hpp"

BankPropertiesDialog::BankPropertiesDialog(Bank* bank, QWidget* parent) :
	QDialog(parent),
	versionValidator(0, std::numeric_limits<u32>::max(), this),
	bank(bank)
{
	ui.setupUi(this);
	setFixedSize(size());

	ui.comboType->addItem(tr("MIDI Program Bank (MPB)"), false);
	ui.comboType->addItem(tr("MIDI Drum Bank (MDB)"), true);

	ui.comboVersion->addItem("1");
	ui.comboVersion->addItem("2");
	ui.comboVersion->setValidator(&versionValidator);

	connect(this, &QDialog::accepted, this, &BankPropertiesDialog::setData);

	loadData();
}

void BankPropertiesDialog::loadData() {
	int typeIndex = ui.comboType->findData(bank->drum);
	assert(typeIndex != -1);
	ui.comboType->setCurrentIndex(typeIndex);

	ui.comboVersion->setCurrentText(formatHex(bank->version));
	ui.lblTotalProgramsVal->setText(QString::number(bank->programs.size()));

	size_t totalSplits = 0;
	for (const auto& program : bank->programs) {
		for (const auto& layer : program.layers) {
			if (!layer.has_value())
				continue;

			totalSplits += layer->splits.size();
		}
	}

	ui.lblTotalSplitsVal->setText(QString::number(totalSplits));
	ui.lblTotalVelocitiesVal->setText(QString::number(bank->velocities.size()));

	// TODO: Show and validate checksum data
}

void BankPropertiesDialog::setData() {
	bank->drum = ui.comboType->currentData().toBool();
	bank->version = ui.comboVersion->currentText().toUInt(nullptr, 16);
}
