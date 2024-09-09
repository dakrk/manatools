#pragma once
#include <variant>
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "io.hpp"
#include "types.hpp"

/**
 * TODO: MSB contains a version field, while an MSD doesn't...
 * Is MSD version based off of its parent MSB? *Are* there multiple MSD versions?
 */
namespace manatools::msd {
	constexpr FourCC MSD_MAGIC("SMSD");

	// For statuses with ranges, the last 4 bits indicates channel
	enum class Status : u8 {
		Note             = 0x00, // 0x00 ... 0x3F
		Reference        = 0x81, // 0x81
		Loop             = 0x82, // 0x82
		EndOfTrack       = 0x83, // 0x83
		TempoChange      = 0x84, // 0x84
		ControlChange    = 0xB0, // 0xB0 ... 0xBF
		ProgramChange    = 0xC0, // 0xC0 ... 0xCF
		ChannelPressure  = 0xD0, // 0xD0 ... 0xDF
		PitchWheelChange = 0xE0, // 0xE0 ... 0xEF
	};

	struct Note {
		u8 channel = 0;
		u8 note = 0;
		u8 velocity = 0;
		u16 gate = 0;
		u16 step = 0;
	};

	struct ControlChange {
		u8 channel = 0;
		u8 controller = 0;
		u8 value = 0;
		u16 step = 0;
	};

	struct ProgramChange {
		u8 channel = 0;
		u8 program = 0;
		u16 step = 0;
	};

	struct ChannelPressure {
		u8 channel = 0;
		u8 pressure = 0;
		u16 step = 0;
	};

	struct PitchWheelChange {
		u8 channel = 0;
		s8 pitch = 0; // -64 ... 63
		u16 step = 0;
	};

	struct Loop {
		u8 unk1;
		u16 step;
	};

	struct TempoChange {
		u16 tempo;
		u8 unk1;
	};

	using Message = std::variant<
		Note,
		ControlChange,
		ProgramChange,
		ChannelPressure,
		PitchWheelChange,

		// Reference,
		Loop,
		// EndOfTrack
		TempoChange
	>;

	struct MSD {
		u32 tpqn;
		u32 initialTempo;
		std::vector<Message> messages;
	};

	MSD load(io::DataIO& io);
	MSD load(const fs::path& path);
} // namespace manatools::msd
