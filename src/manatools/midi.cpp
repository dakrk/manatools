#include <cstring>

#include "midi.hpp"

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

namespace manatools::midi {

static constexpr u8 makeStatus(u8 status, u8 channel) {
	return (status & 0xF0) | (channel & 0x0F);
}

static constexpr u8 makeStatus(Status status, u8 channel) {
	return makeStatus(static_cast<u8>(status), channel);
}

void File::save(io::DataIO& io) {
	bool exceptions = io.exceptions(true); // ugh
	bool eofErrors = io.eofErrors(true);

	io.writeArrT(HEADER_MAGIC);
	io.writeU32BE(6); // chunk size
	io.writeU16BE(0); // format 0 (SMF0)
	io.writeU16BE(1); // only 1 track
	io.writeU16BE(division);

	io.writeArrT(TRACK_MAGIC);
	auto trackSizePos = io.tell(); // we'll write it once we're finished
	io.writeU32BE(0);

	for (const auto& e : events) {
		std::visit(overloaded {
			[&](const NoteOn& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::NoteOn, event.channel));
				io.writeU8(event.note);
				io.writeU8(event.velocity);
			},

			[&](const NoteOff& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::NoteOff, event.channel));
				io.writeU8(event.note);
				io.writeU8(event.velocity);
			},

			[&](const PolyKeyPressure& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::PolyKeyPressure, event.channel));
				io.writeU8(event.note);
				io.writeU8(event.pressure);
			},

			[&](const ControlChange& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::ControlChange, event.channel));
				io.writeU8(event.controller);
				io.writeU8(event.value);
			},

			[&](const ProgramChange& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::ProgramChange, event.channel));
				io.writeU8(event.program);
			},

			[&](const ChannelPressure& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::ChannelPressure, event.channel));
				io.writeU8(event.pressure);
			},

			[&](const PitchWheelChange& event) {
				io.writeVLQ(event.delta);
				io.writeU8(makeStatus(Status::PitchWheelChange, event.channel));
				io.writeU8(event.pitch); // TODO
			},

			[&](const MetaEvent& e2) {
				u8 status = static_cast<u8>(Status::MetaEvent);
				std::visit(overloaded {
					[&](const EndOfTrack& event) {
						io.writeVLQ(event.delta);
						io.writeU8(status);
						io.writeU8(static_cast<u8>(MetaEvents::EndOfTrack));
						io.writeU8(0);
					},

					[&](const SetTempo& event) {
						io.writeVLQ(event.delta);
						io.writeU8(status);
						io.writeU8(static_cast<u8>(MetaEvents::SetTempo));
						io.writeU8(3); // length
						io.writeU8((event.tempo >> 16) & 0xFF); // hmm. maybe have `WriteU24BE` in io.cpp?
						io.writeU8((event.tempo >> 8)  & 0xFF);
						io.writeU8((event.tempo >> 0)  & 0xFF);
					}
				}, e2);
			},		

			[&](const auto& msg) {
				(void)msg;
				// TODO: perhaps assert or throw logic_error or something
			}
		}, e);
	}

	auto endPos = io.tell();
	io.jump(trackSizePos);
	io.writeU32BE(endPos - (trackSizePos + sizeof(u32)));

	io.exceptions(exceptions);
	io.eofErrors(eofErrors);
}

void File::save(const fs::path& path) {
	io::FileIO io(path, "wb");
	save(io);
}

} // namespace manatools::midi
