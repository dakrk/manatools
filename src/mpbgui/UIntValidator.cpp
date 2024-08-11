#include "UIntValidator.hpp"

QValidator::State UIntValidator::validate(QString& input, int& pos) const {
	bool ok = false;
	uint num = input.toUInt(&ok);

	Q_UNUSED(pos);

	if (!ok)
		return QValidator::Invalid;

	if (num < min)
		return QValidator::Intermediate;

	if (num > max)
		return QValidator::Invalid;

	return QValidator::Acceptable;
}
