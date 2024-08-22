#include <QPainter>
#include <QPainterPath>

#include "FilterEnvelopeWidget.hpp"
#include "utilities.hpp"

FilterEnvelopeWidget::FilterEnvelopeWidget(QWidget* parent) :
	QFrame(parent) {}

FilterEnvelopeWidget::FilterEnvelopeWidget(const FilterEnvelope& filterEnvelope, QWidget* parent) :
	QFrame(parent),
	filter(filterEnvelope) {}

void FilterEnvelopeWidget::mouseMoveEvent(QMouseEvent* event) {
	QFrame::mouseMoveEvent(event);
	// TODO: allow moving points around
}

void FilterEnvelopeWidget::paintEvent(QPaintEvent* event) {
	QFrame::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen(painter.pen());

	qreal stepX = width() / (31.0 * 4.0);
	qreal stepY = height() / 8184.0;

	QPointF pointStart   = {0, 8184.0 - filter.startLevel};
	QPointF pointAttack  = {pointStart.x()  + (31.0 - filter.attackRate),  8184.0 - filter.attackLevel};
	QPointF pointDecay1  = {pointAttack.x() + (31.0 - filter.decayRate1),  8184.0 - filter.decayLevel1};
	QPointF pointDecay2  = {pointDecay1.x() + (31.0 - filter.decayRate2),  8184.0 - filter.decayLevel2};
	QPointF pointRelease = {pointDecay2.x() + (31.0 - filter.releaseRate), 8184.0 - filter.releaseLevel};

	pointStart   = mulPoint(pointStart, stepX, stepY);
	pointAttack  = mulPoint(pointAttack, stepX, stepY);
	pointDecay1  = mulPoint(pointDecay1, stepX, stepY);
	pointDecay2  = mulPoint(pointDecay2, stepX, stepY);
	pointRelease = mulPoint(pointRelease, stepX, stepY);

	pen.setWidth(6); painter.setPen(pen);
	painter.drawPoint(pointStart);
	painter.drawPoint(pointAttack);
	painter.drawPoint(pointDecay1);
	painter.drawPoint(pointDecay2);
	painter.drawPoint(pointRelease);
	pen.setWidth(1); painter.setPen(pen);

	QPainterPath path;
	path.moveTo(pointStart);
	path.lineTo(pointAttack);
	path.lineTo(pointDecay1);
	path.lineTo(pointDecay2);
	path.lineTo(pointRelease);
	painter.drawPath(path);
}
