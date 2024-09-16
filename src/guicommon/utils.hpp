#pragma once
#include <concepts>
#include <QPointF>
#include <QString>
#include <manatools/types.hpp>
#include "common.hpp"

GUICOMMON_EXPORT QString noteToString(u8 note);
GUICOMMON_EXPORT QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");

inline QString formatHex(std::unsigned_integral auto num, int width = 0) {
	return QString("0x%1").arg(num, width, 16, QChar('0'));
}

inline QString formatHex(std::signed_integral auto num, int width = 0) {
	return QString(num >= 0 ? "" : "-") % QString("0x%1").arg(std::abs(num), width, 16, QChar('0'));
}

inline constexpr QPointF mulPoint(QPointF point, qreal mulX, qreal mulY) {
	return { point.x() * mulX, point.y() * mulY };
}
