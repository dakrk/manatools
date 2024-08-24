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
		EndOfTrack = 0x2F,
		SetTempo   = 0x51
	};

	struct NoteOn {
		u32 delta;
		u8 channel;
		u8 note;
		u8 velocity;
	};

	struct NoteOff {
		u32 delta;
		u8 channel;
		u8 note;
		u8 velocity;
	};

	struct PolyKeyPressure {
		u32 delta;
		u8 channel;
		u8 note;
		u8 pressure;
	};

	struct ControlChange {
		u32 delta;
		u8 channel;
		u8 controller;
		u8 value;
	};

	struct ProgramChange {
		u32 delta;
		u8 channel;
		u8 program;
	};

	struct ChannelPressure {
		u32 delta;
		u8 channel;
		u8 pressure;
	};

	struct PitchWheelChange {
		u32 delta;
		u8 channel;
		u16 pitch; // TODO
	};

	struct EndOfTrack {
		u32 delta;
	};

	struct SetTempo {
		u32 delta;
		u32 tempo : 24;
	};

	using MetaEvent = std::variant<
		EndOfTrack,
		SetTempo
	>;

	using Event = std::variant<
		NoteOn,
		NoteOff,
		PolyKeyPressure,
		ControlChange,
		ProgramChange,
		ChannelPressure,
		PitchWheelChange,
		MetaEvent
	>;

	// SMF0, as Dreamcast sequences have no concept of tracks
	struct File {
		void save(io::DataIO& io);
		void save(const fs::path& path);

		// ticks per quarter-note if 15th bit is 0, otherwise SMPTE
		u16 division = 480;

		std::vector<Event> events;
	};
} // namespace manatools::midi
