#include <cstdio>
#include <cstring>

#include <manatools/filesystem.hpp>
#include <manatools/io.hpp>
#include <manatools/osb.hpp>
#include <manatools/tonedecoder.hpp>
#include <manatools/wav.hpp>

namespace fs = manatools::fs;
namespace io = manatools::io;

enum class ToneExportType {
	WAV,
	DAT_TXTH
};

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

		std::string filename = outPath.stem().string() += '_' + std::to_string(p);

		// oh so messy
		if (exportType == ToneExportType::WAV) {
			manatools::wav::WAV<s16> wavFile(1, manatools::tone::SAMPLE_RATE);

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
			txth += "sample_rate = " + std::to_string(manatools::tone::SAMPLE_RATE) += '\n';
			txth += "data_size = "   + std::to_string(program.tone.data->size()) += '\n';
			txth += "num_samples = data_size\n";
			txth += "body_file = "   + filename += ".dat\n";
			txthFile.writeStr(txth);
		}
	}	
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
		"osbtool - Dreamcast One Shot Bank tool [version 0.0.1]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s extract <format> <in.osb> <outdir>\n"
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
		argv[0]
	);

	return 1;
}
