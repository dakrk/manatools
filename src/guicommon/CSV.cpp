#include "CSV.hpp"

// Won't support quoted fields containing newlines, but I suppose that's ok for our current purposes
void CSV::read(QTextStream& stream) {
	rows.clear();

	QString row;
	while (stream.readLineInto(&row)) {
		bool inQuotes = false;
		QStringList cols;
		QString col;
		col.reserve(16);

		for (qsizetype i = 0; i < row.size(); i++) {
			QChar c = row[i];
			switch (c.unicode()) {
				case ',': {
					if (!inQuotes) {
						cols.append(col);
						col = "";
					} else {
						col += c;
					}
					break;
				}

				case '"': {
					if (inQuotes && i + 1 < row.size() && row[i + 1] == '"') {
						col += '"';
						i++;
					} else {
						inQuotes = !inQuotes;
					}
					break;
				}

				default: {
					col += c;
					break;
				}
			}
		}

		cols.append(col);
		rows.append(cols);
	}
}

void CSV::write(QTextStream& stream) const {
	for (const QStringList& cols : rows) {
		for (qsizetype i = 0; i < cols.size(); i++) {
			const QString& col = cols[i];

			if (col.contains(',') || col.contains('"')) {
				QString colEscaped = col;
				colEscaped.replace("\"", "\"\"");
				stream << '"' << colEscaped << '"';
			} else {
				stream << col;
			}

			if (i + 1 < cols.size()) {
				stream << ',';
			}
		}
		stream << '\n';
	}
}
