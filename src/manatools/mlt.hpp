#pragma once
#include <vector>

#include "filesystem.hpp"
#include "types.hpp"

namespace manatools::mlt {
	constexpr u8 MLT_MAGIC[4] = {'S', 'M', 'L', 'T'};
	constexpr u32 AICA_BASE = 0x018000;
	constexpr u32 AICA_MAX  = 0x200000;
	constexpr u32 UNUSED    = 0xFFFFFFFF;

	struct Unit {
		char fourCC[5]{};
		u32 bank;
		u32 aicaDataPtr;
		u32 aicaDataSize;
		u32 fileDataPtr() const { return fileDataPtr_; }
		std::vector<u8> data;

	private:
		friend struct MLT;
		u32 fileDataPtr_;
	};

	struct MLT {
		static MLT load(const fs::path& path);
		void save(const fs::path& path);

		void align();
		void pack(bool useAICASizes);

		std::vector<Unit> units;
	};

	inline MLT load(const fs::path& path) {
		return MLT::load(path);
	}
} // namespace manatools::mlt
