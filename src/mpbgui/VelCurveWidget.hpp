#pragma once
#include <QFrame>
#include <manatools/mpb.hpp>

class VelCurveWidget : public QFrame {
	Q_OBJECT
public:
	typedef manatools::mpb::Velocity Velocity;

	explicit VelCurveWidget(QWidget* parent = nullptr);
	VelCurveWidget(const Velocity& velocity, QWidget* parent = nullptr);

	void setVelocity(const Velocity& curve) {
		vel = curve;
		update();
	}

	Velocity& velocity() {
		return vel;
	}

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	Velocity vel;
};
