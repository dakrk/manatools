#pragma once
#include <any>
#include <array>
#include <optional>
#include <vector>

#include "aica.hpp"
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
	using common::PitchRegs;
	using common::LFORegs;
	using common::FXRegs;
	using common::FilterEnvelope;
	
	// Max 128 Splits per Layer
	struct Split {
		static s8 fromPanPot(u8 panPot);
		static u8 toPanPot(s8 panPot, u32 version);

		u8 unkFlags        = 0;
		
		bool loop          = false;
		u16 loopStart      = 0;     // [0 -> 65535]
		u16 loopEnd        = 0;     // [0 -> 65535]

		AmpEnvelope amp;

		PitchRegs pitch;
		LFORegs lfo;
		FXRegs fx;

		u8 unk1            = 0;     // Unknown

		s8 panPot          = 0;     // [-15 -> 15]
		u8 directLevel     = 15;    // [0 -> 15]

		u8 oscillatorLevel = 255;   // [0 -> 255]

		FilterEnvelope filter;

		u8 startNote       = 0;     // [0 -> 127]
		u8 endNote         = 127;   // [0 -> 127]
		u8 baseNote        = 60;    // [0 -> 127]
		s8 fineTune        = 0;     // [-128 -> 127]

		u16 unk2           = 0;     // Unknown (seemingly fine tune related on version 1?)

		u8 velocityCurveID = 0;
		u8 velocityLow     = 1;     // [0 -> 127]
		u8 velocityHigh    = 127;   // [0 -> 127]

		bool drumMode      = false;
		u8 drumGroupID     = 0;     // [0 -> 255]

		u8 unk3            = 0;     // Unknown

		u32 ptrToneData_   = 0;     // Only for presentation post-load
		tone::Tone tone;

		u8 effectiveRate(u8 rate) const {
			return aica::calcEffectiveRate(amp.keyRateScaling, pitch, rate);
		}
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
		std::any userData;
		std::array<std::optional<Layer>, MAX_LAYERS> layers;
	};

	// Seemingly max 31 Velocities per Bank
	struct Velocity {
		static Velocity defaultCurve();
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

	Bank load(const fs::path& path, bool guessToneSize = true);
} // namespace manatools::mpb
