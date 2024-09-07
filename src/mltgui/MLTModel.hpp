#pragma once
#include <QAbstractTableModel>
#include <QColor>
#include <QFont>
#include <manatools/mlt.hpp>

class MLTModel : public QAbstractTableModel {
	Q_OBJECT
public:
	MLTModel(manatools::mlt::MLT* mlt, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	bool insertUnits(int row, int count, const manatools::FourCC fourCC);
	bool insertRows(int row, int count, const QModelIndex& parent = {}) override; // unused, use insertUnits
	bool moveRows(const QModelIndex& srcParent, int srcRow, int count, const QModelIndex& destParent, int destChild) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setMLT(manatools::mlt::MLT* newMLT);

private:
	template <typename T, typename T2>
	bool changeData(const QModelIndex& index, T& out, const T2& in) {
		if (out == in)
			return false;

		out = in;

		emit dataChanged(index, index, { Qt::EditRole });
		return true;
	}

	QColor dimmedTextColor;
	QFont headerFont;
	QFont monoFont;
	manatools::mlt::MLT* mlt;
};
