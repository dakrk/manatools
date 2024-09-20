#include "fob.hpp"
#include "io.hpp"

namespace manatools::fob {

FOB load(const fs::path& path) {
	io::FileIO io(path, "rb");
	FOB fob;

	FourCC magic;
	u32 numMixers;

	io.readFourCC(&magic);
	if (magic != FOB_MAGIC) {
		throw std::runtime_error("Invalid FOB file");
	}

	io.readU32LE(&fob.version);
	if (fob.version != 1 && fob.version != 2) {
		throw std::runtime_error("Invalid FOB version: " + std::to_string(fob.version));
	}

	io.forward(4); // fileSize, but we don't need this
	io.readU32LE(&numMixers);

	fob.mixers.resize(numMixers);

	u32 mixerDataPos;
	io.readU32LE(&mixerDataPos);
	io.jump(mixerDataPos);

	for (auto& mixer : fob.mixers) {
		for (uint i = 0; i < CHANNELS; i++) {
			u8 pan;
			io.readU8(&mixer.level[i]);
			io.readU8(&pan);
			mixer.pan[i] = Mixer::fromPanPot(pan);
		}
	}

	return fob;
}

s8 Mixer::fromPanPot(u8 in) {
	return (in & 0x10) ? -(in & 0xF) : (in & 0xF);
}

} // namespace manatools::fob
