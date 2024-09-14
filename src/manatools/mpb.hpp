#pragma once
#include <array>
#include <optional>
#include <vector>

#include "common.hpp"
#include "filesystem.hpp"
#include "fourcc.hpp"
#include "tone.hpp"
#include "types.hpp"

namespace manatools::mpb {
	constexpr FourCC MPB_MAGIC("SMPB");
	constexpr FourCC MDB_MAGIC("SMDB");
	constexpr FourCC MPB_END("ENDB");

	constexpr size_t MAX_PROGRAMS   = 128;
	constexpr size_t MAX_LAYERS     = 4;
	constexpr size_t MAX_SPLITS     = 128; // per layer
	constexpr size_t MAX_VELOCITIES = 31;

	using common::LFOWaveType;
	using common::AmpEnvelope;
	using common::FilterEnvelope;

	struct SplitLFO {
		u8 ampDepth   = 0; // [0 -> 7]
		LFOWaveType ampWave   = LFOWaveType::Saw;

		u8 pitchDepth = 0; // [0 -> 7]
		LFOWaveType pitchWave = LFOWaveType::Saw;

		u8 frequency  = 0; // [0 -> 31]
	};

	struct SplitFX {
		u8 inputCh = 0; // [0 -> 15]
		u8 level   = 0; // [0 -> 15]
	};
	
	// Max 128 Splits per Layer
	struct Split {
		static s8 fromPanPot(u8 panPot);
		static u8 toPanPot(s8 panPot, u32 version);

		bool loop          = false;
		u16 loopStart      = 0;     // [0 -> 65535]
		u16 loopEnd        = 0;     // [0 -> 65535]

		AmpEnvelope amp;

		u16 unk1           = 0;     // Unknown

		bool lfoOn         = false;
		SplitLFO lfo;
		SplitFX fx;

		u8 unk2            = 0;     // Unknown

		s8 panPot          = 0;     // [-15 -> 15]
		u8 directLevel     = 15;    // [0 -> 15]

		u8 oscillatorLevel = 255;   // [0 -> 255]

		bool filterOn      = true;
		FilterEnvelope filter;

		u8 startNote       = 0;     // [0 -> 127]
		u8 endNote         = 127;   // [0 -> 127]
		u8 baseNote        = 60;    // [0 -> 127]
		u8 fineTune        = 0;     // [-64 -> 63]

		u16 unk3           = 0;     // Unknown (seemingly fine tune related on version 1?)

		u8 velocityCurveID = 0;
		u8 velocityLow     = 1;     // [0 -> 127]
		u8 velocityHigh    = 127;   // [0 -> 127]

		bool drumMode      = false;
		u8 drumGroupID     = 0;     // [0 -> 255]

		u8 unk4            = 0;     // Unknown

		u32 ptrToneData_   = 0;     // Only for presentation post-load
		tone::Tone tone;
	};

	// Max 4 Layers per Program
	struct Layer {
		u16 delay        = 0; // [0 -> 1024]
		u16 unk1         = 0; // Unknown
		u8 bendRangeHigh = 2; // [0 -> 24]
		u8 bendRangeLow  = 2; // [24 <- 0]
		u16 unk2         = 0; // Unknown

		std::vector<Split> splits;
	};

	// Max 128 Programs per Bank
	struct Program {
		uint usedLayers() const;
		std::array<std::optional<Layer>, MAX_LAYERS> layers;
	};

	// Seemingly max 31 Velocities per Bank
	struct Velocity {
		u8 data[128]{};
	};

	struct Bank {
		void save(const fs::path& path);

		/**
		 * Convenience methods that return nullptr if bounds checks don't pass, as to avoid
		 * having the same bounds checks littered around everywhere
		 */
		const Program* program(size_t programIdx) const;
		const Layer* layer(size_t programIdx, size_t layerIdx) const;
		const Split* split(size_t programIdx, size_t layerIdx, size_t splitIdx) const;

		Program* program(size_t programIdx);
		Layer* layer(size_t programIdx, size_t layerIdx);
		Split* split(size_t programIdx, size_t layerIdx, size_t splitIdx);

		bool drum   = false;
		u32 version = 2;
		std::vector<Program> programs;
		std::vector<Velocity> velocities;
	};

	Bank load(const fs::path& path);
} // namespace manatools::mpb
