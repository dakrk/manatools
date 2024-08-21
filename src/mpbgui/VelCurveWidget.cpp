#include <QPainter>
#include <QPainterPath>

#include "VelCurveWidget.hpp"

VelCurveWidget::VelCurveWidget(QWidget* parent) :
	QFrame(parent) {}

VelCurveWidget::VelCurveWidget(const Velocity& velocity, QWidget* parent) :
	QFrame(parent),
	vel(velocity) {}

void VelCurveWidget::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// flip y-axis
	painter.translate(0, height());
	painter.scale(1, -1);

	size_t numPoints = std::size(vel.data);
	double stepX = width() / (double)numPoints;
	double stepY = height() / 128.;

	QPainterPath path;
	path.moveTo(0, vel.data[0] * stepY);

	/**
	 * TODO: I suck at maths, how the hell would I make curves with this smooth?
	 * Not using lineTo would be a step forward but I don't know how to do anything else
	 */
	for (size_t i = 1; i < numPoints; i++) {
		path.lineTo(i * stepX, vel.data[i] * stepY);
	}

	painter.drawPath(path);
}
