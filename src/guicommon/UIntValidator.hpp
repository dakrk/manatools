#pragma once
#include <QValidator>
#include "common.hpp"

class GUICOMMON_EXPORT UIntValidator : public QValidator {
	Q_OBJECT
public:
	explicit UIntValidator(QObject* parent = nullptr) :
		QValidator(parent),
		min(std::numeric_limits<uint>::min()),
		max(std::numeric_limits<uint>::max()) {}

	UIntValidator(uint min, uint max, QObject* parent = nullptr) :
		QValidator(parent),
		min(min),
		max(max) {}

	uint bottom() const {
		return min;
	}

	uint top() const {
		return max;
	}

	/**
	 * The QIntValidator set methods emit bottomChanged and topChanged signals
	 * but we don't need them (well nor do we need the methods at all but)
	 */

	void setBottom(uint bottom) {
		min = bottom;
	}

	void setTop(uint top) {
		max = top;
	}

	void setRange(uint bottom, uint top) {
		setBottom(bottom);
		setTop(top);
	}

	QValidator::State validate(QString& input, int& pos) const override;

private:
	uint min;
	uint max;
};
