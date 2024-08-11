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

	Split& split() {
		return split_;
	}

	const Split& split() const {
		return split_;
	}

private:
	void init();

	void connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label);

	void loadSplitData();
	void setSplitData();

	Ui::SplitUnkEditor ui;

	Split split_;
};
