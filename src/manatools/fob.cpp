#include "fob.hpp"
#include "io.hpp"

namespace manatools::fob {

Bank load(const fs::path& path) {
	io::FileIO io(path, "rb");
	Bank bank;

	FourCC magic;
	u32 numMixers;

	io.readFourCC(&magic);
	if (magic != FOB_MAGIC) {
		throw std::runtime_error("Invalid FOB file");
	}

	io.readU32LE(&bank.version);
	if (bank.version != 1 && bank.version != 2) {
		throw std::runtime_error("Invalid FOB version: " + std::to_string(bank.version));
	}

	io.forward(4); // fileSize, but we don't need this
	io.readU32LE(&numMixers);

	bank.mixers.resize(numMixers);

	u32 mixerDataPos;
	io.readU32LE(&mixerDataPos);
	io.jump(mixerDataPos);

	for (auto& mixer : bank.mixers) {
		for (uint i = 0; i < CHANNELS; i++) {
			u8 pan;
			io.readU8(&mixer.level[i]);
			io.readU8(&pan);
			mixer.pan[i] = Mixer::fromPanPot(pan);
		}
	}

	return bank;
}

void Bank::save(const fs::path& path) {
	io::DynBufIO::VecType outBuf;
	io::DynBufIO io(outBuf);

	io.writeFourCC(FOB_MAGIC);
	io.writeU32LE(version);

	auto fileSizePos = io.tell();
	io.writeU32LE(0);
	io.writeU32LE(mixers.size());
	io.writeU32LE(io.tell() + 8); // mixer data offset
	io.writeU32LE(0); // unknown/padding?

	for (auto& mixer : mixers) {
		for (uint i = 0; i < CHANNELS; i++) {
			io.writeU8(mixer.level[i]);
			io.writeU8(Mixer::toPanPot(mixer.pan[i]));
		}
	}

	auto endPos = io.tell();
	io.jump(fileSizePos);
	io.writeU32LE(version >= 2 ? endPos + 8 : endPos + 4);
	io.jump(endPos);

	if (version >= 2) {
		u32 checksum = 0;
		for (long i = 4; i < endPos; i++) {
			checksum += io.vec()[i];
		}

		io.writeU32LE(checksum);
	}

	io.writeFourCC(FOB_END);

	io::FileIO file(path, "wb");
	file.writeVec(io.vec());
}

s8 Mixer::fromPanPot(u8 in) {
	return (in & 0x10) ? -(in & 0xF) : (in & 0xF);
}

u8 Mixer::toPanPot(s8 in) {
	return in >= 0 ? in : 16 - in;
}

} // namespace manatools::fob
