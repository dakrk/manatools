#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/msb.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;

void msbExtractMSDs(const fs::path& msbPath, const fs::path& msdOutPath) {
	auto msb = manatools::msb::load(msbPath);

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

// TODO: dear god this needs better argument parsing
int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "extract")) {
			if (argc < 4)
				goto invalid;

			msbExtractMSDs(argv[2], argv[3]);
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