#include <guicommon/utils.hpp>
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

	ui.listCurves->setModel(new VelCurvesModel(velocities, this));

	if (velocities.empty()) {
		ui.velCurve->hide();
	}

	connect(ui.btnCurveAdd, &QPushButton::clicked, this, [&]() {
		insertItemRowHere(ui.listCurves);
	});

	connect(ui.btnCurveDel, &QPushButton::clicked, this, [&]() {
		removeItemRowHere(ui.listCurves);
	});

	/**
	 * It would be much easier to make VelCurveWidget take a pointer, but for a widget like
	 * that it sorta makes sense for it to own one. Urgh.
	 */
	connect(ui.listCurves->selectionModel(), &QItemSelectionModel::currentChanged, this,
	        [&](const QModelIndex &current, const QModelIndex &previous)
	{
		if (previous.isValid()) {
			velocities[previous.row()] = ui.velCurve->velocity();
		}

		if (current.isValid()) {
			ui.velCurve->setVelocity(velocities[current.row()]);
			ui.velCurve->show();
		}
	});

	//connect(this, &QDialog::accepted, this, &VelCurveEditor::something);
}
