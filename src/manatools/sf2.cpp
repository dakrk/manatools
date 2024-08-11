#include <cmath>
#include <string>

#include "sf2.hpp"
#include "filesystem.hpp"
#include "mpb.hpp"
#include "tonedecoder.hpp"
#include "utils.hpp"

using namespace sf2cute;

namespace manatools::sf2 {

SoundFont fromMPB(const mpb::Bank& mpb, const std::string& bankName) {
	SoundFont sf2;

	sf2.set_sound_engine("EMU8000");
	sf2.set_bank_name(bankName);
	sf2.set_product("Sega Dreamcast");
	sf2.set_software("manatools");

	for (size_t p = 0; p < mpb.programs.size(); p++) {
		const auto& program = mpb.programs[p];

		auto preset = sf2.NewPreset(std::to_string(p + 1), static_cast<u16>(p), 0);

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];
			if (!layer)
				continue;

			auto instrument = sf2.NewInstrument(preset->name() + ":" += std::to_string(l + 1));

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];
				std::vector<s16> sampleData;

				if (split.tone.data) {
					tone::Decoder decoder(&split.tone);
					sampleData.resize(split.tone.samples());
					decoder.decode(sampleData);
				}

				auto sample = sf2.NewSample(
					instrument->name() + ":" += std::to_string(s + 1),
					sampleData,
					split.loopStart,
					split.loopEnd,
					tone::SAMPLE_RATE,
					split.baseNote - 12, // 1 octave down
					0 // TODO: correction/fine tune
				);

				// TODO: sf2cute outputs velRange as 127-1 instead of 1-127 on big endian, report a bug
				std::vector<SFGeneratorItem> generatorItems = {
					{ SFGenerator::kKeyRange, RangesType(split.startNote, split.endNote) },
					{ SFGenerator::kVelRange, RangesType(split.velocityLow, split.velocityHigh) },
					{ SFGenerator::kPan, static_cast<s16>(roundf(utils::remap(split.panPot, -15, 15, -500, 500))) }
				};

				if (split.loop)
					generatorItems.emplace_back(SFGenerator::kSampleModes, u16(SampleMode::kLoopContinuously));

				/**
				 * TODO: Envelopes, LFO, direct level, all that stuff
				 * 
				 * I don't think SF2 is a fully compatible format with MPB. Not only am I not sure what the MPB
				 * envelope values would map to at the moment, stuff like decayRate2 doesn't seem to have an SF2
				 * equivalent. directLevel could be doable though, however from what I see there doesn't seem to be
				 * some readily available thing to use for that.
				 *
				 * DLS seems to be a much more compatible format, especially because of the drum mode stuff mentioned
				 * below. I wish to aim to support it in the future, but unfortunately hardly as many things support
				 * it and it seems like quite a bit of effort to implement.
				 * 
				 * (A while after writing this I discovered libgig which can read/write DLS files and also *load* SF2s,
				 * which is something sf2cute can't do, however for some reason it still uses SVN in the year 2024 so
				 * would require me making my own Git mirror, gah.)
				 */

				/**
				 * TODO: Drum mode & group ID
				 * According to the DLS 1 spec, if note-on is received while things with the same key group are active,
				 * note-off should be issued to the others.
				 * However, we're outputting an SF2, not a DLS, so I'm not sure if there's an equivalent.
				 */

				instrument->AddZone({ sample, std::move(generatorItems), {} });
			}

			preset->AddZone(SFPresetZone(instrument));
		}
	}

	return sf2;
}

} // namespace manatools::sf2
