#pragma once
#include <QDialog>
#include <manatools/mpb.hpp>

#include "ui_SplitUnkEditor.h"

class SplitUnkEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::Split Split;

	explicit SplitUnkEditor(QWidget* parent = nullptr);
	SplitUnkEditor(const Split& split, QWidget* parent = nullptr);

	Split split;

private:
	void init();

	void connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label);

	void loadSplitData();
	void setSplitData();

	Ui::SplitUnkEditor ui;
};
