#include <QIODevice>
#include <QMimeData>
#include <optional>
#include "FOBModel.hpp"

const QString FOBModel::MIMEType = QStringLiteral("application/x-manatools-fobgui-fobmodel");

FOBModel::FOBModel(manatools::fob::Bank* bank, QObject* parent) :
	QAbstractListModel(parent),
	bank(bank) {}

int FOBModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : bank->mixers.size();
}

QVariant FOBModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 0) {
		const auto& mixer = bank->mixers[index.row()];

		const QString* name = std::any_cast<QString>(&mixer.userData);
		if (name && !name->isEmpty()) {
			return *name;
		} else {
			return tr("Mixer %1").arg(index.row());
		}
	}

	return {};
}

bool FOBModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	if (role == Qt::EditRole && index.column() == 0) {
		bank->mixers[index.row()].userData = value.toString();
		return true;
	}

	return false;
}

bool FOBModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);
	beginInsertRows({}, row, row + count - 1);
	bank->mixers.insert(bank->mixers.begin() + row, count, {});
	endInsertRows();
	return true;
}

bool FOBModel::moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destRow) {
	Q_UNUSED(srcParent);
	Q_UNUSED(destParent);

	if (srcRow < 0 || destRow < 0 || count <= 0 ||
	    srcRow + count - 1 >= rowCount() || destRow > rowCount() ||
	    !beginMoveRows({}, srcRow, srcRow + count - 1, {}, destRow))
		return false;

	auto begin = bank->mixers.begin();
	if (destRow > srcRow) {
		std::rotate(begin + srcRow, begin + srcRow + count, begin + destRow);
	} else {
		std::rotate(begin + destRow, begin + srcRow, begin + srcRow + count);
	}

	endMoveRows();
	return true;
}

bool FOBModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);
	auto begin = bank->mixers.begin() + row;
	beginRemoveRows({}, row, row + count - 1);
	bank->mixers.erase(begin, begin + count);
	endRemoveRows();
	return true;
}

Qt::ItemFlags FOBModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractListModel::flags(index);

	if (index.isValid()) {
		f |= Qt::ItemIsDragEnabled;

		if (index.column() == 0) {
			f |= Qt::ItemIsEditable;
		}
	}

	return f | Qt::ItemIsDropEnabled;
}

bool FOBModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
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

QMimeData* FOBModel::mimeData(const QModelIndexList& indexes) const {
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

QStringList FOBModel::mimeTypes() const {
	return { MIMEType };
}

Qt::DropActions FOBModel::supportedDropActions() const {
	return Qt::MoveAction;
}

void FOBModel::setBank(manatools::fob::Bank* newBank) {
	beginResetModel();
	bank = newBank;
	endResetModel();
}
