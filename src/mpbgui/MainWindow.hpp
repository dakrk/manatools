#pragma once
#include <QSettings>
#include <QMessageBox>
#include <manatools/mpb.hpp>
#include <guicommon/TonePlayer.hpp>

#include "ui_MainWindow.h"
#include "ProgramsModel.hpp"
#include "LayersModel.hpp"
#include "SplitsModel.hpp"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	bool loadFile(const QString& path);
	bool saveFile(const QString& path);
	bool loadMapFile(const QString& path);
	bool exportSF2File(const QString& path);

	void setProgram(size_t idx);
	void setLayer(size_t idx);
	void setSplit(size_t idx);

public slots:
	void newFile();
	bool open();
	bool save();
	bool saveAs();
	bool exportSF2();
	bool importTone();
	bool exportTone();
	void editBankProperties();
	void editLayer();
	void editSplit();
	void about();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	static void setCommonTableProps(QTableView* table);

	void emitRowChanged(QAbstractItemModel* table, int row);
	void connectTableMutations(QAbstractTableModel* model);

	void resetTableLayout();
	void reloadTables();

	QString maybeDropEvent(QDropEvent* event);
	bool maybeSave();
	void setCurrentFile(const QString& path = "");

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	Ui::MainWindow ui;

	ProgramsModel* programsModel;
	LayersModel* layersModel;
	SplitsModel* splitsModel;

	manatools::mpb::Bank bank;
	size_t programIdx;
	size_t layerIdx;
	size_t splitIdx;

	TonePlayer tonePlayer;
};
