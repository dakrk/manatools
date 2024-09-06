#pragma once
#include <QMainWindow>
#include <QSettings>
#include <QTableView>
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
	void packMLT(bool useAICASizes);
	bool addUnit(const QString& fourCC);
	bool importUnitDialog();
	bool exportUnitDialog();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	bool importUnit(manatools::mlt::Unit& unit, const QString& path);
	bool exportUnit(const manatools::mlt::Unit& unit, const QString& path);

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

	manatools::mlt::MLT mlt;
};
