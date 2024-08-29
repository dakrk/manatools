#include "MLTModel.hpp"

MLTModel::MLTModel(manatools::mlt::MLT* mlt, QObject* parent) :
	QAbstractTableModel(parent),
	mlt(mlt)
{
	headerFont.setBold(true);
}

int MLTModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : mlt->units.size();
}

int MLTModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : 6;
}

QVariant MLTModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	manatools::mlt::Unit& unit = mlt->units[index.row()];

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
			case 0: return QString::fromUtf8({unit.fourCC, 4});
			case 1: return unit.bank;
			case 2: return unit.aicaDataPtr_;
			case 3: return unit.aicaDataSize_;
			case 4: return unit.fileDataPtr_;
			case 5: return QVariant::fromValue(unit.data.size());
		}
	} else if (role == Qt::TextAlignmentRole) {
		if (index.column() == 0) {
			return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
		}
	} else if (role == Qt::FontRole) {
		if (index.column() == 0) {
			return headerFont;
		}
	}

	return {};
}

QVariant MLTModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
			case 0: return tr("Type");
			case 1: return tr("Bank");
			case 2: return tr("Offset [AICA]");
			case 3: return tr("Size [AICA]");
			case 4: return tr("Offset [File]");
			case 5: return tr("Size [File]");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

void MLTModel::setMLT(manatools::mlt::MLT* newMLT) {
	beginResetModel();
	mlt = newMLT;
	endResetModel();
}
