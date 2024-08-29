#pragma once
#include <QFrame>
#include <manatools/common.hpp>

class FilterEnvelopeWidget : public QFrame {
	Q_OBJECT
public:
	typedef manatools::common::FilterEnvelope FilterEnvelope;

	explicit FilterEnvelopeWidget(QWidget* parent = nullptr);
	FilterEnvelopeWidget(const FilterEnvelope& filterEnvelope, QWidget* parent = nullptr);

	FilterEnvelope filter;

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	void init();
};
