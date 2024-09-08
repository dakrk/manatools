#pragma once
#include <QPointF>
#include <QString>
#include <manatools/types.hpp>
#include "common.hpp"

GUICOMMON_EXPORT QString noteToString(u8 note);
GUICOMMON_EXPORT QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");
GUICOMMON_EXPORT QString formatHex(uint num, int width = 8);

inline constexpr QPointF mulPoint(QPointF point, qreal mulX, qreal mulY) {
	return {point.x() * mulX, point.y() * mulY};
}
