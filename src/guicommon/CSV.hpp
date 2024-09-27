#include <QStringList>
#include <QTextStream>

struct CSV {
	void read(QTextStream& stream);
	void write(QTextStream& stream) const;

	QList<QStringList> rows;
};
