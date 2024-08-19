#include <QDir>
#include <manatools/note.hpp>
#include "utilities.hpp"

QString noteToString(u8 note) {
	return QString("%1%2").arg(manatools::noteName(note)).arg(manatools::noteOctave(note));
}

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
