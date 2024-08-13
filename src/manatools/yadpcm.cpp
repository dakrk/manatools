#include <algorithm>

#include "yadpcm.hpp"
#include "types.hpp"

// Adapted from https://github.com/superctr/adpcm/blob/master/ymz_codec.c
namespace manatools::yadpcm {

static const int stepTable[8] = {
	230, 230, 230, 230, 307, 409, 512, 614
};

static s16 step(u8 step, s16* history, s16* stepSize) {
	int sign = step & 8;
	int delta = step & 7;
	int diff = ((1 + (delta << 1)) * *stepSize) >> 3;
	int newVal = *history;
	int nStep = (stepTable[delta] * *stepSize) >> 8;

	diff = std::clamp(diff, 0, 32767);

	if (sign > 0)
		newVal -= diff;
	else
		newVal += diff;

	*stepSize = std::clamp(nStep, 127, 24576);
	*history = newVal = std::clamp(newVal, -32768, 32767);

	return newVal;
}

void encode(const s16* in, u8* out, size_t len, u8* nibble, s16* history, s16* stepSize) {
	u8 bufSample = 0;

	for (size_t i = 0; i < len; i++) {
		// we remove a few bits of accuracy to reduce some noise.
		s32 step = ((*in++) & -8) - *history;

		uint adpcmSample = (abs(step) << 16) / (*stepSize << 14);
		adpcmSample = std::clamp(adpcmSample, 0u, 7u);

		if (step < 0)
			adpcmSample |= 8;

		if (!*nibble)
			*out++ = bufSample | (adpcmSample << 4);
		else
			bufSample = adpcmSample & 15;

		*nibble ^= 1;

		yadpcm::step(adpcmSample, history, stepSize);
	}
}

size_t decode(const u8* in, s16* out, size_t len, u8* nibble, s16* history, s16* stepSize, bool highPass) {
	const u8* start = in;

	for (size_t i = 0; i < len; i++) {
		s8 step = (*(s8*)in) << (*nibble);
		step >>= 4;

		if (!(*nibble))
			in++;

		*nibble ^= 4;

		if (highPass)
			*history = (*history) * 254 / 256;

		*out++ = yadpcm::step(step, history, stepSize);
	}

	return in - start;
}

} // namespace manatools::yadpcm
