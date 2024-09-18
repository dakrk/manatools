#include "ChannelSelectDialog.hpp"

ChannelSelectDialog::ChannelSelectDialog(uint channels, QWidget* parent) :
	QDialog(parent),
	channel(0)
{
	setFixedSize(300, 250);

	// TODO: Add question mark icon just like QMessageBox does

	mainLayout = new QVBoxLayout();
	text = new QLabel(tr("The imported audio file contains multiple channels, which one should be used?"));
	list = new QListWidget();
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	text->setWordWrap(true);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);

	for (uint i = 1; i <= channels; i++) {
		list->addItem(QString::number(i));
	}

	list->setCurrentRow(0);
	
	mainLayout->addWidget(text);
	mainLayout->addWidget(list);
	mainLayout->addWidget(buttons);
	
	setLayout(mainLayout);

	connect(list, &QListWidget::currentRowChanged, this, [this](int currentRow) {
		channel = currentRow > 0 ? currentRow : 0;
	});

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
