#include <QPainter>
#include <QPalette>
#include "KeyMapView.hpp"

KeyMapView::KeyMapView(QWidget* parent) :
	QWidget(parent),
	bank(nullptr),
	programIdx(0),
	layerIdx(0),
	splitIdx(0),
	piano(nullptr) {}

QSize KeyMapView::minimumSizeHint() const {
	return { 512, 128 };
}

void KeyMapView::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	qreal widthStep = width() / 127.0;
	qreal heightStep = height() / 127.0;
	qreal middleY = height() / 2.0;

	const auto oldPen = painter.pen();
	painter.setPen(palette().dark().color());
	for (int i = 0; i < 128; i++) {
		qreal x = i * widthStep;
		painter.setOpacity(i % 12 ? 0.25 : 0.75);
		painter.drawLine(QLineF(x, 0, x, height()));
	}
	painter.setOpacity(0.75);
	painter.drawLine(QLineF(0, middleY, width(), middleY));
	painter.setOpacity(1.0);
	painter.setPen(oldPen);

	if (!bank)
		return;

	const auto* layer = bank->layer(programIdx, layerIdx);

	if (!layer)
		return;

	QColor rectColor = palette().midlight().color();
	rectColor.setAlphaF(0.4);
	QBrush rectBrush(rectColor);

	rectColor.setAlphaF(0.75);
	QBrush curRectBrush(rectColor);

	QColor borderColor = palette().dark().color();
	QPen borderPen(borderColor);

	for (size_t s = 0; s < layer->splits.size(); s++) {
		const auto& split = layer->splits[s];

		QPointF tl(piano->keyX(split.startNote), split.velocityLow);
		QPointF br(piano->keyX(split.endNote) + piano->keyWidth(split.endNote), split.velocityHigh);

		tl.ry() *= heightStep;
		br.ry() *= heightStep;

		QRectF rect(tl, br);

		const auto oldPen = painter.pen();
		painter.setPen(borderPen);
		painter.fillRect(rect, s == splitIdx ? curRectBrush : rectBrush);
		painter.drawRect(rect);
		painter.setPen(oldPen);

		painter.drawText(rect, Qt::AlignCenter, QString::number(s));		
	}
}
