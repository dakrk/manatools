#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <manatools/types.hpp>
#include "common.hpp"

class GUICOMMON_EXPORT ChannelSelectDialog : public QDialog {
	Q_OBJECT
public:
	explicit ChannelSelectDialog(uint channels, QWidget* parent = nullptr);
	uint channel;

private:
	QVBoxLayout* mainLayout;
	QLabel* text;
	QListWidget* list;
	QDialogButtonBox* buttons;
};
