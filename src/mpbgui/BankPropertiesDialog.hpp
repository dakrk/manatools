#pragma once
#include <QDialog>
#include <manatools/mpb.hpp>

#include "ui_BankPropertiesDialog.h"
#include "UIntValidator.hpp"

class BankPropertiesDialog : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::Bank Bank;
	BankPropertiesDialog(Bank* split, QWidget* parent = nullptr);

private:
	void loadData();
	void setData();

	UIntValidator versionValidator;

	Ui::BankPropertiesDialog ui;

	Bank* bank;
};
