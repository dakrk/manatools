#include <cstdio>
#include <cstring>
#include <manatools/version.hpp>

#include "operations.hpp"

/**
 * TODO: This is atrocious and desperately in need of much better argument parsing code
 * (and only then can I start working on making usage of this nicer)
 */
int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "convert")) {
			if (argc < 4)
				goto invalid;

			mpbExportSF2(argv[2], argv[3]);
		} else if (!strcmp(argv[1], "extract")) {
			if (argc < 5)
				goto invalid;

			if (!strcmp(argv[2], "wav")) {
				mpbExtractTones(argv[3], argv[4], ToneExportType::WAV);
			} else if (!strcmp(argv[2], "dat+txth")) {
				mpbExtractTones(argv[3], argv[4], ToneExportType::DAT_TXTH);
			} else {
				goto invalid;
			}
		} else if (!strcmp(argv[1], "list")) {
			mpbListInfo(argv[2]);
		} else {
			goto invalid;
		}
	} catch (std::ios_base::failure& err) {
		// iostream's own exception messages are less helpful than this so
		fprintf(stderr, "An error occurred: Failed to handle file\n");
		return 1;
	} catch (std::runtime_error& err) {
		fprintf(stderr, "An error occurred: %s\n", err.what());
		return 1;
	}

	return 0;

invalid:
	fprintf(
		stderr,
		"mpbtool - Dreamcast MIDI Program Bank tool [version %s]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s convert <in.mpb> <out.sf2>\n"
		"       %s extract <format> <in.mpb> <outdir>\n"
		"       %s list <in.mpb>\n"
		"\n"
		"Where \"extract\" exports multiple files of <format> to <outdir>.\n"
		"<format> can be one of the following:\n"
		"  - wav - Audio re-encoded into 16-bit PCM. Contains embedded loop data.\n"
		"  - dat+txth - Raw audio from the file, with an accompanying vgmstream TXTH\n"
		"    file containing loop data.\n"
		"\n"
		"An MPB file is the bank of instruments and samples (called \"tones\") used\n"
		"for music playback and SFX.\n"
		"\n"
		"An MDB (MIDI Drum Bank) file can also take place of an MPB file, as they both\n"
		"have the same file format internally.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0],
		argv[0],
		argv[0]
	);

	return 1;
}
