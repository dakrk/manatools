#pragma once
#include "types.hpp"

namespace manatools::yadpcm {
	/**
	 * The same (nibble), (history) and (stepSize) should not be used across different audio clips.
	 * (nibble) should start at 4, (history) should start at 0, and (stepSize) should start at 127.
	 */

	/**
	 * Given ADPCM samples in (in), return (len) amount of decoded PCM samples in (out).
	 * Output buffer should be at least (len * 2) elements large.
	 * 
	 * Returns number of bytes processed.
	 */
	size_t decode(const u8* in, s16* out, size_t len, u8* nibble, s16* history, s16* stepSize, bool highPass = true);
} // namespace manatools::yadpcm
