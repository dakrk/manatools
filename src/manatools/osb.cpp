#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <stdexcept>

#include "osb.hpp"
#include "io.hpp"
#include "tone.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace manatools::osb {

enum ProgramFlags {
	pfADPCM = 1 << 0,
	pfLoop  = 1 << 1
};

// Pretty much just copied from mpb.cpp
Bank load(const fs::path& path, bool guessToneSize) {
	io::FileIO io(path, "rb");
	Bank bank;
	std::map<u32, u32> tonePtrMap;

	FourCC magic;
	u32 fileSize;
	u32 numPrograms;

	io.readFourCC(&magic);
	if (magic != OSB_MAGIC) {
		throw std::runtime_error("Invalid OSB file");
	}

	io.readU32LE(&bank.version);
	if (bank.version != 1 && bank.version != 2) {
		throw std::runtime_error("Invalid OSB version");
	}

	io.readU32LE(&fileSize);
	io.readU32LE(&numPrograms);

	bank.programs.reserve(numPrograms);

	for (u32 p = 0; p < numPrograms; p++) {
		u32 ptrProgram;
		io.readU32LE(&ptrProgram);

		auto pos = io.tell();
		io.jump(ptrProgram);

		Program program;

		io.readFourCC(&magic);
		if (magic != OSP_MAGIC) {
			throw std::runtime_error("Encountered invalid OSB data");
		}

		u8 jump;
		io.readU8(&jump);

		u8 flags;
		io.readU8(&flags);
		program.unkFlags = flags & 0b11111100;

		if (flags & pfADPCM)
			program.tone.format = tone::Format::ADPCM;
		else if (jump & 0x80)
			program.tone.format = tone::Format::PCM8;
		else
			program.tone.format = tone::Format::PCM16;

		program.loop = flags & pfLoop;

		u16 ptrToneData;
		io.readU16LE(&ptrToneData);
		program.ptrToneData_ = ptrToneData + ((jump & 0x7F) << 16);

		io.readU16LE(&program.loopStart);
		io.readU16LE(&program.loopEnd);

		u32 ampBitfield;
		io.readU32LE(&ampBitfield);
		program.amp.attackRate     = utils::readBits(ampBitfield,  0, 5);
		program.amp.decayRate1     = utils::readBits(ampBitfield,  6, 5); // 1 unknown bit before this
		program.amp.decayRate2     = utils::readBits(ampBitfield, 11, 5);
		program.amp.releaseRate    = utils::readBits(ampBitfield, 16, 5);
		program.amp.decayLevel     = utils::readBits(ampBitfield, 21, 5);
		program.amp.keyRateScaling = utils::readBits(ampBitfield, 26, 4); // 2 unknown bits after this

		io.readU16LE(&program.unk1);

		u16 lfoBitfield;
		io.readU16LE(&lfoBitfield);
		program.lfo.ampDepth   = utils::readBits(lfoBitfield,  0, 3);
		program.lfo.ampWave    = static_cast<LFOWaveType>(utils::readBits(lfoBitfield,  3, 2)); // ugh
		program.lfo.pitchDepth = utils::readBits(lfoBitfield,  5, 3);
		program.lfo.pitchWave  = static_cast<LFOWaveType>(utils::readBits(lfoBitfield,  8, 2)); // ughh
		program.lfo.frequency  = utils::readBits(lfoBitfield, 10, 5);
		program.lfoOn          = utils::readBits(lfoBitfield, 15, 1);

		u8 fxBitfield;
		io.readU8(&fxBitfield);
		program.fx.inputCh = utils::readBits(fxBitfield, 0, 4);
		program.fx.level   = utils::readBits(fxBitfield, 4, 4);

		io.readU8(&program.unk2);

		u8 pan;
		io.readU8(&pan);
		program.panPot = Program::fromPanPot(pan);
		io.readU8(&program.directLevel);

		u8 filterBitfield;
		io.readU8(&filterBitfield);
		program.filter.resonance = utils::readBits(filterBitfield, 0, 5);
		program.filterOn         = utils::readBits(filterBitfield, 5, 1);

		io.readU8(&program.oscillatorLevel);
		program.oscillatorLevel = ~program.oscillatorLevel;

		io.readU16LE(&program.filter.startLevel);
		io.readU16LE(&program.filter.attackLevel);
		io.readU16LE(&program.filter.decayLevel1);
		io.readU16LE(&program.filter.decayLevel2);
		io.readU16LE(&program.filter.releaseLevel);
		io.readU8(&program.filter.decayRate1);
		io.readU8(&program.filter.attackRate);
		io.readU8(&program.filter.releaseRate);
		io.readU8(&program.filter.decayRate2);

		io.readU32LE(&program.loopTime);
		io.readU8(&program.baseNote);

		u8 bitdepth = tone::bitdepth(program.tone.format);
		u32 endBytes = std::ceil(program.loopEnd * (bitdepth / 8.0));

		if (auto t = tonePtrMap.find(program.ptrToneData_); t != tonePtrMap.end()) {
			if (t->second < endBytes) {
				t->second = endBytes;
			}
		} else {
			tonePtrMap.insert({ program.ptrToneData_, endBytes });
		}

		bank.programs.push_back(std::move(program));

		io.jump(pos);
	}

	assert(bank.programs.size() == numPrograms);

	/**
	 * This process is pretty useless for OSB, as their loopEnds should always
	 * be the full size of the tone data, mainly because the SDK tools didn't
	 * allow you to change loop start or end.
	 */
	std::map<u32, tone::DataPtr> toneDataMap;
	for (auto it = tonePtrMap.begin(); it != tonePtrMap.end(); it++) {
		u32 start = it->first;
		size_t size;

		if (!start)
			continue;

		// Subtract 8 to account for contiguous footer then header
		if (guessToneSize) {
			auto it2 = std::next(it);
			if (it2 != tonePtrMap.end()) {
				size = (it2->first - start) - 8;
			} else {
				size = (bank.version >= 2 ? fileSize - 12 : fileSize - 8) - start;
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
		if (!program.ptrToneData_)
			continue;

		if (auto toneData = toneDataMap.find(program.ptrToneData_); toneData != toneDataMap.end()) {
			program.tone.data = toneData->second;
		} else {
			assert(toneData != toneDataMap.end());
		}
	}

	return bank;
}

s8 Program::fromPanPot(u8 in) {
	return in >= 16 ? 15 - (s8)in : (s8)in;
}

u8 Program::toPanPot(s8 in) {
	return in >= 0 ? in : 15 - in;
}

} // namespace manatools::osb
