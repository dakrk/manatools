#include <QHeaderView>
#include <QPainter>
#include <QTableView>
#include "HorizontalLineItemDropStyle.hpp"

void HorizontalLineItemDropStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
	if (element == QStyle::PE_IndicatorItemViewItemDrop) {
		const QTableView* table = qobject_cast<const QTableView*>(widget);

		// TODO: See if Qt makes it even more frustrating to draw for the very last row
		if (!option->rect.isNull() && table) {
			int top = option->rect.top();
			int width = table->horizontalHeader()->length();
			painter->drawLine(QPoint(0, top), QPoint(width, top));
		}
	} else {
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
}
