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

	constexpr u16 GATE_EXT_TABLE[] = { 0x200, 0x800, 0x1000, 0x2000 };
	constexpr u16 STEP_EXT_TABLE[] = { 0x100, 0x200, 0x800, 0x1000 };

	// For statuses with ranges, the last 4 bits indicates channel
	enum class Status : u8 {
		Note             = 0x00, // 0x00 ... 0x3F
		Reference        = 0x81, // 0x81
		Loop             = 0x82, // 0x82
		EndOfTrack       = 0x83, // 0x83
		TempoChange      = 0x84, // 0x84
		GateExtend       = 0x88, // 0x88 ... 0x8B
		StepExtend       = 0x8C, // 0x8C ... 0x8F
		ControlChange    = 0xB0, // 0xB0 ... 0xBF
		ProgramChange    = 0xC0, // 0xC0 ... 0xCF
		ChannelPressure  = 0xD0, // 0xD0 ... 0xDF
		PitchWheelChange = 0xE0, // 0xE0 ... 0xEF
		SysEx            = 0xF0
	};

	struct Note {
		u8 channel = 0;
		u8 note = 0;
		u8 velocity = 0;
		u32 gate = 0;
		u32 step = 0;
	};

	struct ControlChange {
		u8 channel = 0;
		u8 controller = 0;
		u8 value = 0;
		u32 step = 0;
	};

	struct ProgramChange {
		u8 channel = 0;
		u8 program = 0;
		u32 step = 0;
	};

	struct ChannelPressure {
		u8 channel = 0;
		u8 pressure = 0;
		u32 step = 0;
	};

	struct PitchWheelChange {
		u8 channel = 0;
		s8 pitch = 0; // -64 ... 63
		u32 step = 0;
	};

	struct Loop {
		u8 unk1 = 0;
		u32 step = 0;
	};

	struct TempoChange {
		u16 tempo = 0;
		u32 step = 0;
	};

	struct SysEx {
		u32 step = 0;
		std::vector<u8> data;
		u16 stepRelated = 0;
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
		TempoChange,
		// GateExtend
		// StepExtend
		SysEx
	>;

	struct MSD {
		u32 tpqn;
		u32 initialTempo;
		std::vector<Message> messages;
	};

	MSD load(io::DataIO& io);
	MSD load(const fs::path& path);
} // namespace manatools::msd
