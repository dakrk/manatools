#include "FourCCDelegate.hpp"

FourCCDelegate::FourCCDelegate(QObject* parent) :
	QStyledItemDelegate(parent), forceUpperCase(false) {}

FourCCDelegate::FourCCDelegate(bool forceUpperCase, QObject* parent) :
	QStyledItemDelegate(parent), forceUpperCase(forceUpperCase) {}

QWidget* FourCCDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	Q_UNUSED(option);
	Q_UNUSED(index);
	QLineEdit* editor = new QLineEdit(parent);
	editor->setMaxLength(4);

	connect(editor, &QLineEdit::textEdited, this, [this, editor](const QString& text) {
		if (forceUpperCase)
			editor->setText(text.toUpper());
	});

	return editor;
}

void FourCCDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
	QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
	lineEdit->setText(index.data(Qt::EditRole).toString());
}

void FourCCDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
	QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
	model->setData(index, lineEdit->text());
}
