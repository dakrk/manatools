#include <bit>
#include <cassert>

#include "tonedecoder.hpp"
#include "yadpcm.hpp"

namespace manatools::tone {

Decoder::Decoder() : tone_(nullptr) {
	reset();
}

Decoder::Decoder(const Tone* tone) : tone_(tone) {
	reset();
}

void Decoder::reset() {
	pos = 0;
	adpcmNibble = 4;
	adpcmHistory = 0;
	adpcmStepSize = 127;
}

size_t Decoder::decode(s16* out, size_t numSamples) {
	Data* toneData = tone_->data.get();
	if (!toneData)
		return 0;

	auto toneSize = toneData->size();
	if (pos >= toneSize)
		return 0;

	using enum Format;
	switch (tone_->format) {
		case ADPCM: {
			size_t len = std::min(numSamples, (toneSize - pos) * 2);
			pos += yadpcm::decode(toneData->data() + pos, out, len, &adpcmNibble, &adpcmHistory, &adpcmStepSize);
			return len;
		}

		case PCM16: {
			size_t len = std::min(numSamples * 2, toneSize - pos);

			size_t i;
			for (i = 0; i < len; i += 2) {
				u8 low = (*toneData)[pos + i];
				u8 high = 0;

				// except what are the chances that it's not, really
				// don't even know why I'm handling cases where size isn't a multiple of 2
				if ((i + 1) < len)
					high = (*toneData)[pos + i + 1];

				// Hack for the WAV save method's own byte swapping, argh
				// TODO: revisit this
				out[i >> 1] = std::byteswap(static_cast<s16>((low << 8) | high));
			}

			pos += len;
			return i >> 1;
		}

		case PCM8: {
			size_t len = std::min(numSamples, toneSize - pos);
			for (size_t i = 0; i < len; i++) {
				out[i] = static_cast<s16>((*toneData)[pos + i] << 8);
			}
			pos += len;
			return len;
		}

		default: {
			assert(!"Invalid tone format");
			break;
		}
	}

	return 0;
}

} // namespace manatools::tone
