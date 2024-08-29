#include <QDir>
#include "utils.hpp"

QString getOutPath(const QString& curFile, bool dirOnly, const QString& newExtension) {
	if (curFile.isEmpty())
		return QDir::homePath();

	if (dirOnly)
		return QFileInfo(curFile).path();

	if (newExtension.isEmpty())
		return curFile;

	QFileInfo info(curFile);
	return info.dir().filePath(info.baseName() += '.' + newExtension);
}
