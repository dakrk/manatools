#pragma once
#include <QWidget>
#include "common.hpp"

class GUICOMMON_EXPORT PianoKeyboardWidget : public QWidget {
	Q_OBJECT
public:
	static constexpr int  NUM_KEYS = 128;
	static constexpr int  NUM_WHITE_KEYS = 75;
	static constexpr bool KEY_TABLE[] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
	static constexpr int  KEY_OFFSETS[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };

	static constexpr auto WHITE_KEY_COLOR = Qt::white;
	static constexpr auto BLACK_KEY_COLOR = Qt::black;
	static constexpr auto BASE_KEY_COLOR = Qt::red;
	static constexpr auto C_KEY_COLOR = Qt::gray;
	static constexpr auto KEY_BORDER_COLOR = Qt::darkGray;

	explicit PianoKeyboardWidget(QWidget* parent = nullptr) :
		QWidget(parent),
		keyRangeLow_(0),
		keyRangeHigh_(NUM_KEYS),
		baseKey_(60) {}

	PianoKeyboardWidget(int low, int high, int base, QWidget* parent = nullptr) :
		QWidget(parent),
		keyRangeLow_(low),
		keyRangeHigh_(high),
		baseKey_(base) {}

	PianoKeyboardWidget(QPair<int, int> range, int base, QWidget* parent = nullptr) :
		QWidget(parent),
		keyRangeLow_(range.first),
		keyRangeHigh_(range.second),
		baseKey_(base) {}

	QPair<int, int> keyRange() const {
		return { keyRangeLow_, keyRangeHigh_ };
	}

	void setKeyRange(int low, int high) {
		keyRangeLow_ = low;
		keyRangeHigh_ = high;
		repaint();
	}

	void setKeyRange(QPair<int, int> range) {
		setKeyRange(range.first, range.second);
	}

	int baseKey() const {
		return baseKey_;
	}

	void setBaseKey(int key) {
		baseKey_ = key;
		repaint();
	}

protected:
	void mouseReleaseEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	int keyRangeLow_;
	int keyRangeHigh_;
	int baseKey_;
};
