#pragma once
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "types.hpp"

namespace manatools::msb {
	constexpr FourCC MSB_MAGIC("SMSB");

	struct MSD {
		std::vector<u8> data;
	};

	struct MSB {
		u32 version = 2;
		std::vector<MSD> sequences;
	};

	MSB load(const fs::path& path);
} // namespace manatools::msb
