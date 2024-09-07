#pragma once
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "types.hpp"

namespace manatools::mlt {
	constexpr FourCC MLT_MAGIC("SMLT");
	constexpr FourCC FPW_MAGIC("SFPW");

	constexpr u32 AICA_BASE  = 0x018000;
	constexpr u32 AICA_MAX   = 0x200000;
	constexpr u32 UNUSED     = 0xFFFFFFFF;
	constexpr u32 UNIT_ALIGN = 0x20;
	constexpr u32 FPW_ALIGN  = 0x1000;

	struct Unit {
		FourCC fourCC;
		u32 bank;
		u32 aicaDataPtr;
		u32 aicaDataSize;
		u32 fileDataPtr() const { return fileDataPtr_; }
		u32 alignment() const;
		std::vector<u8> data;

	private:
		friend struct MLT;
		u32 fileDataPtr_;
	};

	struct MLT {
		static MLT load(const fs::path& path);
		void save(const fs::path& path);

		bool adjust();
		bool pack(bool useAICASizes);

		std::vector<Unit> units;
	};

	inline MLT load(const fs::path& path) {
		return MLT::load(path);
	}
} // namespace manatools::mlt
