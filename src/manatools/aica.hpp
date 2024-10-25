#pragma once
#include <algorithm>
#include "types.hpp"
#include "common.hpp"

namespace manatools::aica {
	constexpr uint SAMPLE_RATE = 44100;

	/**
	 * Table just took from AICA_E.HTM, not sure the actual calculation for this.
	 * This maps effective rate values to milliseconds.
	 */
	inline constexpr double AEGAttackTime[64] = {
		-1, -1, 8100.0, 6900.0, 6000.0, 4800.0, 4000.0, 3400.0, 3000.0, 2400.0,
		2000.0, 1700.0, 1500.0, 1200.0, 1000.0,  860.0,  760.0,  600.0,  500.0,
		 430.0,  380.0,  300.0,  250.0,  220.0,  190.0,  150.0,  130.0,  110.0,
		  95.0,   76.0,   63.0,   55.0,   47.0,   38.0,   31.0,   27.0,   24.0,
		  19.0,   15.0,   13.0,   12.0,    9.4,    7.9,    6.8,    6.0,    4.7,
		   3.8,    3.4,    3.0,    2.4,    2.0,    1.8,    1.6,    1.3,    1.1,
		   0.93,   0.85,   0.65,   0.53,   0.44,   0.40,   0.35,   0.0,    0.0
	};

	// Perhaps stuff from common.hpp would make more sense moved into aica.hpp...
	inline u8 calcEffectiveRate(u8 KRS, common::PitchRegs pitch, u8 rate) {
		/**
		 * Corlett's docs uses a conditional here. Confused about why the false
		 * branch. It also clamps to 60 instead of 63.
		 * Original docs use `(KRS + OCT) * 2 + FNS + rate * 2`.
		 */
		s32 e;
		if (KRS < 0xF) {
			e = (KRS + pitch.OCT + rate) * 2 + pitch.FNS;
		} else {
			e = rate * 2;
		}
		return std::clamp(e, 0, 63);
	}
} // namespace manatools::aica
