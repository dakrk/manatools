#pragma once
#include <QApplication>
#include <QCursor>

// urgh
class CursorOverride {
public:
	CursorOverride(const QCursor& cursor) {
		QApplication::setOverrideCursor(cursor);
	}

	~CursorOverride() {
		restore();
	}

	void restore() {
		if (restored)
			return;

		QApplication::restoreOverrideCursor();
		restored = true;
	}

private:
	bool restored = false;
};
