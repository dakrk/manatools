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

	auto sizePolicy = ui.velCurve->sizePolicy();
	sizePolicy.setRetainSizeWhenHidden(true);
	ui.velCurve->setSizePolicy(sizePolicy);
	ui.velCurve->hide();

	connect(ui.btnCurveAdd, &QPushButton::clicked, this, [&]() {
		insertItemRowHere(ui.listCurves);
	});

	connect(ui.btnCurveDel, &QPushButton::clicked, this, [&]() {
		removeSelectedViewItems(ui.listCurves);
	});

	connect(ui.btnReset, &QPushButton::clicked, this, [&]() {
		ui.velCurve->setVelocity(Velocity::defaultCurve());
	});

	/**
	 * It would be much easier to make VelCurveWidget take a pointer, but for a widget like that
	 * it sorta makes sense for it to own one. Urgh.
	 */
	connect(ui.listCurves->selectionModel(), &QItemSelectionModel::currentChanged, this,
	        [&](const QModelIndex& current, const QModelIndex& previous)
	{
		if (previous.isValid()) {
			velocities[previous.row()] = ui.velCurve->velocity();
		}

		if (current.isValid()) {
			ui.velCurve->setVelocity(velocities[current.row()]);
			ui.velCurve->show();
		} else {
			ui.velCurve->hide();
		}
	});

	connect(this, &QDialog::accepted, this, [&]() {
		auto index = ui.listCurves->currentIndex();
		if (index.isValid()) {
			velocities[index.row()] = ui.velCurve->velocity();
		}
	});
}
