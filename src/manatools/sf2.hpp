#pragma once
#include <string>

/**
 * GCC and Clang specific options to make it shut up about code I don't control
 * (Clang is GNU compatible though so surely just __GNUG__ would work for both)
 */
#if defined(__GNUG__) || defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpessimizing-move"
	#include <sf2cute.hpp> // Bloats the binary a lot :(
	#pragma GCC diagnostic pop
#else
	#include <sf2cute.hpp>
#endif

#include "filesystem.hpp"
#include "mpb.hpp"

namespace manatools::sf2 {
	sf2cute::SoundFont fromMPB(const mpb::Bank& in, const std::string& bankName = "");
} // namespace manatools::sf2
