#pragma once
#include <QStringList>
#include <QTextStream>
#include "common.hpp"

struct GUICOMMON_EXPORT CSV {
	void read(QTextStream& stream);
	void write(QTextStream& stream) const;

	QList<QStringList> rows;
};
