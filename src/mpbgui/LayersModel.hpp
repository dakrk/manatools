#pragma once
#include <QAbstractTableModel>
#include <QFont>
#include <manatools/mpb.hpp>

class LayersModel : public QAbstractTableModel {
	Q_OBJECT
public:
	typedef manatools::mpb::Bank Bank;
	
	LayersModel(Bank* bank, size_t programIdx, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	bool addLayer(int at);
	bool removeLayer(int at);

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setBank(Bank* newBank);
	void setPath(size_t newProgramIdx);

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
	size_t programIdx;

	QFont headerFont;
};
