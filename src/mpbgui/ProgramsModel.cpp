#include "ProgramsModel.hpp"

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
				int splits = 0;
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

bool ProgramsModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto begin = bank->programs.begin() + row;

	beginRemoveRows({}, row, row + count - 1);
	bank->programs.erase(begin, begin + count);
	endRemoveRows();

	return true;
}

Qt::ItemFlags ProgramsModel::flags(const QModelIndex& index) const {
	return QAbstractTableModel::flags(index);
}

void ProgramsModel::setBank(Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}
