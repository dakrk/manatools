#pragma once
#include <algorithm>
#include <concepts>

#include "types.hpp"

#define MT_DISABLE_COPY(cls) \
	cls(const cls&) = delete; \
	cls& operator=(const cls&) = delete;

#define MT_DISABLE_MOVE(cls) \
	cls(cls&&) = delete; \
	cls& operator=(cls&&) = delete;

namespace manatools::utils {
	template <std::integral T>
	constexpr T roundUp(T num, T mul) {
		if (mul == 0)
			return num;

		int rem = num % mul;
		if (rem == 0)
			return num;

		return num + mul - rem;
	}

	template <std::integral DT, std::integral ST>
	DT writeBits(DT dest, ST src, u8 offset, u8 size) {
		DT mask = (1 << size) - 1;
		dest &= ~(mask << offset);
		dest |= std::clamp(DT(src), DT(0), mask) << offset;
		return dest;
	}

	constexpr float remap(float val, float imin, float imax, float omin, float omax) {
		return omin + ((omax - omin) / (imax - imin)) * (val - imin);
	}
} // namespace manatools::utils
