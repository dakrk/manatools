#pragma once
#include "types.hpp"

namespace manatools::yadpcm {
	// The same context should not be used across different audio clips.
	class Context {
	public:
		/**
		 * Given (len) amount of 16-bit PCM samples in (in), return ADPCM samples in (out).
		 * Output buffer should be at least (len / 2) elements large.
		 * 
		 * Returns number of bytes output.
		 */
		size_t encode(const s16* in, u8* out, size_t len);

		/**
		 * Given ADPCM samples in (in), return (len) amount of decoded 16-bit PCM samples in (out).
		 * Output buffer should be at least (len * 2) elements large.
		 * 
		 * Returns number of bytes processed.
		 */
		size_t decode(const u8* in, s16* out, size_t len, bool highPass = true);

		void reset() {
			nibble    = 4;
			history   = 0;
			stepSize  = 127;
			bufSample = 0;
		}

		u8 nibble    = 4;
		s16 history  = 0;
		s16 stepSize = 127;
		u8 bufSample = 0;
	};
} // namespace manatools::yadpcm
