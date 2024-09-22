#include "VelCurveEditor.hpp"

VelCurveEditor::VelCurveEditor(QWidget* parent) :
	QDialog(parent)
{
	init();
}

VelCurveEditor::VelCurveEditor(const std::vector<Velocity>& velocities, QWidget* parent) :
	QDialog(parent),
	velocities(velocities)
{
	init();
}

void VelCurveEditor::init() {
	ui.setupUi(this);
	setFixedSize(size());

	model = new VelCurvesModel(velocities, this);
	ui.listCurves->setModel(model);

	//connect(this, &QDialog::accepted, this, &VelCurveEditor::something);
}
