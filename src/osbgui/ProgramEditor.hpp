#pragma once
#include <QComboBox>
#include <QDialog>
#include <QSettings>
#include <manatools/osb.hpp>
#include <guicommon/TonePlayer.hpp>
#include <optional>

#include "ui_ProgramEditor.h"

class ProgramEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::osb::LFOWaveType LFOWaveType;
	typedef manatools::osb::Program Program;
	typedef manatools::tone::Tone Tone;

	explicit ProgramEditor(QWidget* parent = nullptr);
	ProgramEditor(const Program& program, QWidget* parent = nullptr);

	void setCurFile(const QString& in);
	void setPath(size_t programIdx);

	Program program;

public slots:
	bool importTone();
	bool exportTone();
	void convertToADPCM();
	void editUnknownProps();

protected:
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void init();

	static void addLFOWaveItems(QComboBox* box);
	bool setLFOWaveType(QComboBox* box, LFOWaveType type);

	QString maybeDropEvent(QDropEvent* event);
	void loadToneData();
	void loadProgramData();
	void setProgramData();

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	Ui::ProgramEditor ui;
	QMenu* editToneMenu;

	std::optional<size_t> programIdx;

	TonePlayer tonePlayer;
};
