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

	void restoreSettings();
	void saveSettings();

protected:
	void closeEvent(QCloseEvent* event);

private:
	QSettings settings;
	QString curFile;

	QTableView* table;
	MLTModel* model;

	manatools::mlt::MLT mlt;
};
