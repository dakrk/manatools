#include <QPainter>
#include "PianoKeyboardWidget.hpp"

PianoKeyboardWidget::PianoKeyboardWidget(QWidget* parent) :
	QWidget(parent),
	keyRangeLow_(0),
	keyRangeHigh_(NUM_KEYS),
	baseKey_(60) {}

void PianoKeyboardWidget::mouseReleaseEvent(QMouseEvent* event) {
	QWidget::mouseReleaseEvent(event);
	// TODO: not sure how to detect what key mouse is over and it's annoying me, ugh
}

// Ideally this should only paint the difference (event.rect/region)
void PianoKeyboardWidget::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	qreal whiteKeyW = static_cast<qreal>(width()) / NUM_WHITE_KEYS;
	qreal whiteKeyH = height();
	qreal blackKeyW = 0.60 * whiteKeyW;
	qreal blackKeyH = 0.65 * whiteKeyH;

	for (uint i = 0, white = 0; i < NUM_KEYS; i++) {
		if (KEY_TABLE[i % 12]) {
			QColor color = (i == baseKey_) ? BASE_KEY_COLOR : (i % 12 ? WHITE_KEY_COLOR : C_KEY_COLOR);
			QColor borderColor = KEY_BORDER_COLOR;
			if (i < keyRangeLow_ || i > keyRangeHigh_) {
				color = color.darker(250);
				borderColor = borderColor.darker(250);
			}

			qreal x = white++ * whiteKeyW;
			painter.fillRect(QRectF { x, 0, whiteKeyW, whiteKeyH }, color);

			x += whiteKeyW;
			painter.setPen(borderColor);
			painter.drawLine(QLineF { x, 0, x, whiteKeyH });
		}
	}

	for (uint i = 0; i < NUM_KEYS + 1; i++) {
		if (!KEY_TABLE[i % 12]) {
			QColor color = (i == baseKey_) ? BASE_KEY_COLOR : BLACK_KEY_COLOR;

			if (i < keyRangeLow_ || i > keyRangeHigh_) {
				color = color.darker(250);
			}

			// still not accurate to a real keyboard, but looks perfectly fine at a small scale
			uint whiteKey = (i / 12) * 7 + KEY_OFFSETS[i % 12];
			qreal x = ((whiteKey + 1) * whiteKeyW) - (blackKeyW / 2);
			
			painter.fillRect(QRectF { x, 0, blackKeyW, blackKeyH }, color);
		}
	}
}
