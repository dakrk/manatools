#pragma once
#include <QString>
#include <QFileInfo>
#include "manatools/types.hpp"

QString noteToString(u8 note);
QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");
