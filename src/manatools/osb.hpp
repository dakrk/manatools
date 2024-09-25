#pragma once
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

		// TODO: Loop points???

		AmpEnvelope amp;

		bool lfoOn         = false;
		ProgramLFO lfo;
		ProgramFX fx;

		s8 panPot          = 0;     // [-15 -> 15]
		u8 directLevel     = 15;    // [0 -> 15]

		u8 oscillatorLevel = 255;   // [0 -> 255]

		bool filterOn      = true;
		FilterEnvelope filter;

		// TODO: There's more data that needs to be investigated (also looping??)

		u32 ptrToneData_   = 0;     // Only for presentation post-load
		tone::Tone tone;
	};

	struct Bank {
		u32 version = 2;
		std::vector<Program> programs;
	};

	Bank load(const fs::path& path);
} // namespace manatools::osb
