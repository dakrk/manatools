#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/msb.hpp>
#include <manatools/msd.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;
namespace msb = manatools::msb;
namespace msd = manatools::msd;

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
			std::visit(overloaded{
				[&](const msd::Note& msg) {
					printf("[Channel %u] NOTE: key=%u velocity=%u gate=%u step=%u\n", msg.channel, msg.key, msg.velocity, msg.gate, msg.step);
				},

				[&](const auto& msg) {
					(void)msg;
					fprintf(stderr, "Unhandled message encountered\n");
				}
			}, m);
		}
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
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		argv[0]
	);

	return 1;
}