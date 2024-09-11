#include <QIODevice>
#include <QMimeData>
#include <QPalette>
#include <guicommon/utils.hpp>
#include <optional>
#include "MLTModel.hpp"

const QString MLTModel::MIMEType = QStringLiteral("application/x-manatools-mltgui-mltmodel");

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
			case 2: return formatHex(unit.aicaDataPtr, 8);
			case 3: return formatHex(unit.aicaDataSize);
			case 4: {
				if (unit.fileDataPtr() == manatools::mlt::UNUSED) {
					return unit.shouldHaveData() ? tr("None") : tr("N/A");
				}
				return formatHex(unit.fileDataPtr(), 8);
			}
			case 5: return formatHex(unit.data.size());
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
		QString str = value.toString();
		if (index.column() == 0)
			return changeData(index, unit.fourCC, str.toUtf8().data());

		bool ok = false;
		uint val = str.toUInt(&ok, 0);
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
	mlt->units.insert(mlt->units.begin() + row, count, { fourCC });
	endInsertRows();
	return true;
}

bool MLTModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(row);
	Q_UNUSED(count);
	Q_UNUSED(parent);
	return false;
}

bool MLTModel::moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destRow) {
	Q_UNUSED(srcParent);
	Q_UNUSED(destParent);

	if (srcRow < 0 || destRow < 0 || count <= 0 || srcRow + count - 1 >= rowCount() || destRow > rowCount())
		return false;

	auto begin = mlt->units.begin();
	if (!beginMoveRows({}, srcRow, srcRow + count - 1, {}, destRow))
		return false;

	if (destRow > srcRow) {
		std::rotate(begin + srcRow, begin + srcRow + count, begin + destRow);
	} else {
		std::rotate(begin + destRow, begin + srcRow, begin + srcRow + count);
	}

	endMoveRows();
	return true;
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

	if (index.isValid()) {
		f |= Qt::ItemIsDragEnabled;

		if (0 <= index.column() && index.column() <= 3) {
			f |= Qt::ItemIsEditable;
		}
	}

	return f | Qt::ItemIsDropEnabled;
}

bool MLTModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
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

QMimeData* MLTModel::mimeData(const QModelIndexList& indexes) const {
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

QStringList MLTModel::mimeTypes() const {
	return { MIMEType };
}

Qt::DropActions MLTModel::supportedDropActions() const {
	return Qt::MoveAction;
}

void MLTModel::setMLT(manatools::mlt::MLT* newMLT) {
	beginResetModel();
	mlt = newMLT;
	endResetModel();
}
