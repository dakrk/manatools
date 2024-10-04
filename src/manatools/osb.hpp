#pragma once
#include <any>
#include <vector>

#include "common.hpp"
#include "filesystem.hpp"
#include "fourcc.hpp"
#include "tone.hpp"
#include "types.hpp"

// For the most part, an OSB "Program" has the same characteristics as an MPB Split
namespace manatools::osb {
	constexpr FourCC OSB_MAGIC("SOSB");
	constexpr FourCC OSP_MAGIC("SOSP");
	constexpr FourCC OSD_MAGIC("SOSD");
	constexpr FourCC OSB_END("ENDB");
	constexpr FourCC OSP_END("ENDP");
	constexpr FourCC OSD_END("ENDD");

	using common::LFOWaveType;
	using common::AmpEnvelope;
	using common::FilterEnvelope;

	struct ProgramLFO {
		u8 ampDepth   = 0; // [0 -> 7]
		LFOWaveType ampWave   = LFOWaveType::Saw;

		u8 pitchDepth = 0; // [0 -> 7]
		LFOWaveType pitchWave = LFOWaveType::Saw;

		u8 frequency  = 0; // [0 -> 31]
	};

	struct ProgramFX {
		u8 inputCh = 0; // [0 -> 15]
		u8 level   = 0; // [0 -> 15]
	};

	// Well, I can assume they're called programs. The FourCC ends with 'P'.
	struct Program {
		static s8 fromPanPot(u8 panPot);
		static u8 toPanPot(s8 panPot);

		u8 unkFlags        = 0;

		bool loop          = false;
		u16 loopStart      = 0;     // [0 -> 65535]
		u16 loopEnd        = 0;     // [0 -> 65535]

		AmpEnvelope amp;

		u16 unk1           = 0;     // Unknown

		bool lfoOn         = false;
		ProgramLFO lfo;
		ProgramFX fx;

		u8 unk2            = 0;     // Unknown

		s8 panPot          = 0;     // [-15 -> 15]
		u8 directLevel     = 15;    // [0 -> 15]

		u8 oscillatorLevel = 255;   // [0 -> 255]

		bool filterOn      = true;
		FilterEnvelope filter;

		u32 loopTime       = 0;     // On version 1, only 1 or 2 bytes, and values unknown
		u8 baseNote        = 60;    // [0 -> 127]
		u8 freqAdjust      = 0;     // Unknown, different between versions

		// TODO: There's more data that needs to be investigated

		u32 ptrToneData_   = 0;     // Only for presentation post-load
		tone::Tone tone;
		std::any userData;
	};

	struct Bank {
		void save(const fs::path& path);
		u32 version = 2;
		std::vector<Program> programs;
	};

	Bank load(const fs::path& path, bool guessToneSize = true);
} // namespace manatools::osb
