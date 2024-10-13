#include <QPainter>
#include "KeyMapView.hpp"

KeyMapView::KeyMapView(QWidget* parent) :
	QWidget(parent),
	bank(nullptr),
	programIdx(0),
	layerIdx(0),
	piano(nullptr) {}

void KeyMapView::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	if (!bank)
		return;

	const auto* layer = bank->layer(programIdx, layerIdx);

	if (!layer)
		return;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	qreal heightStep = height() / 127.0;

	for (size_t s = 0; s < layer->splits.size(); s++) {
		const auto& split = layer->splits[s];

		QPointF tl(piano->keyX(split.startNote), split.velocityLow);
		QPointF br(piano->keyX(split.endNote) + piano->keyWidth(split.endNote), split.velocityHigh);

		tl.ry() *= heightStep;
		br.ry() *= heightStep;

		QRectF rect(tl, br);

		painter.fillRect(rect, Qt::black);
		painter.drawRect(rect);

		painter.drawText(rect, Qt::AlignCenter, QString::number(s));		
	}
}
