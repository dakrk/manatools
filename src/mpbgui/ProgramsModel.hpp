#pragma once
#include <QAbstractTableModel>
#include <manatools/mpb.hpp>

class ProgramsModel : public QAbstractTableModel {
	Q_OBJECT
public:
	typedef manatools::mpb::Bank Bank;
	
	ProgramsModel(Bank* bank, QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool insertRows(int row, int count, const QModelIndex& parent = {}) override;
	bool removeRows(int row, int count, const QModelIndex& parent = {}) override;
	/**
	 * TODO: moving rows in Qt is utterly stupid so I'm not doing it for now. Good bye
	 * Why must the behaviour without making a custom TableView and other stuff be some
	 * serialisation and mime bullshit that calls insertRows and removeRows INSTEAD OF
	 * JUST USING THE MOVEROWS METHOD. THAT I HAVE. IN THE CLASS. WHEN I SPECIFY
	 * "INTERNALMOVE". unless I got something wrong but I really don't think so
	 */

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setBank(Bank* newBank);

private:
	Bank* bank;
};
