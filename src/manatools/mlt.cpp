#include <cassert>
#include <cstring>
#include <limits>
#include <vector>

#include "mlt.hpp"
#include "io.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace manatools::mlt {

/**
 * GCC and Clang optimises FourCC stuff to integer comparisons as it should be,
 * so this ends up being very efficient. Clang seems to be quite a lot smarter
 * than GCC when it comes to passing them. Not sure about MSVC though.
 */
s8 maxBank(FourCC type) {
	if (type == "SMDB") {
		return 1;
	} else if (type == "SFPB" || type == "SFOB" || type == "SFPW") {
		return 0;
	} else {
		/**
		 * SMSB, SMPB, SOSB and SPSR all have this limit.
		 * sdSndMemBlockAllocate in the SDK also uses this as the default,
		 * so it makes sense to do it like that here.
		 */
		return 15;
	}
}

MLT MLT::load(const fs::path& path) {
	io::FileIO io(path, "rb");
	MLT mlt;

	FourCC magic;
	u32 numUnits;

	io.readFourCC(&magic);
	if (magic != MLT_MAGIC) {
		throw std::runtime_error("Invalid MLT file");
	}

	io.readU32LE(&mlt.version);
	io.readU32LE(&numUnits);
	io.forward(20);

	for (u32 i = 0; i < numUnits; i++) {
		Unit unit;
		u32 fileDataSize;

		io.readFourCC(&unit.fourCC);
		io.readS8(&unit.bank);
		io.forward(3);
		io.readU32LE(&unit.aicaDataPtr);
		io.readU32LE(&unit.aicaDataSize);
		io.readU32LE(&unit.fileDataPtr_);
		io.readU32LE(&fileDataSize);
		io.forward(8);

		if (unit.fileDataPtr_ != UNUSED && fileDataSize != UNUSED) {
			// At least we get the data size from the get-go, unlike a certain other format
			unit.data.resize(fileDataSize);

			auto pos = io.tell();
			io.jump(unit.fileDataPtr_);
			io.readVec(unit.data);
			io.jump(pos);
		}

		mlt.units.push_back(std::move(unit));
	}

	assert(mlt.units.size() == numUnits);

	return mlt;
}

void MLT::save(const fs::path& path) {
	io::FileIO io(path, "wb");
	char err[80];

	io.writeFourCC(MLT_MAGIC);
	io.writeU32LE(version);

	if (units.size() >= std::numeric_limits<u32>::max()) {
		throw std::runtime_error("Too many units in MLT");
	}

	io.writeU32LE(units.size());

	for (int i = 0; i < 5; i++) {
		io.writeU32LE(UNUSED);
	}

	/**
	 * Seemingly there can be up to 16 of a Unit type.
	 * (MPB and MDB are treated as the same in the SDK tools though)
	 */
	std::vector<u32> unitOffsets(units.size());
	for (size_t i = 0; i < units.size(); i++) {
		auto& unit = units[i];

		if (!unit.bankInRange()) {
			snprintf(err, std::size(err), "MLT unit %zu's bank is not in range for its type (0 -> %hhd)", i, maxBank(unit.fourCC));
			throw std::runtime_error(err);
		}

		// Trust that the AICA fields are valid and aligned.
		io.writeFourCC(unit.fourCC);
		io.writeS8(unit.bank);
		io.forward(3);
		io.writeU32LE(unit.aicaDataPtr);
		io.writeU32LE(unit.aicaDataSize);

		// Write file data offset once known
		unitOffsets[i] = io.tell();
		io.writeU32LE(0);
		io.writeU32LE(0);

		if (unit.data.size() >= std::numeric_limits<u32>::max()) {
			throw std::runtime_error("MLT unit too large");
		}

		// Reserved space?
		io.writeU32LE(0);
		io.writeU32LE(0);
	}

	for (size_t i = 0; i < units.size(); i++) {
		auto& unit = units[i];
		auto pos = io.tell();

		io.jump(unitOffsets[i]);

		if (!unit.data.empty()) {
			unit.fileDataPtr_ = pos;
			io.writeU32LE(unit.fileDataPtr_);
			io.writeU32LE(unit.data.size());
			io.jump(pos);
			io.writeVec(unit.data);
		} else {
			unit.fileDataPtr_ = UNUSED;
			io.writeU32LE(unit.fileDataPtr_);
			io.writeU32LE(UNUSED);
			io.jump(pos);
		}
	}

	/**
	 * Sizes and padding are weird. Some things are padded to 32 bytes, some not.
	 * Whatever, if our output file works then it's fine. I'm not sure how required
	 * this is.
	 */
	auto pos = io.tell();
	io.writeN<u8>(0x00, utils::roundUp(pos, UNIT_ALIGN) - pos);
}

bool MLT::adjust() {
	bool dataChanged = false;
	auto& first = units[0];
	u32 origPtr = first.aicaDataPtr;
	u32 origSize = first.aicaDataSize;

	first.aicaDataPtr = utils::roundUp(std::max(first.aicaDataPtr, AICA_BASE), first.alignment());
	first.aicaDataSize = utils::roundUp(first.aicaDataSize, UNIT_ALIGN);

	if (first.aicaDataPtr != origPtr || first.aicaDataSize != origSize)
		dataChanged = true;

	for (size_t i = 1; i < units.size(); i++) {
		auto& prev = units[i - 1];
		auto& cur = units[i];
		origPtr = cur.aicaDataPtr;
		origSize = cur.aicaDataSize;

		cur.aicaDataPtr = utils::roundUp(cur.aicaDataPtr, cur.alignment());
		cur.aicaDataSize = utils::roundUp(cur.aicaDataSize, UNIT_ALIGN);

		u32 minPtr = prev.aicaDataPtr + prev.aicaDataSize;
		minPtr = utils::roundUp(minPtr, cur.alignment());

		if (cur.aicaDataPtr < minPtr)
			cur.aicaDataPtr = minPtr;

		if (cur.aicaDataPtr != origPtr || cur.aicaDataSize != origSize)
			dataChanged = true;
	}

	return dataChanged;
}

bool MLT::pack(bool useAICASizes) {
	bool dataChanged = false;
	u32 curAICAOffset = AICA_BASE;

	for (auto& unit : units) {
		u32 origPtr = unit.aicaDataPtr;
		u32 origSize;

		// Don't read the file sizes for units that don't store data in the file
		if (useAICASizes || !unit.shouldHaveData()) {
			origSize = unit.aicaDataSize;
		} else {
			origSize = unit.data.size();
		}

		unit.aicaDataPtr = utils::roundUp(curAICAOffset, unit.alignment());
		unit.aicaDataSize = utils::roundUp(origSize, UNIT_ALIGN);

		if (unit.aicaDataPtr != origPtr || unit.aicaDataSize != origSize)
			dataChanged = true;

		curAICAOffset = unit.aicaDataPtr + unit.aicaDataSize;
	}

	return dataChanged;
}

void MLT::move(size_t srcIdx, size_t count, size_t destIdx) {
	auto begin = units.begin();

	if (destIdx > srcIdx) {
		std::rotate(begin + srcIdx, begin + srcIdx + count, begin + destIdx);
	} else {
		std::rotate(begin + destIdx, begin + srcIdx, begin + srcIdx + count);
		auto it = begin + destIdx;
		auto end = it + count;

		// moving down, so we need to move down the offset in AICA RAM too
		u32 startOffset;
		if (it != begin) {
			auto prevIt = it - 1;
			startOffset = prevIt->aicaDataPtr + prevIt->aicaDataSize;
		} else {
			startOffset = AICA_BASE;
		}
		
		// all alignment related stuff can be dealt with with a subsequent `adjust` call
		while (it < end) {
			it->aicaDataPtr = startOffset;
			startOffset = it->aicaDataPtr + it->aicaDataSize;
			it++;
		}
	}
}

u32 MLT::aicaNextOffset(u32 offset) const {
	u32 nextOffset = AICA_MAX;
	for (const auto& unit : units) {
		if (unit.aicaDataPtr > offset && unit.aicaDataPtr < nextOffset) {
			nextOffset = unit.aicaDataPtr;
		}
	}
	return nextOffset;
}

uintptr_t MLT::aicaUsed() const {
	uintptr_t lastTotal = 0;

	for (const auto& unit : units) {
		uintptr_t total = unit.aicaDataPtr + unit.aicaDataSize;
		if (total > lastTotal)
			lastTotal = total;
	}

	return lastTotal;
}

/**
 * Yeah... currentBank returning negative values when an MLT bank can technically
 * store them isn't so good, but they're not valid anyway so bleh.
 */
s8 MLT::currentBank(FourCC type) const {
	s8 bank = -1;
	for (const auto& unit : units) {
		if (unit.fourCC != type)
			continue;
		if (unit.bank > bank)
			bank = unit.bank;
	}
	return bank;
}

u32 Unit::alignment() const {
	if (fourCC == FPW_MAGIC)
		return FPW_ALIGN;
	return UNIT_ALIGN;
}

bool Unit::shouldHaveData() const {
	return fourCC != FPW_MAGIC && fourCC != PSR_MAGIC;
}

} // namespace manatools::mlt
