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
	io::ErrorHandler errHandler(io, true, true);
	MSD msd;

	// we could be reading from anywhere in a file, store the beginning of *our* data
	auto startPos = io.tell();

	u32 gateExt = 0;
	u32 stepExt = 0;

	FourCC magic;
	io.readFourCC(&magic);
	if (magic != MSD_MAGIC) {
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
		if (IN_RANGE(status, 0x00, 0x7F)) {
			Note msg(channel);
			io.readU8(&msg.note);
			io.readU8(&msg.velocity);

			u32 gate = 0;
			u32 step = 0;
			int gateBytes = (status >> 5) + 1;

			while (gateBytes-- > 0) {
				u8 g; io.readU8(&g);
				gate = (gate << 8) | g;
			}

			/**
			 * So much would be cleaner if the IO API actually returned the value,
			 * so conversion would automatically take place...
			 * Should be able to do that with function overloading, though, as to
			 * not replace the `bool` return value ones that still have a use.
			 */
			if (status & 0x10) {
				u16 s; io.readU16BE(&s);
				step = s;
			} else {
				u8 s; io.readU8(&s);
				step = s;
			}

			/**
			 * No sort of protection to prevent gate/step time overflows, but realistically
			 * nothing would/can be above 32 bits in size anyway
			 */
			msg.gate = gate + gateExt;
			msg.step = step + stepExt;

			gateExt = stepExt = 0;
			msd.messages.push_back(msg);
			return true;
		}

		// Messages that associate to a channel
		switch (static_cast<Status>(status)) {
			case Status::ControlChange: {
				ControlChange msg(channel);

				u8 type;
				io.readU8(&type);
				// remove leftmost bit as that's used to indicate step data size
				msg.controller = type & 0x7F;
				io.readU8(&msg.value);
				msg.step = readVar(io, type) + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			case Status::ProgramChange: {
				ProgramChange msg(channel);

				u8 data;
				io.readU8(&data);
				msg.program = data & 0x7F;
				msg.step = readVar(io, data) + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			case Status::ChannelPressure: {
				ChannelPressure msg(channel);

				u8 data;
				io.readU8(&data);
				msg.pressure = data & 0x7F;
				msg.step = readVar(io, data) + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			case Status::PitchWheelChange: {
				PitchWheelChange msg(channel);

				u8 data;
				io.readU8(&data);
				// Convert pitch from to range of -64 to 63 (from 0 to 127)
				msg.pitch = (data & 0x7F) - 64;
				msg.step = readVar(io, data) + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			default: {
				// here to make clang -wswitch shut up, the rest of the enum values are in another switch
			}
		}

		// Messages that do not associate to a channel
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
					 * TODO: pass count of how much references we're currently reading as
					 * to prevent an infinite loop from a bad file?
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
				msg.step = readVar(io, data) + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			case Status::EndOfTrack: {
				return false;
			}

			case Status::TempoChange: {
				TempoChange msg;

				/**
				 * TempoChange step values are a bit weird, as there doesn't seem to be
				 * any way to specify if it's above 8 bits, and as such in Segagaga,
				 * _BATTLD.MRG had step extend messages preceding a tempo change.
				 */
				io.readU16BE(&msg.tempo);
				u8 step; io.readU8(&step);
				msg.step = step + stepExt;

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			case Status::SysEx: {
				SysEx msg;

				u8 step; io.readU8(&step);
				msg.step = step + stepExt;

				// TODO: Length could be VLQ, but no IO method to do that yet...
				u8 len; io.readU8(&len);
				msg.data.resize(len);
				io.readVec(msg.data);

				// Unknown, but seemingly changes value depending on step
				msg.unk1 = readVar(io, step);

				stepExt = 0;
				msd.messages.push_back(msg);
				return true;
			}

			default: {
				// shush
			}
		}

		/**
		 * Extend messages are not included in a reference's length, so the handlers for
		 * extend messages process a new message themselves, which should keep them in a
		 * loop until the next actual message is encountered.
		 */

		// Gate time extend
		if (IN_RANGE(statusByte, 0x88, 0x8B)) {
			gateExt += GATE_EXT_TABLE[statusByte & 3];
			self(self);
			return true;
		}

		// Step time extend
		if (IN_RANGE(statusByte, 0x8C, 0x8F)) {
			stepExt += STEP_EXT_TABLE[statusByte & 3];
			self(self);
			return true;
		}

		char err[64];
		snprintf(err, std::size(err), "Unknown MSD message encountered at 0x%lx: %x", io.tell(), statusByte);
		throw std::runtime_error(err);
	};

	/**
	 * My GCC version doesn't support C++23 "deducing this" and my Clang version crashes
	 * when trying to do it, so go back to the method of using two lambdas to be able to
	 * call them recursively
	 */
	auto readMessage = [&]() {
		return readMessageImpl(readMessageImpl);
	};

	while (readMessage());

	return msd;
}

MSD load(const fs::path& path) {
	io::FileIO io(path, "rb");
	return load(io);
}

} // namespace manatools::msd
