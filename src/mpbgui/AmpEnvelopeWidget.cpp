#include <QPainter>
#include <QPainterPath>

#include "AmpEnvelopeWidget.hpp"
#include "utilities.hpp"

AmpEnvelopeWidget::AmpEnvelopeWidget(QWidget* parent) :
	QFrame(parent)
{
	init();
}

AmpEnvelopeWidget::AmpEnvelopeWidget(const AmpEnvelope& ampEnvelope, QWidget* parent) :
	QFrame(parent),
	amp(ampEnvelope)
{
	init();
}

void AmpEnvelopeWidget::init() {
	setFrameShape(Panel);
	setFrameShadow(Sunken);
	setStyleSheet("background-color: rgba(0, 0, 0, 48);");
}

void AmpEnvelopeWidget::mouseMoveEvent(QMouseEvent* event) {
	QFrame::mouseMoveEvent(event);
	// TODO: allow moving points around
}

void AmpEnvelopeWidget::paintEvent(QPaintEvent* event) {
	QFrame::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen(painter.pen());

	qreal stepX = width() / (31.0 * 3.0);
	qreal stepY = height() / 31.0;
	qreal decayLevel = amp.decayLevel;

	QPointF pointAttack  = {(31.0 * 1) - amp.attackRate, 0};
	QPointF pointDecay1  = {(31.0 * 2) - ((amp.decayRate1 / 31.0) * (31.0 + amp.attackRate)), decayLevel};
	QPointF pointDecay2  = {(31.0 * 2), (decayLevel + ((amp.decayRate2 / 31.0) * (31.0 - decayLevel)))};
	QPointF pointRelease = {(31.0 * 3) - amp.releaseRate, 31.0};

	pointAttack  = mulPoint(pointAttack, stepX, stepY);
	pointDecay1  = mulPoint(pointDecay1, stepX, stepY);
	pointDecay2  = mulPoint(pointDecay2, stepX, stepY);
	pointRelease = mulPoint(pointRelease, stepX, stepY);

	pen.setWidth(6); painter.setPen(pen);
	painter.drawPoint(pointAttack);
	painter.drawPoint(pointDecay1);
	painter.drawPoint(pointDecay2);
	painter.drawPoint(pointRelease);
	pen.setWidth(1); painter.setPen(pen);

	QPainterPath path;
	path.moveTo(0, 31.0 * stepY);
	path.lineTo(pointAttack);
	path.lineTo(pointDecay1);
	path.lineTo(pointDecay2);
	path.lineTo(pointRelease);
	painter.drawPath(path);
}
