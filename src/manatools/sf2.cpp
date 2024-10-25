#include <cmath>
#include <string>

#include "aica.hpp"
#include "sf2.hpp"
#include "mpb.hpp"
#include "tonedecoder.hpp"
#include "utils.hpp"

using namespace sf2cute;

namespace manatools::sf2 {

/**
 * TODO: Mend inaccuracies
 * A direct level of 0 should give no output, but this doesn't do that.
 * (In that case that's probably a good thing because we can't do FX.)
 * Corlett's documentation says every 0x10 in TL is 3 dB, but the AICA_E.HTM
 * table says 3 dB is 0x8?
 * SF2 spec says this is in centibels, which is a tenth of a decibel, yet
 * Polyphone has "real dB" which they convert to by (10 * sourceDB / 0.4)?
 */
static s16 calcAttenuation(u8 directLevel, u8 oscLevel) {
	s16 a = (15 - directLevel) * 3;
	a += (oscLevel / 16) * 3;
	return 10 * a;
}

static double timecentClamp(double v, bool high) {
	return std::clamp<double>(v, -12000, high ? 8000 : 5000);
}

static s16 msecsToTimecent(double msecs, bool high) {
	return std::roundf(timecentClamp(1200 * std::log2(msecs / 1000), high));
}

SoundFont fromMPB(const mpb::Bank& mpb, const std::string& bankName) {
	SoundFont sf2;

	sf2.set_sound_engine("EMU8000");
	sf2.set_bank_name(bankName);
	sf2.set_product("Sega Dreamcast");
	sf2.set_software("manatools");

	for (size_t p = 0; p < mpb.programs.size(); p++) {
		const auto& program = mpb.programs[p];

		auto preset = sf2.NewPreset(std::to_string(p), static_cast<u16>(p), 0);

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];
			if (!layer)
				continue;

			auto instrument = sf2.NewInstrument(preset->name() + ":" += std::to_string(l));

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];
				std::vector<s16> sampleData;

				if (split.tone.data) {
					tone::Decoder decoder(&split.tone);
					sampleData.resize(split.tone.samples());
					decoder.decode(sampleData);
				}

				// not 100% sure about the exact accuracy of the fine tune
				auto sample = sf2.NewSample(
					instrument->name() + ":" += std::to_string(s),
					sampleData,
					split.loopStart,
					split.loopEnd,
					aica::SAMPLE_RATE,
					split.baseNote,
					static_cast<s8>(roundf(utils::remap(split.fineTune, -128, 127, -48, 47)))
				);

				u8 ampAtkEffRate = split.effectiveRate(split.amp.attackRate);
				u8 ampRelEffRate = split.effectiveRate(split.amp.releaseRate);

				std::vector<SFGeneratorItem> generatorItems = {
					{ SFGenerator::kKeyRange, RangesType(split.startNote, split.endNote) },
					{ SFGenerator::kVelRange, RangesType(split.velocityLow, split.velocityHigh) },
					{ SFGenerator::kPan, static_cast<s16>(roundf(utils::remap(split.panPot, -15, 15, -500, 500))) },
					{ SFGenerator::kInitialAttenuation, calcAttenuation(split.directLevel, ~split.oscillatorLevel) },
					{ SFGenerator::kDelayVolEnv, msecsToTimecent(layer->delay * 4, false) },
					{ SFGenerator::kAttackVolEnv, msecsToTimecent(aica::AEGAttackTime[ampAtkEffRate], true) },
					{ SFGenerator::kReleaseVolEnv, msecsToTimecent(aica::AEGDSRTime[ampRelEffRate], true) }
				};

				if (split.loop)
					generatorItems.emplace_back(SFGenerator::kSampleModes, u16(SampleMode::kLoopContinuously));

				if (split.drumMode)
					generatorItems.emplace_back(SFGenerator::kExclusiveClass, split.drumGroupID);

				/**
				 * TODO: Envelopes, LFO, direct level, all that stuff
				 * 
				 * I don't think SF2 is a fully compatible format with MPB. Not only am I not sure what the MPB
				 * envelope values would map to at the moment, stuff like decayRate2 doesn't seem to have an SF2
				 * equivalent. directLevel could be doable though, however from what I see there doesn't seem to be
				 * some readily available thing to use for that.
				 *
				 * DLS seems to be a much more compatible format.
				 * I wish to aim to support it in the future, but unfortunately hardly as many things support it and it
				 * seems like quite a bit of effort to implement.
				 * 
				 * (A while after writing this I discovered libgig which can read/write DLS files and also *load* SF2s,
				 * which is something sf2cute can't do, however for some reason it still uses SVN in the year 2024 so
				 * would require me making my own Git mirror, gah.)
				 */

				instrument->AddZone({ sample, std::move(generatorItems), {} });
			}

			preset->AddZone(SFPresetZone(instrument));
		}
	}

	return sf2;
}

} // namespace manatools::sf2
