#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/version.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;

constexpr u8 MLT_MAGIC[4] = {'S', 'M', 'L', 'T'};
constexpr u8 MPB_MAGIC[4] = {'S', 'M', 'P', 'B'};
constexpr u8 MDB_MAGIC[4] = {'S', 'M', 'D', 'B'};
constexpr u8 MSB_MAGIC[4] = {'S', 'M', 'S', 'B'};
constexpr u8 OSB_MAGIC[4] = {'S', 'O', 'S', 'B'};

void findFiles(const fs::path& inPath, const fs::path& outPath = fs::path()) {
	io::FileIO io(inPath, "rb", true, false);
	bool dryRun = outPath.empty();

	constexpr auto validFourCCs = std::to_array({
		MLT_MAGIC,
		MPB_MAGIC,
		MDB_MAGIC,
		MSB_MAGIC,
		OSB_MAGIC
	});

	/**
	 * Dumb, *really* inefficient searcher. I'm slow with thinking of algorithms so this will have
	 * to do for now. Perhaps mmap would give a good boost as that should be easier than buffering
	 * and isn't reading a byte a time, expecting the libc to do a good job with fread and doing
	 * its own buffering
	 */
	bool processNewChar = true;
	uint tries = 0;
	u8 chr;
	u8 fourCCBuf[4];
	u8 fourCCPos = 0;
	while (true) {
		if (processNewChar && !io.readU8(&chr)) {
			// reached end of file
			break;
		}

		bool foundMatch = false;

		for (size_t i = 0; i < std::size(validFourCCs); i++) {
			if (chr == validFourCCs[i][fourCCPos]) {
				foundMatch = true;
				fourCCBuf[fourCCPos++] = chr;

				if (fourCCPos != 4)
					continue;

				tries = 0;
				fourCCPos = 0;

				// 4 bytes, so could easily be optimised to be a switch case with 32 bit uints
				if (!memcmp(fourCCBuf, MLT_MAGIC, 4)) {
					puts("found MLT");
				} else if (!memcmp(fourCCBuf, MPB_MAGIC, 4)) {
					puts("found MPB");
				} else if (!memcmp(fourCCBuf, MDB_MAGIC, 4)) {
					puts("found MDB");
				} else if (!memcmp(fourCCBuf, MSB_MAGIC, 4)) {
					puts("found MSB")
				} else if (!memcmp(fourCCBuf, OSB_MAGIC, 4)) {
					puts("found OSB");
				} else {
					assert(!"Unhandled FourCC encountered");
				}

				break;
			}
		}

		// restart sequence on current byte, and go to next byte if still not
		if (!foundMatch) {
			tries++;
			processNewChar = tries >= 2;
			fourCCPos = 0;
		}
	}
}

int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "list")) {
			findFiles(argv[2]);
		} else if (!strcmp(argv[1], "extract")) {
			if (argc < 4)
				goto invalid;

			findFiles(argv[2], argv[3]);
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
		"findtool - Dreamcast audio & music file discovery tool [version %s]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s list <in.bin>\n"
		"       %s extract <in.bin> <outdir>\n"
		"\n"
		"This tool looks through a larger file to find valid files that are supported by\n"
		"manatools nested inside it. This may help if you're dealing with otherwise\n"
		"unknown packed formats.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0],
		argv[0]
	);

	return 1;
}
