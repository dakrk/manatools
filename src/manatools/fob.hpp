#pragma once
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "types.hpp"

namespace manatools::fob {
	constexpr FourCC FOB_MAGIC("SFOB");
	constexpr u32 CHANNELS = 16;

	struct Mixer {
		static s8 fromPanPot(u8 panPot);
		u8 level[CHANNELS];
		s8 pan[CHANNELS];
	};

	struct FOB {
		u32 version = 2;
		std::vector<Mixer> mixers;
	};

	FOB load(const fs::path& path);
} // namespace manatools::fob
