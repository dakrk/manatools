#pragma once
#include <QFrame>
#include <manatools/mpb.hpp>

class AmpEnvelopeWidget : public QFrame {
	Q_OBJECT
public:
	typedef manatools::mpb::AmpEnvelope AmpEnvelope;

	explicit AmpEnvelopeWidget(QWidget* parent = nullptr);
	AmpEnvelopeWidget(const AmpEnvelope& ampEnvelope, QWidget* parent = nullptr);

	AmpEnvelope amp;

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
};
