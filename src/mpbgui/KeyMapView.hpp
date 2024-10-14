#pragma once
#include <QWidget>
#include <manatools/mpb.hpp>
#include <guicommon/PianoKeyboardWidget.hpp>

class KeyMapView : public QWidget {
	Q_OBJECT
public:
	typedef manatools::mpb::Bank Bank;
	explicit KeyMapView(QWidget* parent = nullptr);

	QSize minimumSizeHint() const override;

	void setLayer(Bank* newBank, size_t newProgramIdx, size_t newLayerIdx) {
		bank = newBank;
		programIdx = newProgramIdx;
		layerIdx = newLayerIdx;
		repaint();
	}

	void setPiano(PianoKeyboardWidget* newPiano) {
		piano = newPiano;
		repaint();
	}

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	/**
	 * Because of how dumb the selection system is, passing Bank and indexes
	 * is more reliable, even though that irks me a bit.
	 * Just passing a Layer pointer is *much* more prone to being invalidated.
	 */
	Bank* bank;
	size_t programIdx;
	size_t layerIdx;

	PianoKeyboardWidget* piano;
};
