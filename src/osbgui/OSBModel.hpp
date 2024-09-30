#pragma once
#include <QAbstractTableModel>
#include <manatools/osb.hpp>

class OSBModel : public QAbstractTableModel {
	Q_OBJECT
public:
	typedef manatools::osb::Bank Bank;
	static const QString MIMEType;

	OSBModel(Bank* bank, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
	bool moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destChild) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;

	void setBank(Bank* newBank);

private:
	template <typename T, typename T2>
	bool changeData(const QModelIndex& index, T& out, const T2& in) {
		if (out == in)
			return false;
		out = in;
		emit dataChanged(index, index, { Qt::EditRole });
		return true;
	}

	Bank* bank;
};
