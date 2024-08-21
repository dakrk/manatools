#include <QPainter>
#include <QPainterPath>

#include "AmpEnvelopeWidget.hpp"
#include "utilities.hpp"

AmpEnvelopeWidget::AmpEnvelopeWidget(QWidget* parent) :
	QFrame(parent) {}

AmpEnvelopeWidget::AmpEnvelopeWidget(const AmpEnvelope& ampEnvelope, QWidget* parent) :
	QFrame(parent),
	amp(ampEnvelope) {}

void AmpEnvelopeWidget::mouseMoveEvent(QMouseEvent* event) {
	// TODO: allow moving points around
}

void AmpEnvelopeWidget::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen(painter.pen());

	double stepX = width() / (31.0 * 3.0);
	double stepY = height() / 31.0;
	qreal decayLevel = amp.decayLevel;

	QPointF pointAttackRate  = {(31.0 * 1) - amp.attackRate, 0};
	QPointF pointDecayRate1  = {(31.0 * 2) - ((amp.decayRate1 / 31.0) * (31.0 + amp.attackRate)), decayLevel};
	QPointF pointDecayRate2  = {(31.0 * 2), (decayLevel + ((amp.decayRate2 / 31.0) * (31.0 - decayLevel)))};
	QPointF pointReleaseRate = {(31.0 * 3) - amp.releaseRate, 31.0};

	pointAttackRate  = mulPoint(pointAttackRate, stepX, stepY);
	pointDecayRate1  = mulPoint(pointDecayRate1, stepX, stepY);
	pointDecayRate2  = mulPoint(pointDecayRate2, stepX, stepY);
	pointReleaseRate = mulPoint(pointReleaseRate, stepX, stepY);

	pen.setWidth(6); painter.setPen(pen);
	painter.drawPoint(pointAttackRate);
	painter.drawPoint(pointDecayRate1);
	painter.drawPoint(pointDecayRate2);
	painter.drawPoint(pointReleaseRate);
	pen.setWidth(1); painter.setPen(pen);

	QPainterPath path;
	path.moveTo(0, 31.0 * stepY);
	path.lineTo(pointAttackRate);
	path.lineTo(pointDecayRate1);
	path.lineTo(pointDecayRate2);
	path.lineTo(pointReleaseRate);
	painter.drawPath(path);
}
