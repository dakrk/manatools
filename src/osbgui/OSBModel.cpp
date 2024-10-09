#include <QIODevice>
#include <QMimeData>
#include <optional>
#include "OSBModel.hpp"

const QString OSBModel::MIMEType = QStringLiteral("application/x-manatools-osbgui-osbmodel");

OSBModel::OSBModel(Bank* bank, QObject* parent) :
	QAbstractTableModel(parent),
	bank(bank) {}

int OSBModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : bank->programs.size();
}

int OSBModel::columnCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : 7;
}

QVariant OSBModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const auto& program = bank->programs[index.row()];

		switch (index.column()) {
			case 0: {
				const QString* name = std::any_cast<QString>(&program.userData);
				if (name && !name->isEmpty()) {
					return *name;
				} else {
					return tr("Program %1").arg(index.row());
				}
			}
			case 1: return static_cast<uint>(program.tone.samples());
			case 2: return program.oscillatorLevel;
			case 3: return program.directLevel;
			case 4: return program.panPot;
			case 5: return program.fx.level;
			case 6: return program.fx.inputCh;
		}
	}

	return {};
}

QVariant OSBModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0: return tr("Program Name");
				case 1: return tr("Samples");
				case 2: return tr("Osc. Level");
				case 3: return tr("Direct Level");
				case 4: return tr("Pan Pot");
				case 5: return tr("FX Level");
				case 6: return tr("FX Ch.");
			}
		} else if (orientation == Qt::Vertical) {
			return section;
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

bool OSBModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	auto& program = bank->programs[index.row()];

	/**
	 * Not sure if I want a program name change to appear as if the bank has changed, since
	 * program names are not actually stored in the file
	 */
	if (role == Qt::EditRole) {
		switch (index.column()) {
			case 0: { program.userData = value.toString(); return true; }
			case 2: return changeData(index, program.oscillatorLevel, qBound(value.toInt(),   0, 255));
			case 3: return changeData(index, program.directLevel,     qBound(value.toInt(),   0,  15));
			case 4: return changeData(index, program.panPot,          qBound(value.toInt(), -15,  15));
			case 5: return changeData(index, program.fx.level,        qBound(value.toInt(),   0,  15));
			case 6: return changeData(index, program.fx.inputCh,      qBound(value.toInt(),   0,  15));
		}
	}

	return false;
}

bool OSBModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);
	beginInsertRows({}, row, row + count - 1);
	bank->programs.insert(bank->programs.begin() + row, count, {});
	endInsertRows();
	return true;
}

bool OSBModel::moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destRow) {
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

bool OSBModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);

	auto begin = bank->programs.begin() + row;
	beginRemoveRows({}, row, row + count - 1);
	bank->programs.erase(begin, begin + count);
	endRemoveRows();
	return true;
}

Qt::ItemFlags OSBModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractTableModel::flags(index);

	if (index.isValid()) {
		f |= Qt::ItemIsDragEnabled;

		if (index.column() != 1) {
			f |= Qt::ItemIsEditable;
		}
	} else {
		f |= Qt::ItemIsDropEnabled;
	}

	return f;
}

bool OSBModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
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

QMimeData* OSBModel::mimeData(const QModelIndexList& indexes) const {
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

	mimeData->setData(MIMEType, encodedData);
	return mimeData;
}

QStringList OSBModel::mimeTypes() const {
	return { MIMEType };
}

Qt::DropActions OSBModel::supportedDropActions() const {
	return Qt::MoveAction;
}

void OSBModel::setBank(Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}
