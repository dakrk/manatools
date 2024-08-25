#include <cstring>
#include <utility>

#include "msd.hpp"

#define IN_RANGE(val, min, max) (min <= val && val <= max)

namespace manatools::msd {

static u16 readVar(io::DataIO& io, u8 data) {
	u16 out;

	if (data & 0x80) {
		io.readU16BE(&out);
	} else {
		u8 outU8;
		io.readU8(&outU8);
		out = outU8;
	}

	return out;
}

MSD load(io::DataIO& io) {
	bool exceptions = io.exceptions(true); // ugh
	bool eofErrors = io.eofErrors(true);
	MSD msd;

	// we could be reading from anywhere in a file, store the beginning of *our* data
	auto startPos = io.tell();

	u8 magic[4];
	io.readArrT(magic);
	if (memcmp(MSD_MAGIC, magic, sizeof(magic))) {
		throw std::runtime_error("Invalid MSD data");
	}

	io.readU32LE(&msd.tpqn);
	io.readU32LE(&msd.initialTempo);

	auto readMessageImpl = [&](const auto& self) {
		u8 statusByte;
		io.readU8(&statusByte);
		u8 channel = statusByte & 0x0F;
		u8 status = statusByte & 0xF0;

		// Note event
		if (IN_RANGE(status, 0x00, 0x3F)) {
			Note msg(channel);
			io.readU8(&msg.note);
			io.readU8(&msg.velocity);

			switch (status) {
				case 0x00: {
					u8 gate; io.readU8(&gate);
					u8 step; io.readU8(&step);
					msg.gate = gate;
					msg.step = step;
					break;
				}

				case 0x10: {
					u8 gate; io.readU8(&gate);
					msg.gate = gate;
					io.readU16BE(&msg.step);
					break;
				}

				case 0x20: {
					io.readU16BE(&msg.gate);
					u8 step; io.readU8(&step);
					msg.step = step;
					break;
				}

				case 0x30: {
					io.readU16BE(&msg.gate);
					io.readU16BE(&msg.step);
					break;
				}

				default: {
					std::unreachable();
				}
			}

			msd.messages.push_back(msg);
			return true;
		}

		// Messages that associate to a channel (has an annotated range in the enum)
		switch (static_cast<Status>(status)) {
			case Status::ControlChange: {
				ControlChange msg(channel);

				u8 type;
				io.readU8(&type);

				// remove leftmost bit as that's used to indicate step data size
				msg.controller = static_cast<Controller>(type & 0x7F);
				io.readU8(&msg.value);
				msg.step = readVar(io, type);

				msd.messages.push_back(msg);
				return true;
			}

			case Status::ProgramChange: {
				ProgramChange msg(channel);

				u8 data;
				io.readU8(&data);

				msg.program = data & 0x7F;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				return true;
			}

			case Status::ChannelPressure: {
				ChannelPressure msg(channel);

				u8 data;
				io.readU8(&data);

				msg.pressure = data & 0x7F;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				return true;
			}

			default: {
				// here to make clang -wswitch shut up, the rest of the enum values are in another switch
			}
		}

		// Messages that do not associate to a channel (doesn't have an annotated range in the enum)
		switch (static_cast<Status>(statusByte)) {
			case Status::Reference: {
				u16 offset;
				u8 length;
				io.readU16BE(&offset);
				io.readU8(&length);

				auto pos = io.tell();
				io.jump(startPos + offset);
				for (u8 i = 0; i < length; i++) {
					/**
					 * call ourself recursively. magic
					 * TODO: pass count of how much references we're currently reading as to
					 * prevent an infinite loop from a bad file?
					 */
					self(self);
				}
				io.jump(pos);
				return true;
			}

			case Status::Loop: {
				Loop msg;

				u8 data;
				io.readU8(&data);

				msg.unk1 = data & 0x7F;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				return true;
			}

			case Status::EndOfTrack: {
				return false;
			}

			case Status::TempoChange: {
				TempoChange msg;
				io.readU16BE(&msg.tempo);
				io.readU8(&msg.unk1);
				msd.messages.push_back(msg);
				return true;
			}

			default: {
				throw std::runtime_error("Unknown MSD message encountered");
			}
		}
	};

	/**
	 * My GCC version doesn't support C++23 "deducing this" and my Clang version
	 * crashes when trying to do it, so go back to the method of using two lambdas
	 * to be able to call them recursively
	 */
	auto readMessage = [&]() {
		return readMessageImpl(readMessageImpl);
	};

	while (readMessage());

	io.exceptions(exceptions);
	io.eofErrors(eofErrors);

	return msd;
}

MSD load(const fs::path& path) {
	io::FileIO io(path, "rb");
	return load(io);
}

} // namespace manatools::msd
