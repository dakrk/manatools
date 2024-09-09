#include <cassert>
#include <cstring>
#include <limits>
#include <vector>

#include "mlt.hpp"
#include "io.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace manatools::mlt {

MLT MLT::load(const fs::path& path) {
	io::FileIO io(path, "rb");
	MLT mlt;

	FourCC magic;
	u32 numUnits;

	io.readFourCC(&magic);
	if (magic != MLT_MAGIC) {
		throw std::runtime_error("Invalid MLT file");
	}

	io.forward(4);
	io.readU32LE(&numUnits);
	io.forward(20);

	for (u32 i = 0; i < numUnits; i++) {
		Unit unit;
		u32 fileDataSize;

		io.readFourCC(&unit.fourCC);
		io.readU32LE(&unit.bank);
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

	io.writeFourCC(MLT_MAGIC);
	io.writeU32LE(2); // TODO: 2 surely wouldn't always be right (assuming this is version)

	if (units.size() >= std::numeric_limits<u32>::max()) {
		throw std::runtime_error("Too many units in MLT");
	}

	io.writeU32LE(units.size());

	for (int i = 0; i < 5; i++) {
		io.writeU32LE(UNUSED);
	}

	/**
	 * Seemingly there can be up to 16 of a Unit type.
	 * (MPB and MDB are treated as the same though)
	 */
	std::vector<u32> unitOffsets(units.size());
	for (size_t i = 0; i < units.size(); i++) {
		auto& unit = units[i];

		// Trust that the AICA fields are valid and aligned
		io.writeFourCC(unit.fourCC);
		io.writeU32LE(unit.bank);
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

		if ((unit.aicaDataPtr + unit.aicaDataSize) >= AICA_MAX) {
			char err[64];
			snprintf(err, std::size(err), "MLT unit %zu exceeds available AICA RAM (>0x%x)", i, AICA_MAX);
			throw std::runtime_error(err);
		}
	}

	/**
	 * Sizes and padding are weird. Some things are padded to 32 bytes, some not.
	 * Whatever, if our output file works then it's fine. I'm not sure how required
	 * this is.
	 */
	auto pos = io.tell();
	int padding = utils::roundUp(pos, UNIT_ALIGN) - pos;
	for (int i = 0; i < padding; i++) {
		io.writeU8(0);
	}
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
		u32 origSize = useAICASizes ? unit.aicaDataSize : unit.data.size();

		unit.aicaDataPtr = utils::roundUp(curAICAOffset, unit.alignment());
		unit.aicaDataSize = utils::roundUp(origSize, UNIT_ALIGN);

		if (unit.aicaDataPtr != origPtr || unit.aicaDataSize != origSize)
			dataChanged = true;

		curAICAOffset = unit.aicaDataPtr + unit.aicaDataSize;
	}

	return dataChanged;
}

uintptr_t MLT::ramUsed() const {
	uintptr_t lastTotal = 0;

	for (const auto& unit : units) {
		uintptr_t total = unit.aicaDataPtr + unit.aicaDataSize;
		if (total > lastTotal)
			lastTotal = total;
	}

	return lastTotal;
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
