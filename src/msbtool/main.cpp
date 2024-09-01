#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/midi.hpp>
#include <manatools/msb.hpp>
#include <manatools/msd.hpp>
#include <manatools/note.hpp>
#include <manatools/version.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;
namespace midi = manatools::midi;
namespace msb = manatools::msb;
namespace msd = manatools::msd;

// more sane way to use visitors in our case
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

struct NoteQueueItem {
	u32 endTime;
	u8 channel;
	u8 note;
	u8 velocity;

	bool operator<(const NoteQueueItem& rhs) const {
		return endTime < rhs.endTime;
	}
};

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

		printf("======== Start sequence %zu ========\n", s);

		for (const msd::Message& m : seq.messages) {
			std::visit(overloaded {
				[](const msd::Note& msg) {
					printf("[Ch.%02u] Note             : note=%u (%s%d) velocity=%u gate=%u step=%u\n",
					       msg.channel, msg.note, manatools::noteName(msg.note), manatools::noteOctave(msg.note),
					       msg.velocity, msg.gate, msg.step);
				},

				[](const msd::ControlChange& msg) {
					printf("[Ch.%02u] Control Change   : controller=%u value=%x step=%u\n", msg.channel, static_cast<u8>(msg.controller), msg.value, msg.step);
				},

				[](const msd::ProgramChange& msg) {
					printf("[Ch.%02u] Program Change   : program=%u step=%u\n", msg.channel, msg.program, msg.step);
				},

				[](const msd::ChannelPressure& msg) {
					printf("[Ch.%02u] Channel Pressure : pressure=%u step=%u\n", msg.channel, msg.pressure, msg.step);
				},

				[](const msd::PitchWheelChange& msg) {
					printf("[Ch.%02u] Pitch Wheel Chg. : pitch=%d step=%u\n", msg.channel, msg.pitch, msg.step);
				},

				[](const msd::Loop& msg) {
					printf("        Loop             : unk1=%x step=%u\n", msg.unk1, msg.step);
				},

				[](const msd::TempoChange& msg) {
					printf("        Tempo Change     : tempo=%u msecs/pqn (%.3f BPM)\n", msg.tempo, 60 * 1000.0 / msg.tempo);
				},

				[](const auto& msg) {
					(void)msg;
					assert(!"Recognised MSD message left unhandled");
				}
			}, m);
		}

		printf("======== End sequence %zu ========\n\n", s);
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

		midiFile.division = 0x10000 / seq.tpqn;

		std::multiset<NoteQueueItem> noteQueue;
		u32 curTime = 0;
		u32 lastTime = 0;
		bool startLoop = true;

		/**
		 * rather poor way of doing it especially because it relies on a multiset to make the endtimes
		 * ordered so it doesn't break by doing negative calculations and underflowing, and that makes me sad
		 * and I'm not sure produces entirely correct results :(
		 * I think some note off timings are a bit out of order because of this
		 * TODO: really revisit this
		 */
		auto processNoteQueue = [&](bool flush = false) {
			for (auto it = noteQueue.begin(); it != noteQueue.end();) {
				if (curTime >= it->endTime || flush) {
					u32 delta = it->endTime - lastTime;
					lastTime = it->endTime;
					midiFile.events.push_back(midi::NoteOff {delta, it->channel, it->note, it->velocity});
					it = noteQueue.erase(it);
				} else {
					it++;
				}
			}
		};

		for (const msd::Message& m : seq.messages) {
			processNoteQueue();

			u32 delta = curTime - lastTime;
			lastTime = curTime;

			std::visit(overloaded {
				[&](const msd::Note& msg) {
					midiFile.events.push_back(midi::NoteOn {delta, msg.channel, msg.note, msg.velocity});
					noteQueue.insert({curTime + msg.gate, msg.channel, msg.note, msg.velocity});
					curTime += msg.step;
				},

				[&](const msd::ControlChange& msg) {
					midiFile.events.push_back(midi::ControlChange {delta, msg.channel, msg.controller, msg.value});
					curTime += msg.step;
				},

				[&](const msd::ProgramChange& msg) {
					midiFile.events.push_back(midi::ProgramChange {delta, msg.channel, msg.program});
					curTime += msg.step;
				},

				[&](const msd::ChannelPressure& msg) {
					midiFile.events.push_back(midi::ChannelPressure {delta, msg.channel, msg.pressure});
					curTime += msg.step;
				},

				[&](const msd::PitchWheelChange& msg) {
					s16 pitch = ((msg.pitch + 64) << 7) - 8192;
					midiFile.events.push_back(midi::PitchWheelChange {delta, msg.channel, pitch});
					curTime += msg.step;
				},

				[&](const msd::Loop& msg) {
					// Insert a CC31 for Dreamcast compatibility, and loopStart/loopEnd for other software
					midiFile.events.push_back(midi::ControlChange {delta, 0, 31, msg.unk1});
					midiFile.events.push_back(midi::MetaEvent {midi::Marker {delta, startLoop ? "loopStart" : "loopEnd"}});
					startLoop = !startLoop;
					curTime += msg.step;
				},

				[&](const msd::TempoChange& msg) {
					midiFile.events.push_back(midi::MetaEvent {midi::SetTempo {0, static_cast<u32>(msg.tempo * 1000)}});
				},

				[](const auto& msg) {
					(void)msg;
					assert(!"Recognised MSD message left unhandled");
				}
			}, m);
		}

		// flush remaining note offs
		processNoteQueue(true);

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
		"msbtool - Dreamcast MIDI Sequence Bank tool [version %s]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s extract <in.msb> <outdir>\n"
		"       %s dump <in.msb>\n"
		"       %s exportmidis <in.mlt> <outdir>\n"
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
		manatools::versionString,
		argv[0],
		argv[0],
		argv[0]
	);

	return 1;
}
