#pragma once
#include <QString>
#include <manatools/types.hpp>

QString getOutPath(const QString& curFile, bool dirOnly = false, const QString& newExtension = "");
QString formatPtr32(u32 ptr);
