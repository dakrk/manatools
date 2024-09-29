#pragma once
#include <QMainWindow>
#include <QSettings>
#include <QTableView>
#include <manatools/osb.hpp>

#include "ui_MainWindow.h"
#include "OSBModel.hpp"

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
	void selectAll();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void resetTableLayout();
	void reloadTable();

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
};
