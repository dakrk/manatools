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

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setMLT(manatools::mlt::MLT* newMLT);

private:
	QColor dimmedTextColor;
	QFont headerFont;
	manatools::mlt::MLT* mlt;
};
