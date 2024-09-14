#pragma once
#include <deque>
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "types.hpp"

namespace manatools::mlt {
	constexpr FourCC MLT_MAGIC("SMLT");
	constexpr FourCC FPW_MAGIC("SFPW");
	constexpr FourCC PSR_MAGIC("SPSR");

	constexpr u32 AICA_BASE  = 0x018000;
	constexpr u32 AICA_MAX   = 0x200000;
	constexpr u32 UNUSED     = 0xFFFFFFFF;
	constexpr u32 UNIT_ALIGN = 0x20;
	constexpr u32 FPW_ALIGN  = 0x1000;

	class Unit {
	public:
		Unit() : bank(0), aicaDataPtr(0), aicaDataSize(0) {}

		Unit(FourCC fourCC, u32 bank = 0, u32 aicaDataPtr = 0, u32 aicaDataSize = 0) :
			fourCC(fourCC), bank(bank), aicaDataPtr(aicaDataPtr), aicaDataSize(aicaDataSize) {}

		Unit(FourCC fourCC, const std::vector<u8>& data, u32 bank = 0, u32 aicaDataPtr = 0, u32 aicaDataSize = 0) :
			fourCC(fourCC), bank(bank), aicaDataPtr(aicaDataPtr), aicaDataSize(aicaDataSize), data(data) {}

		FourCC fourCC;
		u32 bank;
		u32 aicaDataPtr;
		u32 aicaDataSize;
		std::vector<u8> data;

		u32 fileDataPtr() const { return fileDataPtr_; }
		u32 alignment() const;
		bool shouldHaveData() const;
	private:
		friend struct MLT;
		u32 fileDataPtr_ = UNUSED;
	};

	struct MLT {
		static MLT load(const fs::path& path);
		void save(const fs::path& path);

		bool adjust();
		bool pack(bool useAICASizes);
		void move(size_t srcIdx, size_t count, size_t destIdx);
		u32 aicaNextOffset(u32 offset) const;
		uintptr_t aicaUsed() const;

		// Version 1 isn't 0x1, instead it's 0x0101???
		u32 version = 2;
		std::deque<Unit> units;
	};

	inline MLT load(const fs::path& path) {
		return MLT::load(path);
	}
} // namespace manatools::mlt
