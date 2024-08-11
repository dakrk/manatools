#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/mlt.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;

#ifndef _WIN32
	#define HEADING "\033[1m"
	#define HEADING_END "\033[0m"
#else
	// forgot Windows conhost sucks
	#define HEADING ""
	#define HEADING_END ""
#endif

void mltListUnits(const fs::path& mltPath) {
	auto mlt = manatools::mlt::load(mltPath);

	printf(HEADING "%4s  %4s  %4s  %13s  %11s  %13s  %11s\n" HEADING_END,
	       "Unit", "Type", "Bank", "Offset [AICA]", "Size [AICA]", "Offset [File]", "Size [File]");

	for (size_t u = 0; u < mlt.units.size(); u++) {
		const auto& unit = mlt.units[u];

		char type[5];
		memcpy(type, unit.fourCC, 4);
		type[4] = '\0';

		// Will become misaligned if something is too big, but such chances are low
		printf("%4zu  %4s  %4u  %13x  %11u  %13x  %11zu\n",
		       u, type, unit.bank, unit.aicaDataPtr_, unit.aicaDataSize_,
		       unit.fileDataPtr_, unit.data.size());
	}
}

void mltExtractUnits(const fs::path& mltPath, const fs::path& unitOutPath) {
	auto mlt = manatools::mlt::load(mltPath);

	for (size_t u = 0; u < mlt.units.size(); u++) {
		const auto& unit = mlt.units[u];

		if (!unit.data.size()) {
			// TODO: Print FourCC of unit (as no data makes sense for SFPW)
			fprintf(stderr, "Warning: Unit %zu has no data\n", u);
			continue;
		}

		// Cheat by using the fact that the file extensions are the last 3 characters of the FourCC
		char type[5];
		memcpy(type + 1, unit.fourCC + 1, 3);
		type[0] = '.';
		for (int i = 1; i < 4; i++) {
			type[i] = tolower(type[i]);
		}
		type[4] = '\0';

		fs::path unitName = mltPath.stem().concat('_' + std::to_string(u) += type);

		io::FileIO unitFile(unitOutPath / unitName, "wb");
		unitFile.writeVec(unit.data);
	}
}

// TODO: As with mpbtool, this really needs better argument parsing
int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "extract")) {
			if (argc < 4)
				goto invalid;

			mltExtractUnits(argv[2], argv[3]);
		} else if (!strcmp(argv[1], "list")) {
			mltListUnits(argv[2]);
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
		"mlttool - Dreamcast Multi-Unit file tool [version 0.0.1]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s extract <in.mlt> <outdir>\n"
		"       %s list <in.mlt>\n"
		"\n"
		"An MLT file groups multiple audio-related files (called \"units\" or \"blocks\")\n"
		"together into a single file. The MLT file specifies where each unit shall be\n"
		"placed in the AICA sound processor's RAM.\n"
		"It can contain MPBs (sound banks), MSBs (MIDI sequences), and more.\n"
		"\n"
		"Offsets/sizes of 0xFFFFFFFF indicate that that block is unused, or that that\n"
		"field is inapplicable for a unit of that type (e.g, with SFPW, where only the\n"
		"AICA offset and size is relevant, as SFPW is only there to indicate how much\n"
		"RAM to allocate to DSP effects).\n"
		"A unit's AICA size may be bigger than its file size seemingly as to satisfy\n"
		"alignment requirements.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		argv[0],
		argv[0]
	);

	return 1;
}