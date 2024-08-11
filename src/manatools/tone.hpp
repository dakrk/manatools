#pragma once
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include "types.hpp"

namespace manatools::tone {
	constexpr size_t MAX_SAMPLES = 65535;

	// TODO: Determine tone sample rate from MPB start/end/base note
	constexpr uint   SAMPLE_RATE = 22050; // seemingly usually.

	enum class Format {
		ADPCM,
		PCM8,
		PCM16
	};

	typedef std::vector<u8> Data;
	typedef std::shared_ptr<Data> DataPtr;

	inline u8 bitdepth(Format format) {
		using enum Format;
		switch (format) {
			case ADPCM: return  4;
			case PCM8:  return  8;
			case PCM16: return 16;
		}
		assert(!"Invalid tone format");
		return 0;
	}

	inline const char* formatName(Format format) {
		using enum Format;
		switch (format) {
			case ADPCM: return "AICA";
			case PCM8:  return "PCM8";
			case PCM16: return "PCM16LE";
		}
		assert(!"Invalid tone format");
		return "";
	}

	struct Tone {
		u8 bitdepth() const {
			return tone::bitdepth(format);
		}

		size_t samples() const {
			return data ? std::ceil((data->size() * 8) / bitdepth()) : 0;
		}

		Format format;
		DataPtr data;
	};

	void toPCM16LE(const Tone& in, std::vector<s16>& out);
} // namespace manatools::tone
