#pragma once
#include <vector>

#include "filesystem.hpp"
#include "types.hpp"

namespace manatools::msb {
	constexpr u8 MSB_MAGIC[4] = {'S', 'M', 'S', 'B'};

	struct MSD {
		std::vector<u8> data;
	};

	struct MSB {
		u32 version = 2;
		std::vector<MSD> sequences;
	};

	MSB load(const fs::path& path);
} // namespace manatools::msb
