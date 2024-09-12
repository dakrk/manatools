#pragma once
#include <QAbstractTableModel>
#include <manatools/mpb.hpp>

class ProgramsModel : public QAbstractTableModel {
	Q_OBJECT
public:
	typedef manatools::mpb::Bank Bank;
	static const QString MIMEType;

	ProgramsModel(Bank* bank, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
	bool moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destChild) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setBank(Bank* newBank);

private:
	Bank* bank;
};
