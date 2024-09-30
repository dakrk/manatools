#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <manatools/types.hpp>
#include "common.hpp"
#include "tone.hpp"

class GUICOMMON_EXPORT InstDataDialog : public QDialog {
	Q_OBJECT
public:
	explicit InstDataDialog(tone::Metadata current, tone::Metadata imported, QWidget* parent = nullptr);

private:
	QVBoxLayout* mainLayout;
	QLabel* text;
	QTableWidget* table;
	QDialogButtonBox* buttons;
};
