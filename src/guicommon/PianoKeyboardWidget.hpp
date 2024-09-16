#pragma once
#include <QWidget>
#include "common.hpp"

class GUICOMMON_EXPORT PianoKeyboardWidget : public QWidget {
	Q_OBJECT
public:
	static constexpr uint NUM_KEYS = 128;
	static constexpr uint NUM_WHITE_KEYS = 75;

	explicit PianoKeyboardWidget(QWidget* parent = nullptr);

protected:
	void paintEvent(QPaintEvent* event) override;
};
