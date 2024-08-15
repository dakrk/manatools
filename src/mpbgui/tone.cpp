#include <QMessageBox>
#include <QFileDialog>
#include <manatools/tonedecoder.hpp>
#include <sndfile.hh>

#include "CursorOverride.hpp"
#include "tone.hpp"

namespace tone {

bool importDialog(QWidget* parent, manatools::mpb::Bank& bank,
                  size_t programIdx, size_t layerIdx, size_t splitIdx,
                  const QString& basePath)
{
	auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	const QString path = QFileDialog::getOpenFileName(
		parent,
		QFileDialog::tr("Import split tone"),
		basePath
	);

	if (path.isEmpty())
		return false;

	return importFile(parent, *split, path);
}

bool importFile(QWidget* parent, manatools::mpb::Split& split, const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);

	SndfileHandle file(path.toStdString());

	return false;
}

bool exportDialog(QWidget* parent, const manatools::mpb::Bank& bank,
                  size_t programIdx, size_t layerIdx, size_t splitIdx,
                  const QString& basePath, const QString& baseName)
{
	const auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	const QStringList filters = {
		QFileDialog::tr("WAV file (*.wav)"),
		QFileDialog::tr("Raw tone data (*.dat)")
	};

	QString selectedFilter;

	const QString defPath = QDir(basePath).filePath(
		QString("%1_%2-%3-%4.wav")
			.arg(baseName.isEmpty() ? "untitled" : baseName)
			.arg(programIdx + 1)
			.arg(layerIdx + 1)
			.arg(splitIdx + 1)
	);

	const QString path = QFileDialog::getSaveFileName(
		parent,
		QFileDialog::tr("Export split tone"),
		defPath,
		filters.join(";;"),
		&selectedFilter
	);

	if (path.isEmpty())
		return false;

	FileType type;
	if (selectedFilter == filters[0])
		type = FileType::WAV;
	else
		type = FileType::DAT;

	return exportFile(parent, *split, path, type);
}

bool exportFile(QWidget* parent, const manatools::mpb::Split& split, const QString& path, FileType type) {
	CursorOverride cursor(Qt::WaitCursor);

	if (!split.tone.data) {
		cursor.restore();
		QMessageBox::warning(parent, "", QMessageBox::tr("Selected split has no tone data."));
		return false;
	}

	try {
		switch (type) {
			case FileType::WAV: {
				manatools::wav::WAV<s16> wav(1, 22050);

				if (split.loop) {
					wav.sampler = {
						.midiUnityNote = 60,
						.midiPitchFraction = 0,
						.loops = {
							{
								.start = split.loopStart,
								.end = split.loopEnd - 1u
							}
						}
					};
				}

				manatools::tone::Decoder decoder(&split.tone);
				wav.data.resize(split.tone.samples());
				decoder.decode(wav.data);
				
				wav.save(path.toStdString());
				break;
			}

			case FileType::DAT: {
				manatools::io::FileIO file(path.toStdString(), "wb");
				file.writeVec(*split.tone.data);
				break;
			}

			default: {
				return false;
			}
		}
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(parent, "", QMessageBox::tr("Failed to export tone: %1").arg(err.what()));
		return false;
	}

	return true;
}

} // namespace tone
