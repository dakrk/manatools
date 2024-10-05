#pragma once
#include <QDialog>
#include <manatools/mpb.hpp>

#include "ui_SplitMiscEditor.h"

/**
 * Shoving stuff in a separate window isn't particularly something I really
 * want to do, but I think the current SplitEditor window is really quite good
 * and trying to shove something like tabs in would ruin it. I dunno...
 */
class SplitMiscEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::Split Split;

	explicit SplitMiscEditor(QWidget* parent = nullptr);
	SplitMiscEditor(const Split& split, QWidget* parent = nullptr);

	Split split;

private:
	void init();

	void connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label);

	void loadSplitData();
	void setSplitData();

	Ui::SplitMiscEditor ui;
};
