#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/version.hpp>
#include <mio/mmap.hpp>

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

#define BEGIN_END(c) std::begin(c), std::end(c)

void findFiles(const fs::path& inPath, const fs::path& outPath = fs::path()) {
	// a ummap_source would be more suitable, but a SpanIO isn't const
	mio::ummap_sink inSink(inPath.string());
	io::SpanIO inIO(inSink, true, false);

	bool dryRun = outPath.empty();

	searchAll(BEGIN_END(inSink), BEGIN_END(MPB_FOURCC), [&](auto it) {
		std::ptrdiff_t startPos = it - inSink.begin();

		// EOF is not expected during init, but is later
		inIO.eofErrors(true);
		inIO.jump(startPos);
		inIO.forward(4); // jump over header FourCC
		inIO.eofErrors(false);

		u32 version;
		if (!inIO.readU32LE(&version)) return;
		if (version != 1 && version != 2) {
			fprintf(stderr, "[%08lx] MPB FourCC found, but unknown/invalid version encountered.\n", startPos);
			return;
		}

		u32 fileSize;
		if (!inIO.readU32LE(&fileSize))        return;
		if (!inIO.jump(startPos + fileSize))   return;
		if (!inIO.backward(4))                 return;

		u8 endCC[4];
		if (!inIO.readArrT(endCC))             return;
		if (memcmp(ENDB_FOURCC, endCC, 4))     {
			printf("%lx 6 %x.%x.%x.%x %c%c%c%c\n", startPos, endCC[0], endCC[1], endCC[2], endCC[3], endCC[0], endCC[1], endCC[2], endCC[3]); return;
		}

		printf("[%08lx] Found MPB, size=%u\n", startPos, fileSize);

		if (!dryRun) {
			fs::path fileName = inPath.stem().concat("_0x" + std::to_string(startPos) += ".mpb");
			io::FileIO outFile(outPath / fileName, "wb");
			outFile.writeSpan(inIO.span().subspan(startPos, fileSize));
		}
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
		"NOTE: This is not a magic tool that will always find everything, some things\n"
		"may be compressed, or data may not be stored contingous (so feeding whole game\n"
		"rips may not fully work either)."
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0],
		argv[0]
	);

	return 1;
}
