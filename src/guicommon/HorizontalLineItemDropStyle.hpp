#pragma once
#include <QProxyStyle>
#include "common.hpp"

// what a mouthful
class GUICOMMON_EXPORT HorizontalLineItemDropStyle : public QProxyStyle {
	Q_OBJECT
public:
	explicit HorizontalLineItemDropStyle(QStyle* style = nullptr) : QProxyStyle(style) {};
	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
};
