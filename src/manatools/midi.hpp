#pragma once
#include <variant>
#include <vector>

#include "filesystem.hpp"
#include "io.hpp"
#include "types.hpp"

namespace manatools::midi {
	constexpr u8 HEADER_MAGIC[4] = {'M', 'T', 'h', 'd'};
	constexpr u8 TRACK_MAGIC[4] = {'M', 'T', 'r', 'k'};

	enum class Status : u8 {
		NoteOff          = 0x80, // 0x80 ... 0x8F
		NoteOn           = 0x90, // 0x90 ... 0x9F
		PolyKeyPressure  = 0xA0, // 0xA0 ... 0xAF
		ControlChange    = 0xB0, // 0xB0 ... 0xBF
		ProgramChange    = 0xC0, // 0xC0 ... 0xCF
		ChannelPressure  = 0xD0, // 0xD0 ... 0xDF
		PitchWheelChange = 0xE0, // 0xE0 ... 0xEF
		// Nothing else between this point matters for us
		MetaEvent        = 0xFF, // 0xFF
	};

	enum class MetaEvents : u8 {

	};

	struct NoteOn {
		u8 delta;
		u8 channel;
		u8 note;
		u8 velocity;
	};

	struct NoteOff {
		u8 delta;
		u8 channel;
		u8 note;
		u8 velocity;
	};

	using Event = std::variant<
		NoteOn,
		NoteOff
	>;

	// SMF0, as Dreamcast sequences have no concept of tracks
	struct File {
		void save(io::DataIO& io);
		void save(const fs::path& path);

		std::vector<Event> events;
	};
} // namespace manatools::midi
