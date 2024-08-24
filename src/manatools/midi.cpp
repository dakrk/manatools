#include <cstring>

#include "midi.hpp"

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

namespace manatools::midi {

void File::save(io::DataIO& io) {
	bool exceptions = io.exceptions(true); // ugh
	bool eofErrors = io.eofErrors(true);

	io.writeArrT(HEADER_MAGIC);
	io.writeU32BE(6); // chunk size
	io.writeU16BE(0); // format 0 (SMF0)
	io.writeU16BE(1); // only 1 track
	io.writeU16BE(192); // ticks per quarter-note (TODO: perhaps make this something you can change)

	io.writeArrT(TRACK_MAGIC);
	auto trackSizePos = io.tell(); // we'll write it once we're finished

	for (const auto& e : events) {
		std::visit(overloaded {
			[&](const NoteOn& event) {
				(void)event;
			},

			[&](const NoteOff& event) {
				(void)event;
			},

			[&](const auto& msg) {
				(void)msg;
			}
		}, e);
	}

	(void)trackSizePos;

	io.exceptions(exceptions);
	io.eofErrors(eofErrors);
}

void File::save(const fs::path& path) {
	io::FileIO io(path, "wb");
	save(io);
}

} // namespace manatools::midi
