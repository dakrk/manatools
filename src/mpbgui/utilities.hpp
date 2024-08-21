#pragma once
#include <QString>
#include <QPointF>
#include <manatools/types.hpp>

QString noteToString(u8 note);
QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");

inline constexpr QPointF mulPoint(QPointF point, qreal mulX, qreal mulY) {
	return {point.x() * mulX, point.y() * mulY};
}
