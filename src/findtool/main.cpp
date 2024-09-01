#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/version.hpp>
#include <mio/mmap.hpp>

#define BEGIN_END(c) std::begin(c), std::end(c)
#define TRY_RET(cond) if (cond) return;

namespace fs = manatools::fs;
namespace io = manatools::io;

constexpr u8 MLT_FOURCC[4] = {'S', 'M', 'L', 'T'};
constexpr u8 MPB_FOURCC[4] = {'S', 'M', 'P', 'B'};
constexpr u8 MDB_FOURCC[4] = {'S', 'M', 'D', 'B'};
constexpr u8 MSB_FOURCC[4] = {'S', 'M', 'S', 'B'};
constexpr u8 OSB_FOURCC[4] = {'S', 'O', 'S', 'B'};
constexpr u8 ENDB_FOURCC[4] = {'E', 'N', 'D', 'B'};

template<typename It1, typename It2, typename Callback>
void searchAll(It1 first, It1 last, It2 s_first, It2 s_last, Callback cb) {
	auto it = first;
	while (it < last) {
		it = std::search(it, last, s_first, s_last);

		if (it == last)
			break;

		cb(it);

		it += std::distance(s_first, s_last);
	}
}

void strToLower(char* str) {
	char ch;
	do {
		ch = *str;
		*str++ = tolower(ch);
	} while (ch);
}

fs::path makeFileName(const fs::path& inPath, uintptr_t pos, const char* name) {
	char nameSuffix[24];
	snprintf(nameSuffix, std::size(nameSuffix), "_0x%zx.%s", pos, name);
	strToLower(nameSuffix);

	return inPath.stem().concat(nameSuffix);
}

void findFiles(const fs::path& inPath, const fs::path& outPath = fs::path()) {
	// a ummap_source would be more suitable, but a SpanIO isn't const
	mio::ummap_sink inSink(inPath.string());
	io::SpanIO inIO(inSink, true, false);

	bool dryRun = outPath.empty();

	auto checkCommon = [&](auto it, const char* name) {
		uintptr_t startPos = it - inSink.begin();

		// EOF is not expected during init, but is later
		inIO.eofErrors(true);
		inIO.jump(startPos);
		inIO.forward(4); // jump over header FourCC
		inIO.eofErrors(false);

		u32 version;
		TRY_RET(!inIO.readU32LE(&version));
		if (version != 1 && version != 2) {
			// zx should consistently be suitable for printing a uintptr_t I'd hope
			fprintf(stderr, "[%08zx] %s FourCC found, but unknown/invalid version encountered.\n", startPos, name);
			return;
		}

		u32 fileSize;
		u8 endCC[4];
		TRY_RET(!inIO.readU32LE(&fileSize));
		TRY_RET(!inIO.jump(startPos + fileSize));
		TRY_RET(!inIO.backward(4));
		TRY_RET(!inIO.readArrT(endCC));

		if (memcmp(ENDB_FOURCC, endCC, 4)) {
			fprintf(stderr, "[%08zx] %s FourCC found, but couldn't find ENDB after supposed fileSize.\n", startPos, name);
			return;
		}

		printf("[%08zx] Found %s, size=%u\n", startPos, name, fileSize);

		if (!dryRun) {
			fs::path fileName = makeFileName(inPath, startPos, name);
			io::FileIO outFile(outPath / fileName, "wb");
			outFile.writeSpan(inIO.span().subspan(startPos, fileSize));
		}
	};

	// More error prone, as there isn't a footer I can easily check against
	searchAll(BEGIN_END(inSink), BEGIN_END(MLT_FOURCC), [&](auto it) {
		uintptr_t startPos = it - inSink.begin();

		inIO.eofErrors(true);
		inIO.jump(startPos);
		inIO.forward(4);
		inIO.eofErrors(false);

		u32 numUnits;
		TRY_RET(!inIO.forward(4)); // perhaps version, but weird
		TRY_RET(!inIO.readU32LE(&numUnits));
		TRY_RET(!inIO.forward(20));

		u32 highPtr = 0;
		u32 highSize = 0;

		/**
		 * TODO: Don't check for failure every read, and only later?
		 * Could perhaps remove the need for this dumb macro
		 */
		for (u32 i = 0; i < numUnits; i++) {
			u32 filePtr, fileSize;
			TRY_RET(!inIO.forward(4 * 4)); // Skip FourCC, bank, and AICA fields
			TRY_RET(!inIO.readU32LE(&filePtr));
			TRY_RET(!inIO.readU32LE(&fileSize));
			TRY_RET(!inIO.forward(8));

			if (filePtr != 0xFFFFFFFF && fileSize != 0xFFFFFFFF) {
				if (filePtr > highPtr) {
					highPtr = filePtr;
					highSize = fileSize;
				}
			}
		}

		printf("[%08zx] Found MLT, size=%u\n", startPos, highSize);

		if (!dryRun) {
			fs::path fileName = makeFileName(inPath, startPos, "MLT");
			io::FileIO outFile(outPath / fileName, "wb");
			outFile.writeSpan(inIO.span().subspan(startPos, highPtr + highSize));
		}
	});

	searchAll(BEGIN_END(inSink), BEGIN_END(MPB_FOURCC), [&](auto it) {
		checkCommon(it, "MPB");
	});

	searchAll(BEGIN_END(inSink), BEGIN_END(MDB_FOURCC), [&](auto it) {
		checkCommon(it, "MDB");
	});

	searchAll(BEGIN_END(inSink), BEGIN_END(MSB_FOURCC), [&](auto it) {
		checkCommon(it, "MSB");
	});

	searchAll(BEGIN_END(inSink), BEGIN_END(OSB_FOURCC), [&](auto it) {
		checkCommon(it, "OSB");
	});
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
		"NOTE: This is not a magic tool that will always find everything and extract\n"
		"perfectly. Some things may be compressed, or data may not be stored\n"
		"continuously, meaning feeding whole game rips may not fully work and could miss\n"
		"out files or output broken ones. This can affect MLT extraction the most.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0],
		argv[0]
	);

	return 1;
}
