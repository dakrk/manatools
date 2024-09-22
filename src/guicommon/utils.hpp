#pragma once
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QPointF>
#include <QString>
#include <concepts>
#include <manatools/types.hpp>
#include "common.hpp"

GUICOMMON_EXPORT QString noteToString(u8 note);
GUICOMMON_EXPORT QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");
GUICOMMON_EXPORT bool insertItemRowHere(QAbstractItemView* view);
GUICOMMON_EXPORT bool removeItemRowHere(QAbstractItemView* view);

inline QString formatHex(std::unsigned_integral auto num, int width = 0) {
	return QString("0x%1").arg(num, width, 16, QChar('0'));
}

inline QString formatHex(std::signed_integral auto num, int width = 0) {
	return QString(num >= 0 ? "" : "-") % QString("0x%1").arg(std::abs(num), width, 16, QChar('0'));
}

inline constexpr QPointF mulPoint(QPointF point, qreal mulX, qreal mulY) {
	return { point.x() * mulX, point.y() * mulY };
}
