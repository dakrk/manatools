#include <QHeaderView>
#include "InstDataDialog.hpp"

#define SET_ROW_NUM_PAIR(table, row, num, num2) \
	table->setItem(row, 0, new QTableWidgetItem(QString::number(num))); \
	table->setItem(row, 1, new QTableWidgetItem(QString::number(num2)));

InstDataDialog::InstDataDialog(tone::Metadata current, tone::Metadata imported, QWidget* parent) :
	QDialog(parent)
{
	setFixedSize(300, 250);

	// TODO: Add question mark icon just like QMessageBox does

	mainLayout = new QVBoxLayout();
	text = new QLabel(tr("The imported audio file contains instrument data, would you like to use them?"));
	table = new QTableWidget();
	buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
	
	QStringList tableHorzHeaders;
	QStringList tableVertHeaders;
	tableHorzHeaders << tr("Current") << tr("Imported");
	tableVertHeaders << tr("L. Key") << tr("H. Key") << tr("Base Key") << tr("L. Vel") << tr("H. Vel");

	text->setWordWrap(true);

	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setColumnCount(tableHorzHeaders.size());
	table->setRowCount(tableVertHeaders.size());
	table->setHorizontalHeaderLabels(tableHorzHeaders);
	table->setVerticalHeaderLabels(tableVertHeaders);

	SET_ROW_NUM_PAIR(table, 0, current.startNote, imported.startNote);
	SET_ROW_NUM_PAIR(table, 1, current.endNote,   imported.endNote);
	SET_ROW_NUM_PAIR(table, 2, current.baseNote,  imported.baseNote);
	SET_ROW_NUM_PAIR(table, 3, current.startVel,  imported.startVel);
	SET_ROW_NUM_PAIR(table, 4, current.endVel,    imported.endVel);
	
	mainLayout->addWidget(text);
	mainLayout->addWidget(table);
	mainLayout->addWidget(buttons);
	
	setLayout(mainLayout);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
