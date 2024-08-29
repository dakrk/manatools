#include <QPalette>
#include <guicommon/utils.hpp>
#include "MLTModel.hpp"

MLTModel::MLTModel(manatools::mlt::MLT* mlt, QObject* parent) :
	QAbstractTableModel(parent),
	dimmedTextColor(QPalette().color(QPalette::WindowText)),
	mlt(mlt)
{
	headerFont.setBold(true);
	dimmedTextColor.setAlpha(96);
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
			case 0: return QString(unit.fourCC);
			case 1: return unit.bank;
			case 2: return formatPtr32(unit.aicaDataPtr_);
			case 3: return unit.aicaDataSize_;
			case 4: {
				if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
					return !strcmp(unit.fourCC, "SFPW") ? "N/A" : tr("None");
				}
				return formatPtr32(unit.fileDataPtr());
			}
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
	} else if (role == Qt::ForegroundRole) {
		if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
			return dimmedTextColor;
		}
	}

	return {};
}

QVariant MLTModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0: return tr("Type");
				case 1: return tr("Bank");
				case 2: return tr("Offset [AICA]");
				case 3: return tr("Size [AICA]");
				case 4: return tr("Offset [File]");
				case 5: return tr("Size [File]");
			}			
		} else if (orientation == Qt::Vertical) {
			return section;
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags MLTModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);

	if (index.isValid() && index.column() < 4) {
		// f |= Qt::ItemIsEditable;
	}

	return f;
}

void MLTModel::setMLT(manatools::mlt::MLT* newMLT) {
	beginResetModel();
	mlt = newMLT;
	endResetModel();
}
