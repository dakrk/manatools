#include "SplitsModel.hpp"

SplitsModel::SplitsModel(Bank* bank, size_t programIdx, size_t layerIdx, QObject* parent) :
	QAbstractTableModel(parent),
	bank(bank),
	programIdx(programIdx),
	layerIdx(layerIdx)
{
	headerFont.setBold(true);
}

int SplitsModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		if (const auto* layer = bank->layer(programIdx, layerIdx)) {
			return layer->splits.size();
		}
	}

	return 0;
}

int SplitsModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : 10;
}

QVariant SplitsModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	const auto* split = bank->split(programIdx, layerIdx, index.row());
	if (!split)
		return {};

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
			case 0: return index.row() + 1;
			case 1: return split->startNote;
			case 2: return split->endNote;
			case 3: return split->baseNote;
			case 4: return split->velocityLow;
			case 5: return split->velocityHigh;
			case 6: return split->directLevel;
			case 7: return split->panPot;
			case 8: return split->fx.level;
			case 9: return split->fx.inputCh;
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

QVariant SplitsModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
			case 0: return tr("Split");
			case 1: return tr("L. Key");
			case 2: return tr("H. Key");
			case 3: return tr("Base Key");
			case 4: return tr("L. Vel");
			case 5: return tr("H. Vel");
			case 6: return tr("Direct Level");
			case 7: return tr("Pan Pot");
			case 8: return tr("FX Level");
			case 9: return tr("FX Ch.");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

bool SplitsModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	auto* split = bank->split(programIdx, layerIdx, index.row());
	if (!split)
		return false;

	if (role == Qt::EditRole) {
		switch (index.column()) {
			/**
			 * Qt's automatic spinboxes when returning uint values doesn't have any way
			 * to set min/max and oddly goes below 0, so we just read as int so negative
			 * values don't wrap (and therefore we use std::clamp instead of qMin).
			 */
			case 1: return changeData(index, split->startNote,    qBound(value.toInt(),   0, 127));
			case 2: return changeData(index, split->endNote,      qBound(value.toInt(),   0, 127));
			case 3: return changeData(index, split->baseNote,     qBound(value.toInt(),   0, 127));
			case 4: return changeData(index, split->velocityLow,  qBound(value.toInt(),   0, 127));
			case 5: return changeData(index, split->velocityHigh, qBound(value.toInt(),   0, 127));
			case 6: return changeData(index, split->directLevel,  qBound(value.toInt(),   0,  15));
			case 7: return changeData(index, split->panPot,       qBound(value.toInt(), -15,  15));
			case 8: return changeData(index, split->fx.level,     qBound(value.toInt(),   0,  15));
			case 9: return changeData(index, split->fx.inputCh,   qBound(value.toInt(),   0,  15));
		}
	}

	return false;
}

bool SplitsModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto* layer = bank->layer(programIdx, layerIdx);
	if (!layer)
		return false;

	if (layer->splits.size() + count > manatools::mpb::MAX_SPLITS)
		return false;

	beginInsertRows({}, row, row + count - 1);
	layer->splits.insert(layer->splits.begin() + row, count, {});
	endInsertRows();

	return true;
}

bool SplitsModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto* layer = bank->layer(programIdx, layerIdx);
	if (!layer)
		return false;

	auto begin = layer->splits.begin() + row;

	beginRemoveRows({}, row, row + count - 1);
	layer->splits.erase(begin, begin + count);
	endRemoveRows();

	return true;
}

Qt::ItemFlags SplitsModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);

	if (index.isValid() && index.column() != 0) {
		f |= Qt::ItemIsEditable;
	}

	return f;
}

void SplitsModel::setBank(Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}

void SplitsModel::setPath(size_t newProgramIdx, size_t newLayerIdx) {
	beginResetModel();
	programIdx = newProgramIdx;
	layerIdx = newLayerIdx;
	endResetModel();
}
