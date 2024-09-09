#pragma once
#include <string>
#include <variant>
#include <vector>

#include "filesystem.hpp"
#include "fourcc.hpp"
#include "io.hpp"
#include "types.hpp"

namespace manatools::midi {
	constexpr FourCC HEADER_MAGIC("MThd");
	constexpr FourCC TRACK_MAGIC("MTrk");

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

	enum class Controller : u8 {
		BankSelect_MSB          = 0,
		ModulationWheel_MSB     = 1,
		BreathController_MSB    = 2,
		Undefined3_MSB          = 3,
		FootController_MSB      = 4,   // pedal
		PortamentoTime_MSB      = 5,
		DataEntry_MSB           = 6,
		Volume_MSB              = 7,
		Balance_MSB             = 8,
		Undefined9_MSB          = 9,
		Pan_MSB                 = 10,
		Expression_MSB          = 11,
		EffectController1_MSB   = 12,  // ignored in Dreamcast?
		EffectController2_MSB   = 13,  // ignored in Dreamcast?
		Undefined14_MSB         = 14,
		Undefined15_MSB         = 15,
		GeneralPurpose1_MSB     = 16,
		GeneralPurpose2_MSB     = 17,
		GeneralPurpose3_MSB     = 18,
		GeneralPurpose4_MSB     = 19,
		Undefined20_MSB         = 20,  // Dreamcast: FEG-Lv0
		Undefined21_MSB         = 21,  // Dreamcast: FEG-Lv1
		Undefined22_MSB         = 22,  // Dreamcast: FEG-Lv2
		Undefined23_MSB         = 23,  // Dreamcast: FEG-Lv3
		Undefined24_MSB         = 24,  // Dreamcast: FEG-Lv4
		Undefined25_MSB         = 25,  // Dreamcast: FEG-AR
		Undefined26_MSB         = 26,  // Dreamcast: FEG-D1R
		Undefined27_MSB         = 27,  // Dreamcast: FEG-D2R
		Undefined28_MSB         = 28,  // Dreamcast: FEG-RR
		Undefined29_MSB         = 29,  // Dreamcast?
		Undefined30_MSB         = 30,  // Dreamcast?
		Undefined31_MSB         = 31,  // Dreamcast: Indicates a loop?

		BankSelect_LSB          = 32,  // important!
		ModulationWheel_LSB     = 33,
		BreathController_LSB    = 34,
		Undefined35_LSB         = 35,
		FootController_LSB      = 36,  // pedal
		PortamentoTime_LSB      = 37,
		DataEntry_LSB           = 38,
		Volume_LSB              = 39,
		Balance_LSB             = 40,
		Undefined41_LSB         = 41,
		Pan_LSB                 = 42,
		Expression_LSB          = 43,
		EffectController1_LSB   = 44,  // ignored in Dreamcast?
		EffectController2_LSB   = 45,  // ignored in Dreamcast?
		Undefined46_LSB         = 46,
		Undefined47_LSB         = 47,
		GeneralPurpose1_LSB     = 48,  // Dreamcast: TimigFlagSet (not my typo)
		GeneralPurpose2_LSB     = 49,  // Dreamcast?
		GeneralPurpose3_LSB     = 50,  // Dreamcast?
		GeneralPurpose4_LSB     = 51,  // Dreamcast?
		Undefined52_LSB         = 52,  // Dreamcast: FEG-Lv0
		Undefined53_LSB         = 53,  // Dreamcast: FEG-Lv1
		Undefined54_LSB         = 54,  // Dreamcast: FEG-Lv2
		Undefined55_LSB         = 55,  // Dreamcast: FEG-Lv3
		Undefined56_LSB         = 56,  // Dreamcast: FEG-Lv4
		Undefined57_LSB         = 57,  // Dreamcast?
		Undefined58_LSB         = 58,  // Dreamcast?
		Undefined59_LSB         = 59,  // Dreamcast?
		Undefined60_LSB         = 60,  // Dreamcast?
		Undefined61_LSB         = 61,  // Dreamcast?
		Undefined62_LSB         = 62,  // Dreamcast?
		Undefined63_LSB         = 63,  // Dreamcast?

		DamperPedal             = 64,  // Dreamcast: Hold1
		Portamento              = 65,
		Sostenuto               = 66,
		SoftPedal               = 67,
		LegatoFootswitch        = 68,  // ignored in Dreamcast?
		Hold2                   = 69,
		SoundController1        = 70,  // ignored in Dreamcast?    (MIDI: Sound Variation)
		SoundController2        = 71,  // Dreamcast: Filter-Q MSB  (MIDI: Same? "Timbre/Harmonic Intensity")
		SoundController3        = 72,  // Dreamcast: Release Time  (MIDI: Release Time)
		SoundController4        = 73,  // Dreamcast: Attack Time   (MIDI: Attack Time)
		SoundController5        = 74,  // Dreamcast: Cutoff Frequ. (MIDI: Same? "Brightness")
		SoundController6        = 75,  // Dreamcast: CutoffF Rate  (MIDI: Generic/Unassigned)
		SoundController7        = 76,  // ignored in Dreamcast?
		SoundController8        = 77,  // Dreamcast: FX Panpot     (MIDI: Generic/Unassigned)
		SoundController9        = 78,  // Dreamcast: FX Level      (MIDI: Generic/Unassigned)
		SoundController10       = 79,  // Dreamcast: FX Prog Chg   (MIDI: Generic/Unassigned)
		GeneralPurpose5         = 80,  // Dreamcast: QSoundCtrl
		GeneralPurpose6         = 81,  // Dreamcast: QSndPanRest
		GeneralPurpose7         = 82,
		GeneralPurpose8         = 83,
		PortamentoControl       = 84,  // ignored in Dreamcast?
		Undefined85             = 85,  // Dreamcast: Filter-Q LSB
		Undefined86             = 86,
		Undefined87             = 87,
		HighResVelocityPrefix   = 88,  // Dreamcast: AEG-DL
		Undefined89             = 89,  // Dreamcast: AEG-D1R
		Undefined90             = 90,  // Dreamcast: AEG-D2R
		Effect1Depth            = 91,  // Dreamcast: FX Send Lev   (MIDI: Reverb Send Level)
		Effect2Depth            = 92,  // Dreamcast: Tremolo       (MIDI: Tremolo Depth)
		Effect3Depth            = 93,  // Dreamcast: Chorus Depth  (MIDI: Chorus Level)
		Effect4Depth            = 94,  // Dreamcast: Detune        (MIDI: Detune Level)
		Effect5Depth            = 95,  // Dreamcast: Phaser        (MIDI: Phaser Level)
		DataIncrement           = 96,
		DataDecrement           = 97,
		NRPN_LSB                = 98,
		NRPN_MSB                = 99,
		RPN_LSB                 = 100,
		RPN_MSB                 = 101,
		Undefined102            = 102,
		Undefined103            = 103,
		Undefined104            = 104,
		Undefined105            = 105,
		Undefined106            = 106,
		Undefined107            = 107,
		Undefined108            = 108,
		Undefined109            = 109,
		Undefined110            = 110, // Dreamcast: CutoffF Rate1
		Undefined111            = 111, // Dreamcast: CutoffF Rate2
		Undefined112            = 112, // Dreamcast: CutoffF Rate4
		Undefined113            = 113, // Dreamcast: CutoffF Rate8
		Undefined114            = 114, // Dreamcast: CutoffF R 16
		Undefined115            = 115, // Dreamcast: CutoffF R 32
		Undefined116            = 116, // Dreamcast: CutoffF R 64
		Undefined117            = 117, // Dreamcast: CutoffF R 128
		Undefined118            = 118,
		Undefined119            = 119,
		AllSoundOff             = 120, // ignored in Dreamcast?
		ResetAllControllers     = 121,
		LocalControl            = 122,
		AllNotesOff             = 123,
		OmniModeOff             = 124,
		OmniModeOn              = 125,
		MonoModeOn              = 126,
		PolyModeOn              = 127,
	};

	enum class MetaEvents : u8 {
		Marker     = 0x06,
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
		s16 pitch; // -8192 ... 8191
	};

	struct Marker {
		u32 delta;
		std::string text;
	};

	struct EndOfTrack {
		u32 delta;
	};

	struct SetTempo {
		u32 delta;
		u32 tempo : 24;
	};

	using MetaEvent = std::variant<
		Marker,
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
