#include <cstring>

#include "msb.hpp"
#include "io.hpp"
#include "types.hpp"

namespace manatools::msb {

MSB load(const fs::path& path) {
	io::FileIO io(path, "rb");
	MSB msb;
	std::vector<u32> ptrsSeqData;

	FourCC magic;
	u32 fileSize;
	u32 numSequences;

	io.readFourCC(&magic);
	if (magic != MSB_MAGIC) {
		throw std::runtime_error("Invalid MSB file");
	}

	io.readU32LE(&msb.version);
	if (msb.version != 1 && msb.version != 2) {
		throw std::runtime_error("Invalid MSB version: " + std::to_string(msb.version));
	}

	io.readU32LE(&fileSize);
	io.readU32LE(&numSequences);

	msb.sequences.resize(numSequences);
	ptrsSeqData.resize(numSequences);

	for (u32 i = 0; i < numSequences; i++) {
		io.readU32LE(&ptrsSeqData[i]);
	}

	for (size_t i = 0; i < ptrsSeqData.size(); i++) {
		u32 start = ptrsSeqData[i];
		u32 end;

		// not sure if MSB accepts nullptr like what other files do, but do this for completeness
		if (!start)
			continue;

		if (i < ptrsSeqData.size() - 1)
			end = ptrsSeqData[i + 1];
		else
			end = fileSize - 8; // account for ENDB and checksum(?)

		io.jump(start);
		msb.sequences[i].data.resize(end - start);
		io.readVec(msb.sequences[i].data);
	}

	return msb;
}

} // namespace manatools::msb
