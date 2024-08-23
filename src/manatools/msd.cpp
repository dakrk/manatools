#include <cstring>

#include "msd.hpp"

#define IN_RANGE(val, min, max) (min <= val && val <= max)

namespace manatools::msd {

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

	io.readU32LE(&msd.unk1);
	io.readU32LE(&msd.unk2);

	bool processing = true;
	while (processing) {
		u8 statusByte;
		io.readU8(&statusByte);
		u8 channel = statusByte & 0x0F;
		u8 status = statusByte & 0xF0;

		// Note event
		if (IN_RANGE(status, 0x00, 0x3F)) {

		}

		// Messages that associate to a channel (has an annotated range in the enum)
		switch (static_cast<Status>(status)) {
			case Status::ControlChange: {
				break;
			}

			case Status::ProgramChange: {

			}

			case Status::ChannelPressure: {

			}

			default: {
				// makes clang -wswitch shush
			}
		}

		// Messages that do not associate to a channel (doesn't have an annotated range in the enum)
		switch (static_cast<Status>(statusByte)) {
			case Status::Reference: {
				break;
			}

			case Status::Loop: {
				break;
			}

			case Status::EndOfTrack: {
				processing = false;
				break;
			}

			case Status::TempoChange: {
				break;
			}

			default: {
				// unhandled, complain
			}
		}

		processing = false;
	}

	io.exceptions(exceptions);
	io.eofErrors(eofErrors);

	return msd;
}

} // namespace manatools::msb
