#include <cstdio>
#include <cstdarg>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/osb.hpp>
#include <manatools/tonedecoder.hpp>
#include <manatools/version.hpp>
#include <manatools/wav.hpp>

#ifndef _WIN32
	#define HEADING "\033[1m"
	#define HEADING_END "\033[0m"
#else
	#define HEADING ""
	#define HEADING_END ""
#endif

#define BOOLSTR(b) (b ? "true" : "false")

namespace fs = manatools::fs;
namespace io = manatools::io;

enum class ToneExportType {
	WAV,
	DAT_TXTH
};

static void printfDepth(uint depth, const char* format, ...) {
	for (uint i = 0; i < depth; i++)
		fputs("    ", stdout);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static void osbVersionCheck(u32 version) {
	if (version != 2) {
		fprintf(stderr, "Unsupported OSB version %u, there may be inaccuracies\n", version);
	}
}

void osbExtractTones(const fs::path& osbPath, const fs::path& outPath, ToneExportType exportType) {
	auto osb = manatools::osb::load(osbPath);

	osbVersionCheck(osb.version);

	for (size_t p = 0; p < osb.programs.size(); p++) {
		const auto& program = osb.programs[p];

		if (!program.tone.data) {
			fprintf(stderr, "Program %zu has no tone data\n", p);
			continue;
		}

		std::string filename = osbPath.stem().string() += '_' + std::to_string(p);

		// oh so messy
		if (exportType == ToneExportType::WAV) {
			manatools::wav::WAV<s16> wavFile(1, program.tone.sampleRate);

			manatools::tone::Decoder decoder(&program.tone);
			wavFile.data.resize(program.tone.samples());
			decoder.decode(wavFile.data);

			wavFile.save(outPath / (filename + ".wav"));
		} else if (exportType == ToneExportType::DAT_TXTH) {
			io::FileIO toneDataFile(outPath / (filename + ".dat"), "wb");
			toneDataFile.writeVec(*program.tone.data);

			io::FileIO txthFile(outPath / (filename + ".dat.txth"), "w");
			std::string txth;
			txth.reserve(120);
			txth += "codec = "       + std::string(manatools::tone::formatName(program.tone.format)) += '\n';
			txth += "channels = 1\n";
			txth += "sample_rate = " + std::to_string(program.tone.sampleRate) += '\n';
			txth += "data_size = "   + std::to_string(program.tone.data->size()) += '\n';
			txth += "num_samples = data_size\n";
			txth += "body_file = "   + filename += ".dat\n";
			txthFile.writeStr(txth);
		}
	}	
}

void osbListInfo(const fs::path& mpbPath) {
	auto osb = manatools::osb::load(mpbPath);

	osbVersionCheck(osb.version);

	printf("Version = %u\n", osb.version);

	for (size_t p = 0; p < osb.programs.size(); p++) {
		const auto& program = osb.programs[p];

		printf(HEADING "Program %zu:\n" HEADING_END, p);
		printfDepth(1, "Base note           = %u\n", program.baseNote);

		printfDepth(1, "Pan pot             = %d\n", program.panPot);
		printfDepth(1, "Direct level        = %u\n", program.directLevel);
		printfDepth(1, "Oscillator level    = %u\n", program.oscillatorLevel);

		printfDepth(1, "Loop?               = %s\n", BOOLSTR(program.loop));
		printfDepth(1, "Loop start          = %u samples\n", program.loopStart);
		printfDepth(1, "Loop end            = %u samples\n", program.loopEnd);
		printfDepth(1, "Loop time           = %u\n", program.loopTime);

		if (program.tone.data) {
			printfDepth(1, "Tone format         = %s\n", manatools::tone::formatName(program.tone.format));
			printfDepth(1, "Tone data size      = %zu samples\n", program.tone.samples());
			printfDepth(1, "Tone data pointer   = %x\n", program.ptrToneData_);
			printfDepth(1, "Tone data use count = %lu\n", program.tone.data.use_count());
		} else {
			printfDepth(1, "No tone data\n");
		}

		printfDepth(1, "FX input channel    = %u\n", program.fx.inputCh);
		printfDepth(1, "FX level            = %u\n", program.fx.level);

		printfDepth(1, "Freq. adjust(?)     = %u\n", program.freqAdjust);
	}
	fputs("Currently, not all OSB information is listed.\n", stderr);
}

// TODO: As with literally every other CLI tool so far, needs better argv parsing
int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "extract")) {
			if (argc < 5)
				goto invalid;

			if (!strcmp(argv[2], "wav")) {
				osbExtractTones(argv[3], argv[4], ToneExportType::WAV);
			} else if (!strcmp(argv[2], "dat+txth")) {
				osbExtractTones(argv[3], argv[4], ToneExportType::DAT_TXTH);
			} else {
				goto invalid;
			}
		} else if (!strcmp(argv[1], "list")) {
			osbListInfo(argv[2]);
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
		"osbtool - Dreamcast One Shot Bank tool [version %s]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s extract <format> <in.osb> <outdir>\n"
		"       %s list <in.osb>\n"
		"\n"
		"Where \"extract\" exports multiple files of <format> to <outdir>.\n"
		"<format> can be one of the following:\n"
		"  - wav - Audio re-encoded into 16-bit PCM.\n"
		"  - dat+txth - Raw audio from the file, with an accompanying vgmstream TXTH\n"
		"    file.\n"
		"\n"
		"An OSB file is a collection of samples used for SFX.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0],
		argv[0]
	);

	return 1;
}
