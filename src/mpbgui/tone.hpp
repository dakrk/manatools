#pragma once
#include <QString>
#include <QWidget>
#include <manatools/mpb.hpp>

namespace tone {
	// TODO: Support P04, P08 and P16 formats
	enum class FileType {
		WAV, DAT
	};

	bool importDialog(QWidget* parent, manatools::mpb::Split& split, const QString& basePath);
	bool importFile(QWidget* parent, manatools::mpb::Split& split, const QString& path);

	bool exportDialog(QWidget* parent, const manatools::mpb::Split& split, const QString& basePath,
	                  const QString& baseName, const QString& tonePath);
	bool exportFile(QWidget* parent, const manatools::mpb::Split& split, const QString& path, FileType type);

	void convertToADPCM(manatools::tone::Tone& tone);
} // namespace tone
