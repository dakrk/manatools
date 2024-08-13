#pragma once
#include "types.hpp"

namespace manatools::yadpcm {
	/**
	 * The same (nibble), (history) and (stepSize) should not be used across different audio clips.
	 * (history) should start at 0, and (stepSize) should start at 127.
	 */

	/**
	 * Given (len) amount of 16-bit PCM samples in (in), return ADPCM samples in (out).
	 * Output buffer should be at least (len / 2) elements large.
	 * 
	 * (nibble) should start at 0.
	 * 
	 * TODO: Return value? Can't stream otherwise because of dumb 4-bit ADPCM stuff
	 */
	void encode(const s16* in, u8* out, size_t len, u8* nibble, s16* history, s16* stepSize);

	/**
	 * Given ADPCM samples in (in), return (len) amount of decoded 16-bit PCM samples in (out).
	 * Output buffer should be at least (len * 2) elements large.
	 * 
	 * (nibble) should start at 4.
	 * 
	 * Returns number of bytes processed.
	 */
	size_t decode(const u8* in, s16* out, size_t len, u8* nibble, s16* history, s16* stepSize, bool highPass = true);
} // namespace manatools::yadpcm
