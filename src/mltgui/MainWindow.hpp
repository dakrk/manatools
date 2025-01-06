#pragma once
#include <QLabel>
#include <QMainWindow>
#include <QSettings>
#include <QTableView>
#include <manatools/fourcc.hpp>
#include <manatools/mlt.hpp>

#include "MLTModel.hpp"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	bool loadFile(const QString& path);
	bool saveFile(const QString& path);

public slots:
	void newFile();
	bool open();
	bool save();
	bool saveAs();
	void about();
	void dataModified();
	void selectAll();
	void versionDialog();
	void packMLT(bool useAICASizes);
	bool addUnit(const manatools::FourCC fourCC);
	void delUnit();
	void clearUnitData();
	bool importUnitDialog();
	bool exportUnitDialog();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	bool importUnit(manatools::mlt::Unit& unit, const QString& path);
	bool exportUnit(const manatools::mlt::Unit& unit, const QString& path);

	void emitRowChanged(QAbstractItemModel* model, int row);
	void updateRAMStatus();
	void updateUnitStatus();
	bool checkUnitBanksValid(QString* log = nullptr);

	void resetTableLayout();
	void reloadTable();

	QString maybeDropEvent(QDropEvent* event);
	bool maybeSave();
	void setCurrentFile(const QString& path = "");

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	QTableView* table;
	MLTModel* model;
	QLabel* ramStatus;
	QLabel* curUnitStatus;

	QAction* importUnitAction;
	QAction* exportUnitAction;
	QAction* clearUnitAction;
	QAction* deleteUnitAction;

	manatools::mlt::MLT mlt;
};
