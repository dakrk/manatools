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

	inline constexpr double AEGDSRTime[64] = {
		-1, -1, 118200.0, 101300.0, 88600.0, 70900.0, 59100.0, 50700.0,
		44300.0, 35500.0,  29600.0, 25300.0, 22200.0, 17700.0, 14800.0,
		12700.0, 11100.0,   8900.0,  7400.0,  6300.0,  5500.0,  4400.0,
		 3700.0,  3200.0,   2800.0,  2200.0,  1800.0,  1600.0,  1400.0,
		 1100.0,   920.0,    790.0,   690.0,   550.0,   460.0,   390.0,
		  340.0,   270.0,    230.0,   200.0,   170.0,   140.0,   110.0,
		   98.0,    85.0,     68.0,    57.0,    49.0,    43.0,    34.0,
		   28.0,    25.0,     22.0,    18.0,    14.0,    12.0,    11.0,
		    8.5,     7.1,      6.1,     5.4,     4.3,     3.6,     3.1
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
