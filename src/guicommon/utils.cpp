#include <QDir>
#include <manatools/note.hpp>
#include "utils.hpp"

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

QString formatPtr32(u32 ptr) {
	return QString("0x%1").arg(ptr, 8, 16, QChar('0'));
}
