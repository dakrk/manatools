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
		u8 outu8;
		io.readU8(&outu8);
		out = outu8;
	}

	return out;
}

MSD load(const fs::path& path) {
	io::FileIO io(path, "rb");
	return load(io);
}

MSD load(io::DataIO& io) {
	bool exceptions = io.exceptions(true); // ugh
	bool eofErrors = io.eofErrors(true);
	MSD msd;

	u8 magic[4];
	io.readArrT(magic);
	if (memcmp(MSD_MAGIC, magic, sizeof(magic))) {
		throw std::runtime_error("Invalid MSD data");
	}

	io.readU32BE(&msd.unk1);
	io.readU32BE(&msd.unk2);

	bool processing = true;
	while (processing) {
		u8 statusByte;
		io.readU8(&statusByte);
		u8 channel = statusByte & 0x0F;
		u8 status = statusByte & 0xF0;

		// Note event
		if (IN_RANGE(status, 0x00, 0x3F)) {
			Note msg(channel);
			io.readU8(&msg.key);
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
			continue;
		}

		// Messages that associate to a channel (has an annotated range in the enum)
		switch (static_cast<Status>(status)) {
			case Status::ControlChange: {
				ControlChange msg(channel);

				u8 type;
				io.readU8(&type);

				// remove leftmost bit as that's used to indicate step data size
				msg.controller = static_cast<Controller>(type >> 1);
				io.readU8(&msg.value);
				msg.step = readVar(io, type);

				msd.messages.push_back(msg);
				continue;
			}

			case Status::ProgramChange: {
				ProgramChange msg(channel);

				u8 data;
				io.readU8(&data);

				msg.program = data >> 1;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				continue;
			}

			case Status::ChannelPressure: {
				ChannelPressure msg(channel);

				u8 data;
				io.readU8(&data);

				msg.pressure = data >> 1;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				continue;
			}

			default: {
				// here to make clang -wswitch shut up, the rest of the enum values are in another switch
			}
		}

		// Messages that do not associate to a channel (doesn't have an annotated range in the enum)
		switch (static_cast<Status>(statusByte)) {
			case Status::Reference: {
				/**
				 * TODO: oh. oh god. I would make reference code replace references with their respective events
				 * but they use actual file offsets and not event indexes. That is probably going to be *horrible*
				 */
				Reference msg;
				io.readU16BE(&msg.offset);
				io.readU8(&msg.length);
				msd.messages.push_back(msg);
				continue;
			}

			case Status::Loop: {
				Loop msg;

				u8 data;
				io.readU8(&data);

				msg.mode = data >> 1;
				msg.step = readVar(io, data);

				msd.messages.push_back(msg);
				continue;
			}

			case Status::EndOfSequence: {
				processing = false;
				continue;
			}

			case Status::TempoChange: {
				TempoChange msg;
				io.readU16BE(&msg.tempo);
				io.readU8(&msg.unk1);
				msd.messages.push_back(msg);
				continue;
			}

			default: {
				throw std::runtime_error("Unknown MSD message encountered");
			}
		}
	}

	io.exceptions(exceptions);
	io.eofErrors(eofErrors);

	return msd;
}

} // namespace manatools::msb
