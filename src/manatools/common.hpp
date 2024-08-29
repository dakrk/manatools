#pragma once
#include "types.hpp"

namespace manatools::common {
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
		// Default is 4 in MPB, 0 in OSB.
		u8 resonance     = 4;    // [0 -> 31]

		// Minimums for these are 0 in MPB, 8 in OSB.
		u16 startLevel   = 8184; // [0 -> 8184]
		u16 attackLevel  = 8184; // [0 -> 8184]
		u16 decayLevel1  = 8184; // [0 -> 8184]
		u16 decayLevel2  = 8184; // [0 -> 8184]
		u16 releaseLevel = 8184; // [0 -> 8184]

		u8 decayRate1    = 25;   // [0 -> 31]
		u8 attackRate    = 25;   // [0 -> 31]
		u8 releaseRate   = 25;   // [0 -> 31]
		u8 decayRate2    = 25;   // [0 -> 31]
	};
} // namespace manatools::common
