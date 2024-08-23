#pragma once
#include <variant>
#include <vector>

#include "filesystem.hpp"
#include "io.hpp"
#include "types.hpp"

/**
 * TODO: MSB contains a version field, while an MSD doesn't...
 * Is MSD version based off of its parent MSB? *Are* there multiple MSD versions?
 */
namespace manatools::msd {
	constexpr u8 MSD_MAGIC[4] = {'S', 'M', 'S', 'D'};

	// For statuses with ranges, the last 4 bits indicates channel
	enum class Status : u8 {
		Note            = 0x00, // 0x00 ... 0x3F
		Reference       = 0x81, // 0x81
		Loop            = 0x82, // 0x82
		EndOfSequence   = 0x83, // 0x83
		TempoChange     = 0x84, // 0x84
		ControlChange   = 0xB0, // 0xB0 ... 0xBF
		ProgramChange   = 0xC0, // 0xC0 ... 0xCF
		ChannelPressure = 0xD0, // 0xD0 ... 0xDF
	};

	enum class Controller : u8 {
		BankSelect_MSB          = 0,
		ModulationWheel_MSB     = 1,
		BreathController_MSB    = 2,
		Undefined3_MSB          = 3,
		FootController_MSB      = 4, // pedal?
		PortamentoTime_MSB      = 5,
		DataEntry_MSB           = 6,
		Volume_MSB              = 7,
		Balance_MSB             = 8,
		Undefined9_MSB          = 9,
		Pan_MSB                 = 10,
		Expression_MSB          = 11,
		EffectController1_MSB   = 12, // ignored in Dreamcast?
		EffectController2_MSB   = 13, // ignored in Dreamcast?
		Undefined14_MSB         = 14,
		Undefined15_MSB         = 15,
		GeneralPurpose16_MSB    = 16,
		GeneralPurpose17_MSB    = 17,
		GeneralPurpose18_MSB    = 18,
		GeneralPurpose19_MSB    = 19,
		Undefined20_MSB         = 20, // used in Dreamcast?
		Undefined21_MSB         = 21, // used in Dreamcast?
		Undefined22_MSB         = 22, // used in Dreamcast?
		Undefined23_MSB         = 23, // used in Dreamcast?
		Undefined24_MSB         = 24, // used in Dreamcast?
		Undefined25_MSB         = 25, // used in Dreamcast?
		Undefined26_MSB         = 26, // used in Dreamcast?
		Undefined27_MSB         = 27, // used in Dreamcast?
		Undefined28_MSB         = 28, // used in Dreamcast?
		Undefined29_MSB         = 29, // used in Dreamcast?
		Undefined30_MSB         = 30, // used in Dreamcast?
		Undefined31_MSB         = 31, // used in Dreamcast?

		BankSelect_LSB          = 32, // important!
		ModulationWheel_LSB     = 33,
		BreathController_LSB    = 34,
		Undefined35_LSB         = 35,
		FootController_LSB      = 36, // pedal?
		PortamentoTime_LSB      = 37,
		DataEntry_LSB           = 38,
		Volume_LSB              = 39,
		Balance_LSB             = 40,
		Undefined41_LSB         = 41,
		Pan_LSB                 = 42,
		Expression_LSB          = 43,
		EffectController1_LSB   = 44, // ignored in Dreamcast?
		EffectController2_LSB   = 45, // ignored in Dreamcast?
		Undefined46_LSB         = 46,
		Undefined47_LSB         = 47,
		GeneralPurpose48_LSB    = 48, // used in Dreamcast?
		GeneralPurpose49_LSB    = 49, // used in Dreamcast?
		GeneralPurpose50_LSB    = 50, // used in Dreamcast?
		GeneralPurpose51_LSB    = 51, // used in Dreamcast?
		Undefined52_LSB         = 52, // used in Dreamcast?
		Undefined53_LSB         = 53, // used in Dreamcast?
		Undefined54_LSB         = 54, // used in Dreamcast?
		Undefined55_LSB         = 55, // used in Dreamcast?
		Undefined56_LSB         = 56, // used in Dreamcast?
		Undefined57_LSB         = 57, // used in Dreamcast?
		Undefined58_LSB         = 58, // used in Dreamcast?
		Undefined59_LSB         = 59, // used in Dreamcast?
		Undefined60_LSB         = 60, // used in Dreamcast?
		Undefined61_LSB         = 61, // used in Dreamcast?
		Undefined62_LSB         = 62, // used in Dreamcast?
		Undefined63_LSB         = 63, // used in Dreamcast?

		// TODO: There's a lot more

		ResetAllControllers     = 121
	};

	struct Note {
		u8 channel = 0;
		u8 key = 0;
		u8 velocity = 0;
		u16 gate = 0;
		u16 step = 0;
	};

	// Bank Select (MSB) is index 0, so good default to make GCC not warn
	struct ControlChange {
		u8 channel = 0;
		Controller controller = Controller::BankSelect_MSB;
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

	struct Loop {
		u8 mode;
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

		// Reference,
		Loop,
		// EndOfSequence
		TempoChange
	>;

	struct MSD {
		u32 unk1;
		u32 unk2;
		std::vector<Message> messages;
	};

	MSD load(io::DataIO& io);
	MSD load(const fs::path& path);
} // namespace manatools::msd
