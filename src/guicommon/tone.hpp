#pragma once
#include <QString>
#include <QWidget>
#include <manatools/mpb.hpp>

namespace tone {
	// TODO: Support P04, P08 and P16 formats
	enum class FileType {
		WAV, DAT
	};

	bool importDialog(manatools::mpb::Split& split, const QString& basePath, QWidget* parent = nullptr);
	bool importFile(manatools::mpb::Split& split, const QString& path, QWidget* parent = nullptr);

	bool exportDialog(const manatools::mpb::Split& split, const QString& basePath, const QString& baseName,
	                  const QString& tonePath, QWidget* parent = nullptr);
	bool exportFile(const manatools::mpb::Split& split, const QString& path, FileType type, QWidget* parent = nullptr);

	bool convertToADPCM(manatools::tone::Tone& tone, QWidget* parent = nullptr);
} // namespace tone
