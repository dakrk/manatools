#pragma once
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QValidator>
#include "common.hpp"

class GUICOMMON_EXPORT FourCCDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit FourCCDelegate(QObject* parent = nullptr);
	explicit FourCCDelegate(bool forceUpperCase, QObject* parent = nullptr);
	
	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

	bool forceUpperCase;
};
