#pragma once
#include <QSettings>
#include <manatools/osb.hpp>
#include <guicommon/TonePlayer.hpp>
#include <optional>

#include "ui_MainWindow.h"
#include "OSBModel.hpp"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	bool loadFile(const QString& path);
	bool saveFile(const QString& path);
	bool loadMapFile(const QString& path);
	bool saveMapFile(const QString& path);

public slots:
	void newFile();
	bool open();
	bool save();
	bool saveAs();
	void delProgram();
	void selectAll();
	void versionDialog();
	void about();
	bool importTone();
	bool exportTone();
	void editProgram();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void emitRowChanged(QAbstractItemModel* table, int row);

	void reloadTable();

	bool programNameSet() const;
	bool saveMappingsDialog();

	QString maybeDropEvent(QDropEvent* event);
	bool maybeSave();
	void setCurrentFile(const QString& path = "");

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	Ui::MainWindow ui;
	OSBModel* model;

	manatools::osb::Bank bank;

	TonePlayer tonePlayer;

	std::optional<bool> saveMappings;
};
