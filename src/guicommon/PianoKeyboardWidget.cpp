#include <QPainter>
#include "PianoKeyboardWidget.hpp"

PianoKeyboardWidget::PianoKeyboardWidget(QWidget* parent)
	: QWidget(parent) {}

void PianoKeyboardWidget::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	qreal whiteKeyW = static_cast<qreal>(width()) / NUM_WHITE_KEYS;
	qreal whiteKeyH = height();
	qreal blackKeyW = 0.60 * whiteKeyW;
	qreal blackKeyH = 0.65 * whiteKeyH;

	for (uint i = 0; i < NUM_WHITE_KEYS; i++) {
		qreal x = i * whiteKeyW;
		painter.fillRect(QRectF { x, 0, whiteKeyW, whiteKeyH }, i % 7 ? Qt::white : Qt::gray);

		x += whiteKeyW;
		painter.setPen(Qt::darkGray);
		painter.drawLine(x, 0, x, whiteKeyH);
	}

	for (uint i = 0; i < NUM_WHITE_KEYS; i++) {
		if (i % 7 != 2 && i % 7 != 6) {
			painter.fillRect(QRectF { (i * whiteKeyW) + blackKeyW, 0, blackKeyW, blackKeyH }, Qt::black);
		}
	}
}
