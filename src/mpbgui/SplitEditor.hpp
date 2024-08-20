#pragma once
#include <QComboBox>
#include <QDialog>
#include <QSettings>
#include <manatools/mpb.hpp>

#include "ui_SplitEditor.h"
#include "TonePlayer.hpp"

class SplitEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::LFOWaveType LFOWaveType;
	typedef manatools::mpb::Split Split;
	typedef manatools::mpb::Bank Bank;
	typedef manatools::tone::Tone Tone;

	explicit SplitEditor(QWidget* parent = nullptr);
	SplitEditor(const Split& split, Bank* bank, QWidget* parent = nullptr);

	void setCurFile(const QString& in);
	void setPath(size_t programIdx, size_t layerIdx, size_t splitIdx);

	Split& split() {
		return split_;
	}

	const Split& split() const {
		return split_;
	}

public slots:
	void editVelCurve();
	bool importTone();
	bool exportTone();
	void convertToADPCM();
	void editUnknownProps();

protected:
	void closeEvent(QCloseEvent* event);

private:
	void init();

	void addVelCurveItems(QComboBox* box);
	bool setVelCurve(QComboBox* box, u32 i);

	static void addLFOWaveItems(QComboBox* box);
	bool setLFOWaveType(QComboBox* box, LFOWaveType type);

	void loadToneData();
	void loadSplitData();
	void setSplitData();

	void restoreSettings();
	void saveSettings();

	QSettings settings;
	QString curFile;

	Ui::SplitEditor ui;

	QMenu* editToneMenu;

	Split split_;
	Bank* bank;

	// Used for window title and export file name
	bool pathSet = false;
	size_t programIdx_ = 0;
	size_t layerIdx_ = 0;
	size_t splitIdx_ = 0;

	// TODO: Initialising one of these each time a dialog is opened is noticeably slower
	TonePlayer tonePlayer;
};
