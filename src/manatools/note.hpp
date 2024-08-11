#pragma once
#include "types.hpp"

namespace manatools {
	static inline constexpr const char* noteMap[12] = {
		"C", "C#",
		"D", "D#",
		"E",
		"F", "F#",
		"G", "G#",
		"A", "A#",
		"B"
	};

	constexpr const char* noteName(u8 note) {
		return noteMap[note % 12];
	}

	constexpr int noteOctave(u8 note) {
		return (note / 12) - 2;
	}
} // namespace manatools
