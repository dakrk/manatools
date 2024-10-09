#pragma once
#include <any>
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "types.hpp"

namespace manatools::fob {
	constexpr FourCC FOB_MAGIC("SFOB");
	constexpr FourCC FOB_END("ENDB");
	constexpr u32 CHANNELS = 16;

	struct Mixer {
		static s8 fromPanPot(u8 panPot);
		static u8 toPanPot(s8 panPot);
		u8 level[CHANNELS];
		s8 pan[CHANNELS];
		std::any userData;
	};

	struct Bank {
		void save(const fs::path& path);
		u32 version = 2;
		std::vector<Mixer> mixers;
	};

	Bank load(const fs::path& path);
} // namespace manatools::fob
