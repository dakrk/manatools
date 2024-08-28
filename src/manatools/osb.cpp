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
OSB load(const fs::path& path) {
	io::FileIO io(path, "rb");
	OSB osb;
	std::vector<u32> ptrsToneData;

	u8 magic[sizeof(OSB_MAGIC)];
	u32 fileSize;
	u32 numPrograms;

	io.readArrT(magic);
	if (memcmp(OSB_MAGIC, magic, sizeof(magic))) {
		throw std::runtime_error("Invalid OSB file");
	}

	io.readU32LE(&osb.version);
	if (osb.version != 1 && osb.version != 2) {
		throw std::runtime_error("Invalid OSB version");
	}

	io.readU32LE(&fileSize);
	io.readU32LE(&numPrograms);

	osb.programs.reserve(numPrograms);
	ptrsToneData.reserve(numPrograms);

	for (u32 p = 0; p < numPrograms; p++) {
		u32 ptrProgram;
		io.readU32LE(&ptrProgram);

		auto pos = io.tell();
		io.jump(ptrProgram);

		Program program;

		io.readArrT(magic);
		if (memcmp(OSP_MAGIC, magic, sizeof(magic))) {
			throw std::runtime_error("Encountered invalid OSB data");
		}

		u8 jumpAndBitDepth;
		io.readU8(&jumpAndBitDepth);

		u8 flags;
		io.readU8(&flags);

		if (flags & ProgramFlags::pfADPCM)    program.tone.format = tone::Format::ADPCM;
		else if ((jumpAndBitDepth >> 4) == 8) program.tone.format = tone::Format::PCM8;
		else                                  program.tone.format = tone::Format::PCM16;

		if (flags & ProgramFlags::pfLoop)     program.loop = true;

		u16 ptrToneData;
		io.readU16LE(&ptrToneData);
		program.ptrToneData_ = ptrToneData + ((jumpAndBitDepth & 0xF) * 0x10000);

		io.forward(4);

		u32 ampBitfield;
		io.readU32LE(&ampBitfield);
		program.amp.attackRate     = utils::readBits(ampBitfield,  0, 5);
		program.amp.decayRate1     = utils::readBits(ampBitfield,  6, 5); // 1 unknown bit before this
		program.amp.decayRate2     = utils::readBits(ampBitfield, 11, 5);
		program.amp.releaseRate    = utils::readBits(ampBitfield, 16, 5);
		program.amp.decayLevel     = utils::readBits(ampBitfield, 21, 5);
		program.amp.keyRateScaling = utils::readBits(ampBitfield, 26, 4); // 2 unknown bits after this

		io.forward(2);

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

		io.forward(1);

		u8 pan;
		io.readU8(&pan);
		//program.panPot = Program::calcPanPot(pan, osb.version);
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

		// TODO: More data

		ptrsToneData.push_back(program.ptrToneData_);
		osb.programs.push_back(std::move(program));

		io.jump(pos);
	}

	assert(osb.programs.size() == numPrograms);

	// Same slop from mpb.cpp
	std::sort(ptrsToneData.begin(), ptrsToneData.end());
	ptrsToneData.erase(std::unique(ptrsToneData.begin(), ptrsToneData.end()), ptrsToneData.end());

	std::map<u32, tone::DataPtr> toneDataMap;
	for (size_t i = 0; i < ptrsToneData.size(); i++) {
		u32 start = ptrsToneData[i];
		u32 end;

		if (!start)
			continue;

		// TODO: This probably has the same trailing bytes/samples issue as MPB
		if (i < ptrsToneData.size() - 1)
			end = ptrsToneData[i + 1] - 8;
		else
			end = fileSize - 12; // account for ENDB, checksum(?) and ENDD

		io.jump(start);

		// I don't think OSB shares tone data like MSB does but whatever
		auto toneData = tone::makeDataPtr(end - start);
		io.readVec(*toneData);

		toneDataMap[start] = toneData;
	}

	for (auto& program : osb.programs) {
		if (!program.ptrToneData_)
			continue;

		if (auto toneData = toneDataMap.find(program.ptrToneData_); toneData != toneDataMap.end()) {
			program.tone.data = toneData->second;
		} else {
			assert(toneData != toneDataMap.end());
		}
	}

	return osb;
}

} // namespace manatools::osb
