#pragma once
#include <QString>
#include <QWidget>
#include <manatools/mpb.hpp>
#include <manatools/osb.hpp>
#include <manatools/tone.hpp>
#include "common.hpp"

namespace tone {
	// TODO: Support P04, P08 and P16 formats
	enum class FileType {
		WAV, DAT
	};

	struct GUICOMMON_EXPORT Metadata {
		bool loop;
		u32 loopStart;
		u32 loopEnd;

		bool isInstrument;
		u8 startNote;
		u8 endNote;
		u8 baseNote;
		u8 startVel;
		u8 endVel;

		static Metadata fromMPB(const manatools::mpb::Split& split) {
			return {
				.loop = split.loop,
				.loopStart = split.loopStart,
				.loopEnd = split.loopEnd,

				.isInstrument = true,
				.startNote = split.startNote,
				.endNote = split.endNote,
				.baseNote = split.baseNote,
				.startVel = split.velocityLow,
				.endVel = split.velocityHigh
			};
		}

		static Metadata fromOSB(const manatools::osb::Program& program) {
			return {
				.loop = program.loop,
				.loopStart = program.loopStart,
				.loopEnd = program.loopEnd,

				.isInstrument = false,
				.startNote = 0,
				.endNote = 0,
				.baseNote = 0,
				.startVel = 0,
				.endVel = 0
			};
		}

		void toMPB(manatools::mpb::Split& split) const {
			split.loop = loop;
			split.loopStart = loopStart;
			split.loopEnd = loopEnd;
			split.startNote = startNote;
			split.endNote = endNote;
			split.baseNote = baseNote;
			split.velocityLow = startVel;
			split.velocityHigh = endVel;
		}

		void toOSB(manatools::osb::Program& program) const {
			program.loop = loop;
			program.loopStart = loopStart;
			program.loopEnd = loopEnd;
		}
	};

	using manatools::tone::Tone;

	GUICOMMON_EXPORT bool importDialog(Tone& tone, Metadata* metadata, const QString& basePath, QWidget* parent = nullptr);
	GUICOMMON_EXPORT bool importFile(Tone& tone, Metadata* metadata, const QString& path, QWidget* parent = nullptr);

	GUICOMMON_EXPORT QString exportFolderDialog(const QString& basePath, FileType* type, QWidget* parent = nullptr);
	GUICOMMON_EXPORT bool exportDialog(const Tone& tone, const Metadata* metadata, const QString& basePath,
	                                   const QString& baseName, const QString& tonePath, QWidget* parent = nullptr);
	GUICOMMON_EXPORT bool exportFile(const Tone& tone, const Metadata* metadata, const QString& path, FileType type, QWidget* parent = nullptr);

	GUICOMMON_EXPORT bool convertToADPCM(Tone& tone, QWidget* parent = nullptr);
} // namespace tone
