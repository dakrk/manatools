#include <QIODevice>
#include <QMimeData>
#include <optional>
#include "VelCurvesModel.hpp"

const QString VelCurvesModel::MIMEType = QStringLiteral("application/x-manatools-mpbgui-velcurvesmodel");

VelCurvesModel::VelCurvesModel(std::vector<Velocity>& velocities, QObject* parent) :
	QAbstractListModel(parent),
	velocities(&velocities) {}

int VelCurvesModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : velocities->size();
}

QVariant VelCurvesModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	if (role == Qt::DisplayRole) {
		return tr("Curve %1").arg(index.row() + 1);
	}

	return {};
}

bool VelCurvesModel::insertRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);
	beginInsertRows({}, row, row + count - 1);
	velocities->insert(velocities->begin() + row, count, Velocity::defaultCurve());
	endInsertRows();
	return true;
}

bool VelCurvesModel::moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destRow) {
	Q_UNUSED(srcParent);
	Q_UNUSED(destParent);

	if (srcRow < 0 || destRow < 0 || count <= 0 ||
	    srcRow + count - 1 >= rowCount() || destRow > rowCount() ||
	    !beginMoveRows({}, srcRow, srcRow + count - 1, {}, destRow))
		return false;

	auto begin = velocities->begin();
	if (destRow > srcRow) {
		std::rotate(begin + srcRow, begin + srcRow + count, begin + destRow);
	} else {
		std::rotate(begin + destRow, begin + srcRow, begin + srcRow + count);
	}

	endMoveRows();
	return true;
}

bool VelCurvesModel::removeRows(int row, int count, const QModelIndex& parent) {
	Q_UNUSED(parent);
	auto begin = velocities->begin() + row;
	beginRemoveRows({}, row, row + count - 1);
	velocities->erase(begin, begin + count);
	endRemoveRows();
	return true;
}

Qt::ItemFlags VelCurvesModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags f = QAbstractListModel::flags(index);
	if (index.isValid())
		f |= Qt::ItemIsDragEnabled;
	return f | Qt::ItemIsDropEnabled;
}

bool VelCurvesModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
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

QMimeData* VelCurvesModel::mimeData(const QModelIndexList& indexes) const {
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

QStringList VelCurvesModel::mimeTypes() const {
	return { MIMEType };
}

Qt::DropActions VelCurvesModel::supportedDropActions() const {
	return Qt::MoveAction;
}
