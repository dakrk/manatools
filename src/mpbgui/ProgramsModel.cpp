#include <QIODevice>
#include <QMimeData>
#include <optional>
#include "ProgramsModel.hpp"

const QString ProgramsModel::MIMEType = QStringLiteral("application/x-manatools-mpbgui-programsmodel");

ProgramsModel::ProgramsModel(Bank* bank, QObject* parent) :
	QAbstractTableModel(parent),
	bank(bank) {}

int ProgramsModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : bank->programs.size();
}

int ProgramsModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : 3;
}

/**
 * Qt seems to call this rather excessively, which is a bit annoying when paired with
 * repeatedly recalculating layer/split counts
 */
QVariant ProgramsModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case 0: {
				return tr("Program %1").arg(index.row() + 1);
			}

			case 1: {
				return bank->programs[index.row()].usedLayers();
			}

			case 2: {
				uint splits = 0;
				for (const auto& layer : bank->programs[index.row()].layers) {
					if (!layer)
						continue;

					splits += layer->splits.size();
				}
				return splits;
			}
		}
	}

	return {};
}

QVariant ProgramsModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
			case 0: return tr("Program");
			case 1: return tr("Layers");
			case 2: return tr("Splits");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

/**
 * hmm. maybe the programs and splits vectors should be deques instead so insertion isn't as
 * expensive
 */
bool ProgramsModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	if (bank->programs.size() + count > manatools::mpb::MAX_PROGRAMS)
		return false;

	beginInsertRows({}, row, row + count - 1);
	bank->programs.insert(bank->programs.begin() + row, count, {});
	endInsertRows();

	return true;
}

bool ProgramsModel::moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destRow) {
	Q_UNUSED(srcParent);
	Q_UNUSED(destParent);

	if (srcRow < 0 || destRow < 0 || count <= 0 ||
	    srcRow + count - 1 >= rowCount() || destRow > rowCount() ||
	    !beginMoveRows({}, srcRow, srcRow + count - 1, {}, destRow))
		return false;

	auto begin = bank->programs.begin();
	if (destRow > srcRow) {
		std::rotate(begin + srcRow, begin + srcRow + count, begin + destRow);
	} else {
		std::rotate(begin + destRow, begin + srcRow, begin + srcRow + count);
	}

	endMoveRows();
	return true;
}

bool ProgramsModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto begin = bank->programs.begin() + row;

	beginRemoveRows({}, row, row + count - 1);
	bank->programs.erase(begin, begin + count);
	endRemoveRows();

	return true;
}

Qt::ItemFlags ProgramsModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);
	if (index.isValid())
		f |= Qt::ItemIsDragEnabled;
	return f | Qt::ItemIsDropEnabled;
}

bool ProgramsModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
	Q_UNUSED(column);

	if (action != Qt::MoveAction)
		return action == Qt::IgnoreAction;

	if (!data || !data->hasFormat(MIMEType))
		return false;

	int beginRow;
	int rowFrom;
	QByteArray encodedData = data->data(MIMEType);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);

	if (row != -1)
		beginRow = row;
	else if (parent.isValid())
		beginRow = parent.row();
	else
		beginRow = rowCount();

	if (stream.atEnd())
		return false;

	stream >> rowFrom;
	moveRow({}, rowFrom, {}, beginRow);

	return true;
}

QMimeData* ProgramsModel::mimeData(const QModelIndexList& indexes) const {
	std::optional<int> row;
	for (const auto& index : indexes) {
		if (index.isValid()) {
			row = index.row();
			break;
		}
	}

	if (!row.has_value())
		return nullptr;

	QMimeData* mimeData = new QMimeData();
	QByteArray encodedData;
	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	stream << *row;

	// I think QByteArray uses COW, so passing large data shouldn't be a huge issue
	mimeData->setData(MIMEType, encodedData);
	return mimeData;
}

QStringList ProgramsModel::mimeTypes() const {
	return { MIMEType };
}

Qt::DropActions ProgramsModel::supportedDropActions() const {
	return Qt::MoveAction;
}

void ProgramsModel::setBank(Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}
