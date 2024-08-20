#pragma once
#include <QFrame>
#include <manatools/mpb.hpp>

class VelCurveWidget : public QFrame {
	Q_OBJECT
public:
	typedef manatools::mpb::Velocity Velocity;

	explicit VelCurveWidget(QWidget* parent = nullptr);
	VelCurveWidget(const Velocity& velocity, QWidget* parent = nullptr);

	Velocity vel;

protected:
	void paintEvent(QPaintEvent* event);
};
