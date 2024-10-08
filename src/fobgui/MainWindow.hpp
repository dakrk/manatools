#pragma once
#include <QListView>
#include <QMainWindow>
#include <QSettings>
#include <QSlider>
#include <QVector>
#include <manatools/fob.hpp>

#include "FOBModel.hpp"

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
	void versionDialog();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void loadMixerData(const manatools::fob::Mixer& mixer);
	void saveMixerData(manatools::fob::Mixer& mixer);

	QString maybeDropEvent(QDropEvent* event);
	bool maybeSave();
	void setCurrentFile(const QString& path = "");

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	QListView* list;
	FOBModel* model;

	QVector<QSlider*> levelSliders;
	QVector<QSlider*> panSliders;

	manatools::fob::Bank bank;
};
