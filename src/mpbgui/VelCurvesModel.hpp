#pragma once
#include <QAbstractListModel>
#include <manatools/mpb.hpp>

class VelCurvesModel : public QAbstractListModel {
	Q_OBJECT
public:
	typedef manatools::mpb::Velocity Velocity;
	static const QString MIMEType;

	VelCurvesModel(std::vector<Velocity>& velocities, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
	bool moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destChild) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
	std::vector<Velocity>* velocities;
};
