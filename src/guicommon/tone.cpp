#include <QFileDialog>
#include <QMessageBox>
#include <guicommon/CursorOverride.hpp>
#include <manatools/tone.hpp>
#include <manatools/tonedecoder.hpp>
#include <manatools/wav.hpp>
#include <sndfile.hh>

#include "tone.hpp"
#include "InstDataDialog.hpp"

// we're not in a class, and can't make aliases to a static method with default args
constexpr auto tr = [](const char* sourceText, const char* disambiguation = nullptr, int n = -1) {
	return QObject::tr(sourceText, disambiguation, n);
};

// well, libsndfile doesn't have a "read_s16" precisely
static_assert(sizeof(short) == 2, "Size of short is not 2 bytes/16 bits");

namespace tone {

constexpr const size_t READ_SIZE = 512;

bool importDialog(manatools::mpb::Split& split, const QString& basePath, QWidget* parent) {
	const QString path = QFileDialog::getOpenFileName(
		parent,
		tr("Import tone"),
		basePath
	);

	if (path.isEmpty())
		return false;

	return importFile(split, path, parent);
}

bool importFile(manatools::mpb::Split& split, const QString& path, QWidget* parent) {
	CursorOverride cursor(Qt::WaitCursor);

	#ifdef _WIN32
	SndfileHandle sndFile(path.toStdWString().c_str());
	#else
	SndfileHandle sndFile(path.toStdString());
	#endif

	/**
	 * well... for `!sndFile` it wouldn't even be able to get a message because
	 * it would be a null handle
	 */
	if (!sndFile || sndFile.error() != SF_ERR_NO_ERROR) {
		cursor.restore();
		QMessageBox::warning(parent, tr("Import tone"), tr("Failed to import sound file: %1").arg(sndFile.strError()));
		return false;
	}

	// have to use cmp_greater due to dumb signedness stuff
	if (std::cmp_greater(sndFile.frames(), manatools::tone::MAX_SAMPLES)) {
		cursor.restore();
		QMessageBox::warning(
			parent,
			tr("Import tone"),
			tr(
				"Cannot use imported sound file as it contains more than %1 samples (%2).\n\n"
				"Tip: Try reducing its sample rate."
			).arg(manatools::tone::MAX_SAMPLES).arg(sndFile.frames())
		);
		return false;
	}

	// TODO: Channel select dialog, as well as option to downmix multiple
	auto channels = sndFile.channels();
	if (channels > 1) {
		QMessageBox::warning(
			parent,
			tr("Import tone"),
			tr("More than 1 channel was found in the imported file. Only channel 1 will be read.")
		);
	}

	// Otherwise reading floats would result in the output just being zeros
	sndFile.command(SFC_SET_SCALE_FLOAT_INT_READ, nullptr, true);

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
		QMessageBox::warning(parent, tr("Import tone"), tr("Could not fully read audio data from imported sound file."));
		return false;
	}

	SF_INSTRUMENT instrument;
	if (sndFile.command(SFC_GET_INSTRUMENT, &instrument, sizeof(instrument))) {
		if (instrument.loop_count >= 1) {
			if (instrument.loop_count > 1)
				QMessageBox::warning(parent, tr("Import tone"), tr("Imported sound file has more than one loop. Using first loop."));

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

bool exportDialog(const manatools::mpb::Split& split, const QString& basePath, const QString& baseName,
                  const QString& tonePath, QWidget* parent)
{
	const QStringList filters = {
		tr("WAV file (*.wav)"),
		tr("Raw tone data (*.dat)")
	};

	QString selectedFilter;

	const QString bankName = baseName.isEmpty() ? "untitled" : baseName;

	const QString defPath = QDir(basePath).filePath(
		tonePath.isEmpty() ? bankName : QString("%1_%2.wav").arg(bankName).arg(tonePath)		
	);

	const QString path = QFileDialog::getSaveFileName(
		parent,
		tr("Export tone"),
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

	return exportFile(split, path, type, parent);
}

bool exportFile(const manatools::mpb::Split& split, const QString& path, FileType type, QWidget* parent) {
	CursorOverride cursor(Qt::WaitCursor);

	if (!split.tone.data) {
		cursor.restore();
		QMessageBox::warning(parent, tr("Export tone"), tr("Selected split has no tone data."));
		return false;
	}

	try {
		switch (type) {
			case FileType::WAV: {
				// TODO: hmm. could probably replace this with libsndfile
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
				
				wav.save(path.toStdWString());
				break;
			}

			case FileType::DAT: {
				manatools::io::FileIO file(path.toStdWString(), "wb");
				file.writeVec(*split.tone.data);
				break;
			}

			default: {
				return false;
			}
		}
	} catch (const std::runtime_error& err) {
		cursor.restore();
		QMessageBox::warning(parent, tr("Export tone"), tr("Failed to export tone: %1").arg(err.what()));
		return false;
	}

	return true;
}

bool convertToADPCM(manatools::tone::Tone& tone, QWidget* parent) {
	if (!tone.data)
		return false;

	using enum manatools::tone::Format;
	switch (tone.format) {
		case ADPCM:
			return true;

		case PCM16: {
			if (tone.data->size() % 2) {
				QMessageBox::warning(parent, tr("Convert to ADPCM"), tr("Cannot convert tone to ADPCM: Size of PCM-16 data must be a multiple of 2 bytes."));
				return false;
			}

			manatools::tone::Tone newTone;
			newTone.format = manatools::tone::Format::ADPCM,
			// really not sure about this whole divide and ceil stuff
			newTone.data = manatools::tone::makeDataPtr(std::ceil(tone.samples() / 2.));

			manatools::yadpcm::Context adpcmCtx;
			adpcmCtx.encode(reinterpret_cast<s16*>(tone.data->data()), newTone.data->data(), tone.samples());

			tone = newTone;
			break;
		}

		case PCM8: {
			/**
			 * TODO: PCM8 to PCM16, which can then be passed to yadpcm::encode
			 * (PCM8 in the switch case could then possibly be moved above PCM16 as to fallthrough
			 * to reduce duplicate code)
			 */
			return false;
		}
	}

	return true;
}

} // namespace tone
