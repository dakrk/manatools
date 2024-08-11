#pragma once
#include <vector>

#include "filesystem.hpp"
#include "tone.hpp"
#include "types.hpp"

// For the most part, an OSB "Program" has the same characteristics as an MPB Split
namespace manatools::osb {
	constexpr u8 OSB_MAGIC[4] = {'S', 'O', 'S', 'B'};
	constexpr u8 OSP_MAGIC[4] = {'S', 'O', 'S', 'P'};

	enum class LFOWaveType : u8 {
		Saw      = 0,
		Square   = 1,
		Triangle = 2,
		Noise    = 3
	};

	struct AmpEnvelope {
		u8 attackRate     = 31; // [0 -> 31]
		u8 decayRate1     = 0;  // [0 -> 31]
		u8 decayRate2     = 0;  // [0 -> 31]
		u8 releaseRate    = 31; // [0 -> 31]
		u8 decayLevel     = 0;  // [0 -> 31]
		u8 keyRateScaling = 0;  // [0 -> 15]
	};

	struct FilterEnvelope {
		u8 resonance     = 0;    // [0 -> 31]

		u16 startLevel   = 8184; // [8 -> 8184]
		u16 attackLevel  = 8184; // [8 -> 8184]
		u16 decayLevel1  = 8184; // [8 -> 8184]
		u16 decayLevel2  = 8184; // [8 -> 8184]
		u16 releaseLevel = 8184; // [8 -> 8184]

		u8 decayRate1    = 25;   // [0 -> 31]
		u8 attackRate    = 25;   // [0 -> 31]
		u8 releaseRate   = 25;   // [0 -> 31]
		u8 decayRate2    = 25;   // [0 -> 31]
	};

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
		bool loop          = false;

		// TODO: Loop points???

		AmpEnvelope amp;

		bool lfoOn         = false;
		ProgramLFO lfo;
		ProgramFX fx;

		// s8 panPot          = 0;     // TODO: not quite sure yet
		u8 directLevel     = 15;    // [0 -> 15]

		u8 oscillatorLevel = 255;   // [0 -> 255]

		bool filterOn      = true;
		FilterEnvelope filter;

		// TODO: There's more data that needs to be investigated (also looping??)

		u32 ptrToneData_   = 0;     // Only for presentation post-load
		tone::Tone tone;
	};

	struct OSB {
		u32 version = 2;
		std::vector<Program> programs;
	};

	OSB load(const fs::path& path);
} // namespace manatools::osb
