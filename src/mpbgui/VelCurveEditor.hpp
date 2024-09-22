#pragma once
#include <QDialog>
#include <manatools/mpb.hpp>

#include "ui_VelCurveEditor.h"
#include "VelCurvesModel.hpp"

class VelCurveEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::Velocity Velocity;

	explicit VelCurveEditor(QWidget* parent = nullptr);
	VelCurveEditor(const std::vector<Velocity>& velocities, QWidget* parent = nullptr);

	std::vector<Velocity> velocities;

private:
	void init();

	Ui::VelCurveEditor ui;
	VelCurvesModel* model;
};
