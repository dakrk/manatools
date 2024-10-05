#pragma once
#include <QDialog>
#include <manatools/osb.hpp>

#include "ui_ProgramMiscEditor.h"

class ProgramMiscEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::osb::Program Program;

	explicit ProgramMiscEditor(QWidget* parent = nullptr);
	ProgramMiscEditor(const Program& program, QWidget* parent = nullptr);

	Program program;

private:
	void init();

	void connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label);

	void loadProgramData();
	void setProgramData();

	Ui::ProgramMiscEditor ui;
};
