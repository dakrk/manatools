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

void removeSelectedViewItems(QAbstractItemView* view) {
	const auto selRows = view->selectionModel()->selectedRows();
	auto* model = view->model();

	if (selRows.size() > 1) {
		/**
		 * Need to store persistent indexes instead, as removing rows will make row
		 * numbers of the normal indexes invalid
		 */
		QList<QPersistentModelIndex> indexes;

		for (const auto& idx : selRows) {
			indexes.append(idx);
		}

		for (const auto& idx : indexes) {
			model->removeRow(idx.row());
		}
	} else if (selRows.size() > 0) {
		/**
		 * With ExtendedSelection, Qt doesn't keep a selection after a removal, grr.
		 * Now we have to do this ourselves...
		 */
		const auto& idx = selRows[0];
		if (model->removeRow(idx.row())) {
			auto row = idx.row();
			if (row >= model->rowCount())
				row--;

			view->setCurrentIndex(model->index(row, idx.column()));
		}
	}
}
