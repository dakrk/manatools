#pragma once
#include <QAbstractListModel>
#include <manatools/fob.hpp>

class FOBModel : public QAbstractListModel {
	Q_OBJECT
public:
	static const QString MIMEType;

	FOBModel(manatools::fob::Bank* bank, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
	bool moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destChild) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;

	void setBank(manatools::fob::Bank* newBank);

private:
	manatools::fob::Bank* bank;
};
