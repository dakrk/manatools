#include <cassert>
#include <cstring>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "mpb.hpp"
#include "io.hpp"
#include "tone.hpp"
#include "types.hpp"
#include "utils.hpp"

#define WRITEBITS(dest, src, offset, size) (dest = utils::writeBits(dest, src, offset, size))

namespace manatools::mpb {

enum SplitFlags {
	sfADPCM = 1 << 0,
	sfLoop  = 1 << 1
};

Bank load(const fs::path& path, bool guessToneSize) {
	io::FileIO io(path, "rb");
	Bank bank;
	std::map<u32, u32> tonePtrMap;

	FourCC magic;
	u32 fileSize;
	u32 ptrPrograms;
	u32 numPrograms;
	u32 ptrVelocities;
	u32 numVelocities;

	io.readFourCC(&magic);
	if (magic == MPB_MAGIC) {
		bank.drum = false;
	} else if (magic == MDB_MAGIC) {
		bank.drum = true;
	} else {
		throw std::runtime_error("Invalid MPB file");
	}

	/**
	 * It may be worth changing this to only check if the first byte (major version) is over 2?
	 * I wouldn't think minor/patch version differences to cause any breaking changes.
	 */
	io.readU32LE(&bank.version);
	if (bank.version != 0x01 && bank.version != 0x5001 && bank.version != 0x02) {
		char err[32];
		snprintf(err, std::size(err), "Invalid MPB version: %x", bank.version);
		throw std::runtime_error(err);
	}

	io.readU32LE(&fileSize);
	io.forward(4);
	io.readU32LE(&ptrPrograms);
	io.readU32LE(&numPrograms);
	io.readU32LE(&ptrVelocities);
	io.readU32LE(&numVelocities);
	io.forward(16);

	bank.programs.reserve(numPrograms);
	bank.velocities.reserve(numVelocities);

	// ============ Programs ============
	io.jump(ptrPrograms);
	for (u32 p = 0; p < numPrograms; p++) {
		Program program;

		u32 ptrProgram;
		io.readU32LE(&ptrProgram);

		auto pos = io.tell();
		io.jump(ptrProgram);

		// ============ Layers ============
		for (u32 l = 0; l < MAX_LAYERS; l++) {
			Layer layer;

			u32 ptrLayer;
			io.readU32LE(&ptrLayer);
			if (!ptrLayer)
				continue;

			auto pos = io.tell();
			io.jump(ptrLayer);

			{
				u32 numSplits;
				u32 ptrSplits;

				io.readU32LE(&numSplits);
				io.readU32LE(&ptrSplits);

				io.readU16LE(&layer.delay);
				io.readU16LE(&layer.unk1);
				io.readU8(&layer.bendRangeHigh);
				io.readU8(&layer.bendRangeLow);
				io.readU16LE(&layer.unk2);

				layer.splits.reserve(numSplits);

				auto pos = io.tell();
				io.jump(ptrSplits);

				// ============ Splits ============
				for (u32 s = 0; s < numSplits; s++) {
					Split split;

					u8 jump;
					io.readU8(&jump);

					u8 flags;
					io.readU8(&flags);
					split.unkFlags = flags & 0b11111100;

					if (flags & sfADPCM)
						split.tone.format = tone::Format::ADPCM;
					else if (jump & 0x80)
						split.tone.format = tone::Format::PCM8;
					else
						split.tone.format = tone::Format::PCM16;

					// TODO: Guess from start/end/base note and OCT/FNS
					split.tone.sampleRate = 22050;

					split.loop = flags & sfLoop;

					u16 ptrToneData;
					io.readU16LE(&ptrToneData);
					split.ptrToneData_ = ptrToneData + ((jump & 0x7F) << 16);

					io.readU16LE(&split.loopStart);
					io.readU16LE(&split.loopEnd);

					u32 ampBitfield;
					io.readU32LE(&ampBitfield);
					split.amp.attackRate     = utils::readBits(ampBitfield,  0, 5);
					split.amp.decayRate1     = utils::readBits(ampBitfield,  6, 5); // 1 unknown bit before this
					split.amp.decayRate2     = utils::readBits(ampBitfield, 11, 5);
					split.amp.releaseRate    = utils::readBits(ampBitfield, 16, 5);
					split.amp.decayLevel     = utils::readBits(ampBitfield, 21, 5);
					split.amp.keyRateScaling = utils::readBits(ampBitfield, 26, 4);
					split.amp.LPSLNK         = utils::readBits(ampBitfield, 30, 1); // 1 unknown bit after this

					u16 pitchBitfield;
					io.readU16LE(&pitchBitfield);
					split.pitch.FNS = utils::readBits(pitchBitfield, 0, 11);
					split.pitch.OCT = utils::readBits(pitchBitfield, 11, 4);

					// Convert from 4 bit signed
					if (split.pitch.OCT & 0b1000) {
						split.pitch.OCT = -(split.pitch.OCT & 0b0111);
					} else {
						split.pitch.OCT = (split.pitch.OCT & 0b0111);
					}

					u16 lfoBitfield;
					io.readU16LE(&lfoBitfield);
					split.lfo.ampDepth   = utils::readBits(lfoBitfield,  0, 3);
					split.lfo.ampWave    = static_cast<LFOWaveType>(utils::readBits(lfoBitfield,  3, 2)); // ugh
					split.lfo.pitchDepth = utils::readBits(lfoBitfield,  5, 3);
					split.lfo.pitchWave  = static_cast<LFOWaveType>(utils::readBits(lfoBitfield,  8, 2)); // ughh
					split.lfo.frequency  = utils::readBits(lfoBitfield, 10, 5);
					split.lfo.sync       = utils::readBits(lfoBitfield, 15, 1);

					u8 fxBitfield;
					io.readU8(&fxBitfield);
					split.fx.inputCh = utils::readBits(fxBitfield, 0, 4);
					split.fx.level   = utils::readBits(fxBitfield, 4, 4);

					io.readU8(&split.unk1);

					u8 pan;
					io.readU8(&pan);
					split.panPot = Split::fromPanPot(pan);
					io.readU8(&split.directLevel);

					u8 filterBitfield;
					io.readU8(&filterBitfield);
					split.filter.resonance = utils::readBits(filterBitfield, 0, 5);
					split.filter.on        = !utils::readBits(filterBitfield, 5, 1);
					split.filter.voff      = utils::readBits(filterBitfield, 6, 1);

					io.readU8(&split.oscillatorLevel);
					split.oscillatorLevel = ~split.oscillatorLevel;

					io.readU16LE(&split.filter.startLevel);
					io.readU16LE(&split.filter.attackLevel);
					io.readU16LE(&split.filter.decayLevel1);
					io.readU16LE(&split.filter.decayLevel2);
					io.readU16LE(&split.filter.releaseLevel);
					io.readU8(&split.filter.decayRate1);
					io.readU8(&split.filter.attackRate);
					io.readU8(&split.filter.releaseRate);
					io.readU8(&split.filter.decayRate2);

					io.readU8(&split.startNote);
					io.readU8(&split.endNote);
					io.readU8(&split.baseNote);
					io.readS8(&split.fineTune);

					io.readU16LE(&split.unk2);

					io.readU8(&split.velocityCurveID);
					io.readU8(&split.velocityLow);
					io.readU8(&split.velocityHigh);

					io.readBool(&split.drumMode);
					io.readU8(&split.drumGroupID);

					io.readU8(&split.unk3);

					u8 bitdepth = tone::bitdepth(split.tone.format);
					u32 endBytes = std::ceil(split.loopEnd * (bitdepth / 8.0));

					if (auto t = tonePtrMap.find(split.ptrToneData_); t != tonePtrMap.end()) {
						if (t->second < endBytes) {
							t->second = endBytes;
						}
					} else {
						tonePtrMap.insert({ split.ptrToneData_, endBytes });
					}

					layer.splits.push_back(std::move(split));
				}
				assert(layer.splits.size() == numSplits);

				io.jump(pos);
			}
			program.layers[l] = layer;

			io.jump(pos);
		}
		bank.programs.push_back(std::move(program));

		io.jump(pos);
	}

	io.jump(ptrVelocities);
	for (u32 i = 0; i < numVelocities; i++) {
		Velocity velocity;
		io.readArrT(velocity.data);
		bank.velocities.push_back(std::move(velocity));
	}

	assert(bank.programs.size() == numPrograms);
	assert(bank.velocities.size() == numVelocities);

	// TODO: Confirm checksum

	/**
	 * MPB files do not store the full size of each tone, so we have to use the fact that
	 * each tone is written consecutively to determine their length by taking the position
	 * of the tone that succeeds the current one, and subtract it to get the length.
	 * 
	 * TODO: Oops, seems like they're not always written consecutively. See issue #20.
	 * As a reliable workaround for this, there exists the option to determine the tone
	 * size from the biggest loopEnd seen set by a split using it.
	 */
	std::map<u32, tone::DataPtr> toneDataMap;
	for (auto it = tonePtrMap.begin(); it != tonePtrMap.end(); it++) {
		u32 start = it->first;
		size_t size;

		if (!start)
			continue;

		/**
		 * Oddly, 16 bit PCM audio seems to have 2 extra bytes (1 sample) at the end, being a copy
		 * of the first two?
		 * A similar issue also seems to happen with ADPCM too (with seemingly usually 6 samples),
		 * and presumably also 8 bit PCM.
		 * This can cause clicks at the end of the audio.
		 * You can hear this if you try and play 0:0:2 from Rez S_M05 at its intended pitch.
		 * 
		 * TODO: Investigate trailing bytes/samples (probably just some SDK tool quirk)
		 */
		if (guessToneSize) {
			auto it2 = std::next(it);
			if (it2 != tonePtrMap.end()) {
				size = it2->first - start;
			} else {
				size = ((bank.version & 0xFF) >= 2 ? fileSize - 8 : fileSize - 4) - start;
			}
		} else {
			size = it->second;
		}

		io.jump(start);
		auto toneData = tone::makeDataPtr(size);
		io.readVec(*toneData);
		toneDataMap.insert({ start, toneData });
	}

	for (auto& program : bank.programs) {
		for (auto& layer : program.layers) {
			if (!layer)
				continue;

			for (auto& split : layer->splits) {
				if (!split.ptrToneData_)
					continue;

				if (auto toneData = toneDataMap.find(split.ptrToneData_); toneData != toneDataMap.end()) {
					split.tone.data = toneData->second;
				} else {
					assert(toneData != toneDataMap.end());
				}
			}
		}
	}

	// And the copy *should* be elided
	return bank;
}

void Bank::save(const fs::path& path) {
	io::DynBufIO::VecType outBuf;
	io::DynBufIO io(outBuf);

	// ============ Start header ============

	io.writeFourCC(drum ? MDB_MAGIC : MPB_MAGIC);
	io.writeU32LE(version);

	auto fileSizePos = io.tell();
	io.writeU32LE(0);
	io.writeU32LE(0); // unknown

	auto ptrProgramPtrsPos = io.tell();
	io.writeU32LE(0);

	if (programs.size() >= MAX_PROGRAMS)
		throw std::runtime_error("Too many programs in MPB");
	io.writeU32LE(programs.size());

	auto ptrVelocitiesPos = io.tell();
	io.writeU32LE(0);

	if (velocities.size() >= MAX_VELOCITIES)
		throw std::runtime_error("Too many velocities in MPB");
	io.writeU32LE(velocities.size());

	auto ptrUnk1Pos = io.tell();
	io.writeU32LE(0);
	io.writeU32LE(1); // numUnk1

	auto ptrUnk2Pos = io.tell();
	io.writeU32LE(0);
	io.writeU32LE(1); // numUnk2

	// ============ End header ============

	/**
	 * Write an empty array, which shall later contain offsets to each program
	 * once they are known.
	 */
	auto programPtrsPos = io.tell();
	io.jump(ptrProgramPtrsPos);
	io.writeU32LE(programPtrsPos);
	io.jump(programPtrsPos);
	for (size_t i = 0; i < programs.size(); i++) {
		io.writeU32LE(0);
	}

	// Write an array containing each velocity curve, which are 128 bytes each
	auto velocitiesPos = io.tell();
	io.jump(ptrVelocitiesPos);
	io.writeU32LE(velocitiesPos);
	io.jump(velocitiesPos);
	for (const auto& velocity : velocities) {
		io.writeArrT(velocity.data);
	}

	/**
	 * I have zero clue what these are and they always seem to be the same
	 * almost-null data but I still feel I'm required to write them properly.
	 * TODO: Copy data from loaded file in case it differs?
	 */
	auto unk1Pos = io.tell();
	io.jump(ptrUnk1Pos);
	io.writeU32LE(unk1Pos);
	io.jump(unk1Pos);
	io.writeU32LE(0x00800000);
	io.writeU32LE(0x00800080);
	io.writeU32LE(0x00000080);

	auto unk2Pos = io.tell();
	io.jump(ptrUnk2Pos);
	io.writeU32LE(unk2Pos);
	io.jump(unk2Pos);
	io.writeU32LE(0);

	std::vector<std::vector<u32>> splitPtrs(programs.size());

	for (size_t p = 0; p < programs.size(); p++) {
		const auto& program = programs[p];

		// Write the position of this program to the program pointer array
		auto programPos = io.tell();
		io.jump(programPtrsPos + (p * sizeof(u32)));
		io.writeU32LE(programPos);
		io.jump(programPos);

		// Write an empty layer pointer array
		for (size_t i = 0; i < program.layers.size(); i++) {
			io.writeU32LE(0);
		}

		/**
		 * 8 unknown (seemingly unused) bytes
		 * TODO: copy these from loaded file too?
		 */
		for (int i = 0; i < 2; i++) {
			io.writeU32LE(0);
		}

		std::vector<u32> ptrSplitsPtrs(program.layers.size());

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];

			// Write the position of this layer to the program's layer pointer array
			auto layerPos = io.tell();
			io.jump(programPos + (l * sizeof(u32)));
			io.writeU32LE(layer ? layerPos : 0);
			io.jump(layerPos);

			if (!layer)
				continue;

			if (layer->splits.size() >= MAX_SPLITS) {
				char err[64];
				snprintf(err, std::size(err), "Too many splits in MPB program layer %zu:%zu", p, l);
				throw std::runtime_error(err);
			}

			io.writeU32LE(layer->splits.size());

			ptrSplitsPtrs[l] = io.tell();
			io.writeU32LE(0);

			io.writeU16LE(layer->delay);
			io.writeU16LE(layer->unk1);
			io.writeU8(layer->bendRangeHigh);
			io.writeU8(layer->bendRangeLow);
			io.writeU16LE(layer->unk2);
		}

		splitPtrs[p].resize(program.layers.size());

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];

			if (!layer)
				continue;

			assert(ptrSplitsPtrs[l]);

			// Write the position of where this layer's split data starts
			auto splitsPos = io.tell();
			io.jump(ptrSplitsPtrs[l]);
			io.writeU32LE(splitsPos);
			io.jump(splitsPos);

			splitPtrs[p][l] = splitsPos;

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];

				// The only unknown flags I've seen in the wild are the 2 MSBs
				u8 flags = split.unkFlags & 0b11111100;

				if (split.loop)
					flags |= sfLoop;

				if (split.tone.format == tone::Format::ADPCM)
					flags |= sfADPCM;

				io.writeU8(0); // jump (filled in later)
				io.writeU8(flags);
				io.writeU16LE(0); // ptrToneData (filled in later)

				io.writeU16LE(split.loopStart);
				io.writeU16LE(split.loopEnd);

				u32 ampBits = 0;
				WRITEBITS(ampBits, split.amp.attackRate,      0, 5);
				WRITEBITS(ampBits, split.amp.decayRate1,      6, 5);
				WRITEBITS(ampBits, split.amp.decayRate2,     11, 5);
				WRITEBITS(ampBits, split.amp.releaseRate,    16, 5);
				WRITEBITS(ampBits, split.amp.decayLevel,     21, 5);
				WRITEBITS(ampBits, split.amp.keyRateScaling, 26, 4);
				WRITEBITS(ampBits, split.amp.LPSLNK,         30, 1);
				io.writeU32LE(ampBits);

				u16 pitchBits = 0;
				WRITEBITS(pitchBits, split.pitch.FNS, 0, 11);

				u8 pitchOCT = split.pitch.OCT & 0b0111;
				if (split.pitch.OCT < 0) {
					pitchOCT |= 0b1000;
				}

				WRITEBITS(pitchBits, pitchOCT, 11, 4);
				io.writeU16LE(pitchBits);

				u16 lfoBits = 0;
				WRITEBITS(lfoBits, split.lfo.ampDepth,   0, 3);
				WRITEBITS(lfoBits, static_cast<u8>(split.lfo.ampWave), 3, 2);
				WRITEBITS(lfoBits, split.lfo.pitchDepth, 5, 3);
				WRITEBITS(lfoBits, static_cast<u8>(split.lfo.pitchWave), 8, 2);
				WRITEBITS(lfoBits, split.lfo.frequency, 10, 5);
				WRITEBITS(lfoBits, split.lfo.sync,      15, 1);
				io.writeU16LE(lfoBits);

				u8 fxBits = 0;
				WRITEBITS(fxBits, split.fx.inputCh, 0, 4);
				WRITEBITS(fxBits, split.fx.level,   4, 4);
				io.writeU8(fxBits);

				io.writeU8(split.unk1);

				io.writeU8(Split::toPanPot(split.panPot, version));
				io.writeU8(split.directLevel);

				u8 filterBits = 0;
				WRITEBITS(filterBits, split.filter.resonance, 0, 5);
				WRITEBITS(filterBits, !split.filter.on,       5, 1);
				WRITEBITS(filterBits, split.filter.voff,      6, 1);
				io.writeU8(filterBits);

				io.writeU8(~split.oscillatorLevel);

				io.writeU16LE(split.filter.startLevel);
				io.writeU16LE(split.filter.attackLevel);
				io.writeU16LE(split.filter.decayLevel1);
				io.writeU16LE(split.filter.decayLevel2);
				io.writeU16LE(split.filter.releaseLevel);
				io.writeU8(split.filter.decayRate1);
				io.writeU8(split.filter.attackRate);
				io.writeU8(split.filter.releaseRate);
				io.writeU8(split.filter.decayRate2);

				io.writeU8(split.startNote);
				io.writeU8(split.endNote);
				io.writeU8(split.baseNote);
				io.writeS8(split.fineTune);

				io.writeU16LE(split.unk2);

				io.writeU8(split.velocityCurveID);
				io.writeU8(split.velocityLow);
				io.writeU8(split.velocityHigh);

				io.writeBool(split.drumMode);
				io.writeU8(split.drumGroupID);

				io.writeU8(split.unk3);
			}
		}
	}

	/**
	 * Finally, we can write each split's tone data as everything else has finished being written,
	 * as tone data should ideally be written consecutively.
	 */
	std::unordered_map<tone::DataPtr, u32> tonePtrs;
	for (size_t p = 0; p < programs.size(); p++) {
		const auto& program = programs[p];

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];

			if (!layer)
				continue;

			assert(splitPtrs[p][l]);

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];
				const auto& toneData = split.tone.data;

				if (!toneData)
					continue;

				if (!tonePtrs.contains(toneData)) {
					if (split.tone.samples() >= tone::MAX_SAMPLES) {
						char err[96];
						snprintf(err, std::size(err), "Too many samples (>%zu) in MPB tone %zu:%zu:%zu", tone::MAX_SAMPLES, p, l, s);
						throw std::runtime_error(err);
					}
					
					/**
					 * OSB requires padding, presumably because Manatee requires the magic to be
					 * aligned to read it, but MPB doesn't have data magic so I wouldn't think that
					 * to be applicable, and after inspecting numerous files this seems to be true.
					 */
					tonePtrs[toneData] = io.tell();
					io.writeVec(*toneData);
				}

				auto tonePos = tonePtrs[toneData];
				u8 jump = (tonePos >> 16) & 0x7F;
				if (split.tone.format == tone::Format::PCM8)
					jump |= 0x80;

				auto pos = io.tell();
				io.jump(splitPtrs[p][l]);
				io.forward(48 * s);              // jump to current split, where each split is 48 bytes, ugh
				io.writeU8(jump);                // jump
				io.forward(1);                   // don't need to rewrite flags
				io.writeU16LE(tonePos & 0xFFFF); // ptrToneData
				io.jump(pos);
			}
		}
	}

	auto endPos = io.tell();
	io.jump(fileSizePos);
	io.writeU32LE((version & 0xFF) >= 2 ? endPos + 8 : endPos + 4);
	io.jump(endPos);

	// Version 1 does not store a checksum
	if ((version & 0xFF) >= 2) {
		u32 checksum = 0;
		for (long i = 4; i < endPos; i++) {
			checksum += io.vec()[i];
		}

		io.writeU32LE(checksum);
	}

	io.writeFourCC(MPB_END);

	// Almost same situation as MLT, I hope this is right though
	auto pos = io.tell();
	io.writeN<u8>(0x00, utils::roundUp(pos, 32) - pos);

	// Finally write the file
	io::FileIO file(path, "wb");
	file.writeVec(io.vec());
}

const Program* Bank::program(size_t programIdx) const {
	if (programIdx < programs.size())
		return &programs[programIdx];

	return nullptr;
}

const Layer* Bank::layer(size_t programIdx, size_t layerIdx) const {
	if (auto* p = program(programIdx)) {
		if (layerIdx >= MAX_LAYERS)
			return nullptr;
		if (auto& l = p->layers[layerIdx]; l.has_value())
			return &(*l);
	}

	return nullptr;
}

const Split* Bank::split(size_t programIdx, size_t layerIdx, size_t splitIdx) const {
	if (auto* l = layer(programIdx, layerIdx))
		if (splitIdx < l->splits.size())
			return &l->splits[splitIdx];

	return nullptr;
}

// Do a little dance in order to use these in both const and non-const contexts
Program* Bank::program(size_t programIdx) {
	return const_cast<Program*>(std::as_const(*this).program(programIdx));
}

Layer* Bank::layer(size_t programIdx, size_t layerIdx) {
	return const_cast<Layer*>(std::as_const(*this).layer(programIdx, layerIdx));
}

Split* Bank::split(size_t programIdx, size_t layerIdx, size_t splitIdx) {
	return const_cast<Split*>(std::as_const(*this).split(programIdx, layerIdx, splitIdx));
}

uint Program::usedLayers() const {
	uint used = 0;
	for (const auto& layer : layers) {
		if (layer)
			used++;
	}
	return used;
}

/**
 * Despite being version 2, Segagaga's MPBs use version 1 panning.
 * I guess the driver doesn't care and the fact panning is usually different
 * between versions is possibly just a quirk of the widely used SDK tools.
 */
s8 Split::fromPanPot(u8 in) {
	return (in & 0x10) ? -(in & 0xF) : (in & 0xF);
}

u8 Split::toPanPot(s8 in, u32 version) {
	if (version == 2 || version == 0x5001)
		return in >= 0 ? in + 32 : 16 - in;
	else if (version == 1)
		return in >= 0 ? in : 16 - in;
	return 0;
}

Velocity Velocity::defaultCurve() {
	Velocity vel;
	for (uint i = 0; i < std::size(vel.data); i++) {
		vel.data[i] = i;
	}
	return vel;
}

} // namespace manatools::mpb
