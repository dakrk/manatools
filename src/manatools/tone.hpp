#pragma once
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include "types.hpp"

namespace manatools::tone {
	/**
	 * TODO: Cannot be 65535, as formats use that to indicate "unused", or similar.
	 * However, because of the whole MPB trailing bytes thing, 65534 sample tones
	 * can read to above that.
	 */
	constexpr size_t MAX_SAMPLES = 65535;

	// TODO: Determine tone sample rate from MPB start/end/base note
	constexpr uint SAMPLE_RATE = 22050; // seemingly usually.

	enum class Format {
		ADPCM,
		PCM8,
		PCM16
	};

	typedef std::vector<u8> Data;
	typedef std::shared_ptr<Data> DataPtr;

	inline DataPtr makeDataPtr(size_t bytes) {
		return std::make_shared<Data>(bytes);
	}

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

		// TODO: With an ADPCM sample being able to take half a byte, should I really be doing this?
		size_t samples() const {
			return data ? std::ceil((data->size() * 8.) / bitdepth()) : 0;
		}

		Format format{Format::PCM16};
		DataPtr data;
	};
} // namespace manatools::tone
