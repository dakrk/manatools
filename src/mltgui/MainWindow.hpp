#pragma once
#include <QMainWindow>
#include <QSettings>

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
};
