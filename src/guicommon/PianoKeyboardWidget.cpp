#include <QPainter>
#include "PianoKeyboardWidget.hpp"

QSize PianoKeyboardWidget::minimumSizeHint() const {
	return { 730, 40 };
}

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

	for (int i = 0, white = 0; i < NUM_KEYS; i++) {
		if (keyIsWhite(i)) {
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

	for (int i = 0; i < NUM_KEYS + 1; i++) {
		if (!keyIsWhite(i)) {
			QColor color = (i == baseKey_) ? BASE_KEY_COLOR : BLACK_KEY_COLOR;

			if (i < keyRangeLow_ || i > keyRangeHigh_) {
				color = color.darker(250);
			} else if (i == keyRangeLow_ || i == keyRangeHigh_) {
				// Impossible to see where a range starts/ends on a black key otherwise
				float h, s, v;
				color.getHsvF(&h, &s, &v);
				color.setHsvF(h, s, 0.15);
			}

			// still not accurate to a real keyboard, but looks perfectly fine at a small scale
			int whiteKey = (i / 12) * 7 + KEY_OFFSETS[i % 12];
			qreal x = ((whiteKey + 1) * whiteKeyW) - (blackKeyW / 2);
			
			painter.fillRect(QRectF { x, 0, blackKeyW, blackKeyH }, color);
		}
	}
}

bool PianoKeyboardWidget::keyIsWhite(int key) const {
	return KEY_TABLE[key % 12];
}

qreal PianoKeyboardWidget::keyTypeWidth(bool white) const {
	qreal w = static_cast<qreal>(width()) / NUM_WHITE_KEYS;
	return white ? w : 0.60 * w;
}

qreal PianoKeyboardWidget::keyTypeHeight(bool white) const {
	return white ? height() : 0.65 * height();
}

qreal PianoKeyboardWidget::keyX(int key) const {
	int whiteKey = ((key / 12) * 7 + KEY_OFFSETS[key % 12]);
	if (keyIsWhite(key)) {
		return keyTypeWidth(true) * whiteKey;
	} else {
		return ((whiteKey + 1) * keyTypeWidth(true)) - (keyTypeWidth(false) / 2);
	}
}
