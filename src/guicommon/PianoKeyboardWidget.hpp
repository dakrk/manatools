#pragma once
#include <QWidget>
#include "common.hpp"

class GUICOMMON_EXPORT PianoKeyboardWidget : public QWidget {
	Q_OBJECT
public:
	static constexpr uint NUM_KEYS = 128;
	static constexpr uint NUM_WHITE_KEYS = 75;
	static constexpr bool KEY_TABLE[] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
	static constexpr uint KEY_OFFSETS[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };

	static constexpr auto WHITE_KEY_COLOR = Qt::white;
	static constexpr auto BLACK_KEY_COLOR = Qt::black;
	static constexpr auto BASE_KEY_COLOR = Qt::red;
	static constexpr auto C_KEY_COLOR = Qt::gray;
	static constexpr auto KEY_BORDER_COLOR = Qt::darkGray;

	explicit PianoKeyboardWidget(QWidget* parent = nullptr);

	QPair<uint, uint> keyRange() const {
		return { keyRangeLow_, keyRangeHigh_ };
	}

	void setKeyRange(uint low, uint high) {
		keyRangeLow_ = low;
		keyRangeHigh_ = high;
		repaint();
	}

	void setKeyRange(QPair<uint, uint> range) {
		setKeyRange(range.first, range.second);
	}

	uint baseKey() const {
		return baseKey_;
	}

	void setBaseKey(uint key) {
		baseKey_ = key;
		repaint();
	}

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	uint keyRangeLow_;
	uint keyRangeHigh_;
	uint baseKey_;
};
