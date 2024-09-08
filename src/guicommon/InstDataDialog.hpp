#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <manatools/types.hpp>
#include "common.hpp"

class GUICOMMON_EXPORT InstDataDialog : public QDialog {
	Q_OBJECT
public:
	struct Data {
		u8 startNote;
		u8 endNote;
		u8 baseNote;
		u8 startVel;
		u8 endVel;
	};

	explicit InstDataDialog(Data current, Data imported, QWidget* parent = nullptr);

private:
	QVBoxLayout* mainLayout;
	QLabel* text;
	QTableWidget* table;
	QDialogButtonBox* buttons;
};
