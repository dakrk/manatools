#include <QMessageBox>
#include <QFileDialog>
#include <manatools/tonedecoder.hpp>
#include <sndfile.hh>

#include "tone.hpp"
#include "CursorOverride.hpp"
#include "InstDataDialog.hpp"

// we're not in a class, and can't make aliases to a static method with default args
constexpr auto tr = [](const char* sourceText, const char* disambiguation = nullptr, int n = -1) {
	return QObject::tr(sourceText, disambiguation, n);
};

// well, libsndfile doesn't have a "read_s16" precisely
static_assert(sizeof(short) == 2, "Size of short is not 2 bytes/16 bits");

namespace tone {

constexpr const size_t READ_SIZE = 512;

bool importDialog(QWidget* parent, manatools::mpb::Bank& bank,
                  size_t programIdx, size_t layerIdx, size_t splitIdx,
                  const QString& basePath)
{
	auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	const QString path = QFileDialog::getOpenFileName(
		parent,
		tr("Import split tone"),
		basePath
	);

	if (path.isEmpty())
		return false;

	return importFile(parent, *split, path);
}

bool importFile(QWidget* parent, manatools::mpb::Split& split, const QString& path) {
	CursorOverride cursor(Qt::WaitCursor);
	SndfileHandle sndFile(path.toStdString());

	/**
	 * well... for `!sndFile` it wouldn't even be able to get a message because
	 * it would be a null handle
	 */
	if (!sndFile || sndFile.error() != SF_ERR_NO_ERROR) {
		cursor.restore();
		QMessageBox::warning(parent, "", tr("Failed to import sound file: %1").arg(sndFile.strError()));
		return false;
	}

	// TODO: Channel select dialog, as well as option to downmix multiple
	auto channels = sndFile.channels();
	if (channels > 1) {
		QMessageBox::warning(
			parent, "",
			tr("More than 1 channel was found in the imported file. Only channel 1 will be read.")
		);
	}

	// TODO: Ask for option if you want to convert to ADPCM?

	std::vector<s16> readBuf(READ_SIZE * channels);

	manatools::tone::Tone newTone;
	newTone.format = manatools::tone::Format::PCM16,
	newTone.data = manatools::tone::makeDataPtr(sndFile.frames() * sizeof(s16));	

	sf_count_t framesRead;
	sf_count_t totalFramesRead = 0;
	while ((framesRead = sndFile.readf(readBuf.data(), READ_SIZE)) > 0) {
		for (sf_count_t i = 0; i < framesRead; i++) {
			s16* data = reinterpret_cast<s16*>(newTone.data->data());
			data[totalFramesRead + i] = readBuf[i * channels + 0]; // 0 meaning channel 1
		}
		totalFramesRead += framesRead;
	}

	if (totalFramesRead != sndFile.frames()) {
		cursor.restore();
		QMessageBox::warning(parent, "", tr("Could not fully read audio data from imported sound file."));
		return false;
	}

	SF_INSTRUMENT instrument;
	if (sndFile.command(SFC_GET_INSTRUMENT, &instrument, sizeof(instrument))) {
		if (instrument.loop_count >= 1) {
			if (instrument.loop_count > 1)
				QMessageBox::warning(parent, "", tr("Imported sound file has more than one loop. Using first loop."));

			auto loop = instrument.loops[0];
			split.loop = loop.mode != SF_LOOP_NONE;
			split.loopStart = loop.start;
			split.loopEnd = loop.end;
		}

		InstDataDialog instDataDlg(
			{
				.startNote = split.startNote,
				.endNote   = split.endNote,
				.baseNote  = split.baseNote,
				.startVel  = split.velocityLow,
				.endVel    = split.velocityHigh
			},
			{
				// should probably just make instdatadialog take an s8 instead but... whatever
				.startNote = static_cast<u8>(instrument.key_lo),
				.endNote   = static_cast<u8>(instrument.key_hi),
				.baseNote  = static_cast<u8>(instrument.basenote),
				.startVel  = static_cast<u8>(instrument.velocity_lo),
				.endVel    = static_cast<u8>(instrument.velocity_hi)
			},
			parent
		);

		if (instDataDlg.exec() == QDialog::Accepted) {
			split.startNote    = instrument.key_lo;
			split.endNote      = instrument.key_hi;
			split.baseNote     = instrument.basenote;
			split.velocityLow  = instrument.velocity_lo;
			split.velocityHigh = instrument.velocity_hi;
		}
	}

	split.tone = std::move(newTone);

	return true;
}

bool exportDialog(QWidget* parent, const manatools::mpb::Bank& bank,
                  size_t programIdx, size_t layerIdx, size_t splitIdx,
                  const QString& basePath, const QString& baseName)
{
	const auto* split = bank.split(programIdx, layerIdx, splitIdx);
	if (!split)
		return false;

	const QStringList filters = {
		tr("WAV file (*.wav)"),
		tr("Raw tone data (*.dat)")
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
		tr("Export split tone"),
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
		QMessageBox::warning(parent, "", tr("Selected split has no tone data."));
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
								.end = split.loopEnd - 1u // WAV plays last sample
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
		QMessageBox::warning(parent, "", tr("Failed to export tone: %1").arg(err.what()));
		return false;
	}

	return true;
}

} // namespace tone
