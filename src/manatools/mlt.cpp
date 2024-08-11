#include <cassert>
#include <cstring>
#include <limits>
#include <vector>

#include "mlt.hpp"
#include "io.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace manatools::mlt {

MLT load(const fs::path& path) {
	io::FileIO io(path, "rb");
	MLT mlt;

	u8 magic[sizeof(MLT_MAGIC)];
	u32 numUnits;

	io.readArrT(magic);
	if (memcmp(MLT_MAGIC, magic, sizeof(magic))) {
		throw std::runtime_error("Invalid MLT file");
	}

	io.forward(4);
	io.readU32LE(&numUnits);
	io.forward(20);

	mlt.units.reserve(numUnits);

	for (u32 i = 0; i < numUnits; i++) {
		Unit unit;
		u32 fileDataSize;

		io.readArrT(unit.fourCC);
		io.readU32LE(&unit.bank);
		io.readU32LE(&unit.aicaDataPtr_);
		io.readU32LE(&unit.aicaDataSize_);
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
	io::FileIO io(path, "rb");

	io.writeArrT(MLT_MAGIC);
	io.writeU32LE(2); // TODO: 2 surely wouldn't always be right (assuming this is version)

	if (units.size() >= std::numeric_limits<u32>::max()) {
		throw std::length_error("Too many units in MLT");
	}

	io.writeU32LE(units.size());

	for (int i = 0; i < 5; i++) {
		io.writeU32LE(UNUSED);
	}

	std::vector<u32> unitOffsets(units.size());
	u32 curAICAOffset = AICA_BASE;

	/**
	 * Seemingly there can be up to 16 of a Unit type.
	 * (MPB and MDB are treated as the same though)
	 */

	for (size_t i = 0; i < units.size(); i++) {
		auto& unit = units[i];

		io.writeArrT(unit.fourCC);
		io.writeU32LE(unit.bank);

		// Pushed to a vector so offsets can be written later once they're known
		unitOffsets[i] = io.tell();
		io.writeU32LE(0); // AICA Data Offset
		io.writeU32LE(0); // AICA Data Size
		io.writeU32LE(0); // File Data Offset

		if (unit.data.size() >= std::numeric_limits<u32>::max()) {
			throw std::length_error("MLT unit too large");
		}

		io.writeU32LE(unit.data.size());

		// Reserved space?
		for (int p = 0; p < 2; p++) {
			io.writeU32LE(0);
		}
	}

	for (size_t i = 0; i < units.size(); i++) {
		auto& unit = units[i];

		unit.aicaDataPtr_ = curAICAOffset;
		unit.aicaDataSize_ = unit.data.size();
		unit.fileDataPtr_ = io.tell();

		io.jump(unitOffsets[i]);
		io.writeU32LE(unit.aicaDataPtr_);
		io.writeU32LE(unit.aicaDataSize_);
		io.writeU32LE(unit.fileDataPtr_);
		io.jump(unit.fileDataPtr_);
		io.writeVec(unit.data);

		curAICAOffset += unit.aicaDataSize_;
		if (curAICAOffset >= AICA_MAX) {
			throw std::length_error("MLT unit exceeds available AICA RAM");
		}
	}

	/**
	 * Sizes and padding are weird. Some things are padded to 32 bytes, some not.
	 * Whatever, if our output file works then it's fine. I'm not sure how required
	 * this is.
	 */
	auto pos = io.tell();
	int padding = utils::roundUp(pos, 32l) - pos;
	for (int i = 0; i < padding; i++) {
		io.writeU8(0);
	}
}

} // namespace manatools::mlt
