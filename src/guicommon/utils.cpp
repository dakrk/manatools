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

bool insertItemRowHere(QAbstractItemView* view) {
	QModelIndex cur = view->currentIndex();
	auto* model = view->model();
	int row, col;

	if (cur.isValid()) {
		row = cur.row() + 1;
		col = cur.column();
	} else {
		row = 0;
		col = 0;
	}

	if (model->insertRow(row)) {
		view->setCurrentIndex(model->index(row, col));
		return true;
	}
	
	return false;
}

bool removeItemRowHere(QAbstractItemView* view) {
	QModelIndex cur = view->currentIndex();
	auto* model = view->model();

	if (!cur.isValid())
		return false;

	int curRow = cur.row();

	/**
	 * JANK MODE: ACTIVATE
	 * fuck you Qt. why can't you fire the right damn index in currentChanged that isn't
	 * off by one when I remove from in the middle of a table without me having to move back,
	 * remove, then move back forward again just to account for this shit.
	 */
	bool wasBlocked = view->signalsBlocked();
	view->blockSignals(true);
	view->setCurrentIndex(model->index(curRow - 1, cur.column()));
	view->blockSignals(wasBlocked);

	if (model->removeRow(curRow)) {
		if (curRow >= model->rowCount())
			curRow--;

		view->setCurrentIndex(model->index(curRow, cur.column()));
		return true;
	}

	return false;
}
