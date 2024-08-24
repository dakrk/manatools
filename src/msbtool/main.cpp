#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/midi.hpp>
#include <manatools/msb.hpp>
#include <manatools/msd.hpp>
#include <manatools/note.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;
namespace midi = manatools::midi;
namespace msb = manatools::msb;
namespace msd = manatools::msd;

// more sane way to use visitors in our case
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

void msbExtractMSDs(const fs::path& msbPath, const fs::path& msdOutPath) {
	auto msb = msb::load(msbPath);

	for (size_t s = 0; s < msb.sequences.size(); s++) {
		const auto& sequence = msb.sequences[s];

		if (!sequence.data.size()) {
			fprintf(stderr, "Warning: Sequence %zu has no data\n", s);
			continue;
		}

		fs::path msdName = msbPath.stem().concat('_' + std::to_string(s) += ".msd");

		io::FileIO msdFile(msdOutPath / msdName, "wb");
		msdFile.writeVec(sequence.data);
	}
}

void msbDumpMSDs(const fs::path& msbPath) {
	auto msb = msb::load(msbPath);

	for (size_t s = 0; s < msb.sequences.size(); s++) {
		auto& data = msb.sequences[s];

		if (!data.data.size()) {
			fprintf(stderr, "Warning: Sequence %zu has no data\n", s);
			continue;
		}

		io::DynBufIO io(data.data);
		auto seq = msd::load(io);

		for (const msd::Message& m : seq.messages) {
			std::visit(overloaded {
				[](const msd::Note& msg) {
					printf("[Ch.%02u] Note            : note=%u (%s%d) velocity=%u gate=%u step=%u\n",
					       msg.channel, msg.note, manatools::noteName(msg.note), manatools::noteOctave(msg.note),
					       msg.velocity, msg.gate, msg.step);
				},

				[](const msd::ControlChange& msg) {
					printf("[Ch.%02u] Control Change  : controller=%u value=%x step=%u\n", msg.channel, static_cast<u8>(msg.controller), msg.value, msg.step);
				},

				[](const msd::ProgramChange& msg) {
					printf("[Ch.%02u] Program Change  : program=%u step=%u\n", msg.channel, msg.program, msg.step);
				},

				[](const msd::ChannelPressure& msg) {
					printf("[Ch.%02u] Channel Pressure: pressure=%u step=%u\n", msg.channel, msg.pressure, msg.step);
				},

				[](const auto& msg) {
					(void)msg;
					// perhaps this should assert/throw because it's a bug if we get to this point
					fprintf(stderr, "Unhandled message encountered!\n");
				}
			}, m);
		}
	}
}

void msbExportMIDIs(const fs::path& msbPath, const fs::path& midiOutPath) {
	auto msb = msb::load(msbPath);

	for (size_t s = 0; s < msb.sequences.size(); s++) {
		auto& data = msb.sequences[s];

		if (!data.data.size()) {
			fprintf(stderr, "Warning: Sequence %zu has no data\n", s);
			continue;
		}

		io::DynBufIO io(data.data);
		midi::File midiFile;
		auto seq = msd::load(io);

		for (const msd::Message& m : seq.messages) {
			std::visit(overloaded {
				[&](const msd::Note& msg) {
					midiFile.events.push_back(midi::NoteOn {msg.step, msg.channel, msg.note, msg.velocity});
				},

				[&](const msd::ControlChange& msg) {
					midiFile.events.push_back(midi::ControlChange {msg.step, msg.channel, static_cast<u8>(msg.controller), msg.value});
				},

				[&](const msd::ProgramChange& msg) {
					midiFile.events.push_back(midi::ProgramChange {msg.step, msg.channel, msg.program});
				},

				[](const auto& msg) {
					(void)msg;
					// perhaps this should assert/throw because it's a bug if we get to this point
					fprintf(stderr, "Unhandled message encountered!\n");
				}
			}, m);
		}

		midiFile.events.push_back(midi::EndOfTrack{});

		fs::path midiName = msbPath.stem().concat('_' + std::to_string(s) += ".mid");
		midiFile.save(midiOutPath / midiName);
	}
}

// TODO: dear god this needs better argument parsing
int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "extract")) {
			if (argc < 4)
				goto invalid;

			msbExtractMSDs(argv[2], argv[3]);
		} else if (!strcmp(argv[1], "dump")) {
			msbDumpMSDs(argv[2]);
		} else if (!strcmp(argv[1], "exportmidis")) {
			if (argc < 4)
				goto invalid;

			msbExportMIDIs(argv[2], argv[3]);
		} else {
			goto invalid;
		}
	} catch (std::runtime_error& err) {
		fprintf(stderr, "An error occurred: %s\n", err.what());
		return 1;
	}

	return 0;

invalid:
	fprintf(
		stderr,
		"msbtool - Dreamcast MIDI Sequence Bank tool [version 0.1.0]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s extract <in.msb> <outdir>\n"
		"       %s dump <in.msb>\n"
		"\n"
		"An MSB file is a collection of sequences of MIDI messages.\n"
		"Typically these are packed inside an MLT, and are used for music.\n"
		"\n"
		"Despite its name, the data is not particularly MIDI compliant and uses\n"
		"different status IDs for its messages, and employs different file size\n"
		"optimisations. Such optimisations include:\n"
		"    - Replacing Note Off messages with \"gate time\" in the Note On message.\n"
		"    - Having a \"reference\" message as some sort of basic compression, by\n"
		"      storing a file offset and the number of messages to an instance of a\n"
		"      previously repeated sequence.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		argv[0],
		argv[0]
	);

	return 1;
}