#include <manatools/utils.hpp>
#include "LayersModel.hpp"

LayersModel::LayersModel(Bank* bank, size_t programIdx, QObject* parent) :
	QAbstractTableModel(parent),
	bank(bank),
	programIdx(programIdx)
{
	headerFont.setBold(true);
}

int LayersModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : manatools::mpb::MAX_LAYERS;
}

int LayersModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : 5;
}

QVariant LayersModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	const auto* layer = bank->layer(programIdx, index.row());
	if (!layer)
		return {};

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
			case 0: return index.row() + 1;
			case 1: return static_cast<uint>(layer->splits.size());
			case 2: return layer->delay * 4;
			case 3: return layer->bendRangeLow;
			case 4: return layer->bendRangeHigh;
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

QVariant LayersModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
			case 0: return tr("Layer");
			case 1: return tr("Splits");
			case 2: return tr("Delay");
			case 3: return tr("Bend [-]");
			case 4: return tr("Bend [+]");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

bool LayersModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	auto* layer = bank->layer(programIdx, index.row());
	if (!layer)
		return false;

	if (role == Qt::EditRole) {
		switch (index.column()) {
			case 2: return changeData(index, layer->delay,         std::clamp(manatools::utils::roundUp(value.toInt(), 4) / 4, 0, 1024));
			case 3: return changeData(index, layer->bendRangeLow,  std::clamp(value.toInt(), 0, 24));
			case 4: return changeData(index, layer->bendRangeHigh, std::clamp(value.toInt(), 0, 24));
		}
	}

	return false;
}

bool LayersModel::addLayer(int at) {
	auto* program = bank->program(programIdx);
	if (!program)
		return false;

	auto& layer = program->layers[at];
	if (layer.has_value())
		return false;

	layer.emplace();
	emit dataChanged(index(at, 0), index(at, 4));

	return true;
}

bool LayersModel::removeLayer(int at) {
	auto* program = bank->program(programIdx);
	if (!program)
		return false;

	auto& layer = program->layers[at];
	if (!layer.has_value())
		return false;

	layer.reset();
	emit dataChanged(index(at, 0), index(at, 4));

	return true;
}

Qt::ItemFlags LayersModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);

	if (index.isValid() && index.column() > 1 && bank->layer(programIdx, index.row())) {
		f |= Qt::ItemIsEditable;
	}

	return f;
}

void LayersModel::setBank(Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}

void LayersModel::setProgram(size_t idx) {
	beginResetModel();
	programIdx = idx;
	endResetModel();
}
