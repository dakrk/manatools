#include <QPalette>
#include <guicommon/utils.hpp>
#include "MLTModel.hpp"

MLTModel::MLTModel(manatools::mlt::MLT* mlt, QObject* parent) :
	QAbstractTableModel(parent),
	dimmedTextColor(QPalette().color(QPalette::WindowText)),
	monoFont("monospace"),
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
			case 0: return QString(unit.fourCC.data());
			case 1: return unit.bank;
			case 2: return formatHex(unit.aicaDataPtr);
			case 3: return formatHex(unit.aicaDataSize, 0);
			case 4: {
				if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
					return unit.fourCC == "SFPW" ? "N/A" : tr("None");
				}
				return formatHex(unit.fileDataPtr());
			}
			case 5: return formatHex(unit.data.size(), 0);
		}
	} else if (role == Qt::TextAlignmentRole) {
		if (index.column() == 0) {
			return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
		}
	} else if (role == Qt::FontRole) {
		switch (index.column()) {
			case 0: return headerFont;
			case 2: return monoFont;
			case 3: return monoFont;
			case 4: return monoFont;
			case 5: return monoFont;
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

bool MLTModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	manatools::mlt::Unit& unit = mlt->units[index.row()];

	if (role == Qt::EditRole) {
		bool ok = false;
		uint val = value.toString().toUInt(&ok, 0);
		if (!ok)
			return false;

		switch (index.column()) {
			case 1: return changeData(index, unit.bank,         val);
			case 2: return changeData(index, unit.aicaDataPtr,  val);
			case 3: return changeData(index, unit.aicaDataSize, val);
		}
	}

	return false;
}

bool MLTModel::insertUnits(int row, int count, const manatools::FourCC fourCC) {
	beginInsertRows({}, row, row + count - 1);
	auto startIt = mlt->units.insert(mlt->units.begin() + row, count, {});
	for (auto it = startIt; it < startIt + count; it++) {
		it->fourCC = fourCC;
	}
	endInsertRows();

	return true;
}

bool MLTModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(row);
	Q_UNUSED(count);
	Q_UNUSED(parent);
	return false;
}

bool MLTModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto begin = mlt->units.begin() + row;
	beginRemoveRows({}, row, row + count - 1);
	mlt->units.erase(begin, begin + count);
	endRemoveRows();

	return true;
}

Qt::ItemFlags MLTModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);

	if (index.isValid() && (1 <= index.column() && index.column() <= 3)) {
		f |= Qt::ItemIsEditable;
	}

	return f;
}

void MLTModel::setMLT(manatools::mlt::MLT* newMLT) {
	beginResetModel();
	mlt = newMLT;
	endResetModel();
}
