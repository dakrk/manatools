#pragma once
#include <span>

#include "tone.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace manatools::tone {
	/**
	 * TODO: Allow taking in loop points.
	 * (Not quite sure how I'd approach that with ADPCM yet)
	 */
	class Decoder {
	public:
		Decoder();
		Decoder(const Tone* tone);

		void reset();

		const Tone* tone() const {
			return tone_;
		}

		void setTone(const Tone* tone) {
			tone_ = tone;
			reset();
		}

		// Returns number of samples read
		size_t decode(s16* out, size_t numSamples);
		size_t decode(std::span<s16> out) {
			return decode(out.data(), out.size());
		}

	private:
		MT_DISABLE_COPY(Decoder)

		const Tone* tone_;

		size_t pos;
		u8 adpcmNibble;
		s16 adpcmHistory;
		s16 adpcmStepSize;
	};
} // namespace manatools::tone
