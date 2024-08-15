#pragma once
#include <QSettings>
#include <QMessageBox>
#include <manatools/mpb.hpp>

#include "ui_MainWindow.h"
#include "ProgramsModel.hpp"
#include "LayersModel.hpp"
#include "SplitsModel.hpp"
#include "TonePlayer.hpp"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	enum class ToneExportType {
		WAV, DAT
	};

	bool loadFile(const QString& path);
	bool saveFile(const QString& path);
	bool exportSF2File(const QString& path);

	void setProgram(size_t newProgramIdx);
	void setLayer(size_t newLayerIdx);
	void setSplit(size_t newSplitIdx);

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
	void closeEvent(QCloseEvent* event);
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);

private:
	static void setCommonTableProps(QTableView* table);
	static manatools::mpb::Velocity genDefVelCurve();

	void connectTableMutations(QAbstractTableModel* model);

	void resetTableLayout();
	void reloadTables();

	QString maybeDropEvent(QDropEvent* event);

	bool maybeSave();
	void setCurrentFile(const QString& path = "");
	QString getOutPath(bool dirOnly = false, const QString& newExtension = "") const;

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
