#pragma once
#include <vector>

#include "filesystem.hpp"
#include "types.hpp"

namespace manatools::mlt {
	constexpr u8 MLT_MAGIC[4] = {'S', 'M', 'L', 'T'};
	constexpr u32 AICA_BASE = 0x018000;
	constexpr u32 AICA_MAX  = 0x200000;
	constexpr u32 UNUSED    = 0xFFFFFFFF;

	/**
	 * TODO: Allow setting AICA data size and making file offset/size
	 * UNUSED for FX Program Work (FPW) as it's only there to specify
	 * how much RAM to allocate to FX
	 */
	struct Unit {
		u8 fourCC[4];
		u32 bank;
		u32 aicaDataPtr_;
		u32 aicaDataSize_;
		u32 fileDataPtr_;
		std::vector<u8> data;
	};

	struct MLT {
		void save(const fs::path& path);

		std::vector<Unit> units;
	};

	MLT load(const fs::path& path);
} // namespace manatools::mlt
