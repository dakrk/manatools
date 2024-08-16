#include <QHeaderView>
#include "InstDataDialog.hpp"

InstDataDialog::InstDataDialog(Data current, Data imported, QWidget* parent) :
	QDialog(parent)
{
	resize(300, 250);

	// TODO: Add question mark icon just like QMessageBox does

	mainLayout = new QVBoxLayout();
	text = new QLabel(tr("The imported audio file contains instrument data, would you like to use it?"));
	table = new QTableWidget();
	buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
	
	QStringList tableHorzHeaders;
	QStringList tableVertHeaders;
	tableHorzHeaders << tr("Current") << tr("Imported");
	tableVertHeaders << tr("L. Key") << tr("H. Key") << tr("Base Key") << tr("L. Vel") << tr("H. Vel");

	text->setWordWrap(true);

	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setColumnCount(tableHorzHeaders.size());
	table->setRowCount(tableVertHeaders.size());
	table->setHorizontalHeaderLabels(tableHorzHeaders);
	table->setVerticalHeaderLabels(tableVertHeaders);
	
	mainLayout->addWidget(text);
	mainLayout->addWidget(table);
	mainLayout->addWidget(buttons);
	
	setLayout(mainLayout);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
