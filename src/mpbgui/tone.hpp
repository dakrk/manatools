#pragma once
#include <QString>
#include <QWidget>
#include <manatools/mpb.hpp>
#include <manatools/wav.hpp>

/**
 * Tone import/export will be shared between MainWindow and ToneEditor (once it exists)
 * so it only makes sense to all be moved here
 */
namespace tone {
	// TODO: Support P04, P08 and P16 formats
	enum class FileType {
		WAV, DAT
	};

	bool importDialog(QWidget* parent, manatools::mpb::Bank& bank,
	                  size_t programIdx, size_t layerIdx, size_t splitIdx,
	                  const QString& basePath);

	bool importFile(QWidget* parent, manatools::mpb::Split& split, const QString& path);

	bool exportDialog(QWidget* parent, const manatools::mpb::Bank& bank,
	                  size_t programIdx, size_t layerIdx, size_t splitIdx,
	                  const QString& basePath, const QString& baseName);

	bool exportFile(QWidget* parent, const manatools::mpb::Split& split, const QString& path, FileType type);
} // namespace tone
