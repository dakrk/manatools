#include <QStringList>
#include <QTextStream>

class CSV {
	void read(QTextStream& stream);
	void write(QTextStream& stream);

	QList<QStringList> rows;
};
