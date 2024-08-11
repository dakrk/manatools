#pragma once
#include <QDialog>
#include <manatools/mpb.hpp>

#include "ui_LayerEditor.h"

class LayerEditor : public QDialog {
	Q_OBJECT
public:
	typedef manatools::mpb::Layer Layer;
	
	LayerEditor(Layer* layer, QWidget* parent = nullptr);

	void setPath(size_t programIdx, size_t layerIdx);

private:
	void connectSpinnerHexLabel(QSpinBox* spinner, QLabel* label);

	void loadData();
	void setData();

	Ui::LayerEditor ui;

	Layer* layer;
};
