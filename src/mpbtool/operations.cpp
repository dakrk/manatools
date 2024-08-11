#include <cmath>
#include <cstdarg>
#include <string>

#include <manatools/io.hpp>
#include <manatools/mpb.hpp>
#include <manatools/tone.hpp>
#include <manatools/tonedecoder.hpp>
#include <manatools/wav.hpp>
#include <manatools/sf2.hpp>

#include "operations.hpp"
#include "filesystem.hpp"

namespace io = manatools::io;

static void mpbVersionCheck(u32 version) {
	if (version != 2) {
		fprintf(stderr, "Unsupported MPB version %u, there may be inaccuracies\n", version);
	}
}

void mpbExportSF2(const fs::path& mpbPath, const fs::path& sf2Path) {
	auto mpb = manatools::mpb::load(mpbPath);
	mpbVersionCheck(mpb.version);

	auto sf2 = manatools::sf2::fromMPB(mpb, mpbPath.stem().string());
	sf2.Write(sf2Path.string());
}

void mpbExtractTones(const fs::path& mpbPath, const fs::path& outPath, ToneExportType exportType) {
	auto mpb = manatools::mpb::load(mpbPath);
	mpbVersionCheck(mpb.version);

	for (size_t p = 0; p < mpb.programs.size(); p++) {
		const auto& program = mpb.programs[p];

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];
			if (!layer)
				continue;

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];

				if (!split.tone.data) {
					fprintf(stderr, "%zu:%zu:%zu has no tone data\n", p + 1, l + 1, s + 1);
					continue;
				}

				/**
				 * In this case I'd rather choose slightly more messy and less efficient concatenation
				 * rather than 100KB+ (and noticibly ever incrementing) code bloat (ahem, std::format)
				 */
				std::string filename = mpbPath.stem().string() += '_';
				filename += std::to_string(p + 1) += '-';
				filename += std::to_string(l + 1) += '-';
				filename += std::to_string(s + 1);

				if (exportType == ToneExportType::WAV) {
					manatools::wav::WAV<s16> wavFile(1, manatools::tone::SAMPLE_RATE);

					if (split.loop) {
						wavFile.sampler = {
							.midiUnityNote = 60,
							.midiPitchFraction = 0,
							.loops = {
								{
									.start = split.loopStart,
									.end = split.loopEnd - 1u
								}
							}
						};
					}

					manatools::tone::Decoder decoder(&split.tone);
					wavFile.data.resize(split.tone.samples());
					decoder.decode(wavFile.data);
					
					wavFile.save(outPath / (filename + ".wav"));
				} else if (exportType == ToneExportType::DAT_TXTH) {
					io::FileIO toneDataFile(outPath / (filename + ".dat"), "wb");
					toneDataFile.writeVec(*split.tone.data);

					io::FileIO txthFile(outPath / (filename + ".dat.txth"), "w");
					std::string txth; // ditto: see the earlier formatting related comment
					txth.reserve(175);
					txth += "codec = "       + std::string(manatools::tone::formatName(split.tone.format)) += '\n';
					txth += "channels = 1\n";
					txth += "sample_rate = " + std::to_string(manatools::tone::SAMPLE_RATE) += '\n';
					txth += "data_size = "   + std::to_string(split.tone.data->size()) += '\n';
					txth += "num_samples = data_size\n";
					txth += "loop_flag = "   + std::string(split.loop ? "1" : "0") += '\n';
					txth += "loop_start = "  + std::to_string(split.loopStart) += '\n';
					txth += "loop_end = "    + std::to_string(split.loopEnd) += '\n';
					txth += "body_file = "   + filename += ".dat\n";
					txthFile.writeStr(txth);
				}
			}
		}
	}
}

static void printfDepth(uint depth, const char* format, ...) {
	for (uint i = 0; i < depth; i++)
		fputs("    ", stdout);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

#ifndef _WIN32
	#define HEADING "\033[1m"
	#define HEADING_END "\033[0m"
#else
	// forgot Windows conhost sucks
	#define HEADING ""
	#define HEADING_END ""
#endif

#define BOOLSTR(b) (b ? "true" : "false")

void mpbListInfo(const fs::path& mpbPath) {
	auto mpb = manatools::mpb::load(mpbPath);

	mpbVersionCheck(mpb.version);

	printf("Version   = %u\n", mpb.version);
	printf("Drum bank = %s\n", BOOLSTR(mpb.drum));

	for (size_t p = 0; p < mpb.programs.size(); p++) {
		const auto& program = mpb.programs[p];

		printf(HEADING "Program %zu:\n" HEADING_END, p + 1);

		for (size_t l = 0; l < program.layers.size(); l++) {
			const auto& layer = program.layers[l];
			if (!layer)
				continue;

			printfDepth(1, HEADING "Layer %zu:\n" HEADING_END, l + 1);
			printfDepth(2, "Delay = %u\n", layer->delay);
			printfDepth(2, "Bend range [+] = %u\n", layer->bendRangeHigh);
			printfDepth(2, "Bend range [-] = %u\n", layer->bendRangeLow);

			for (size_t s = 0; s < layer->splits.size(); s++) {
				const auto& split = layer->splits[s];

				printfDepth(2, HEADING "Split %zu:\n" HEADING_END, s + 1);

				printfDepth(3, "Start note          = %u\n", split.startNote);
				printfDepth(3, "End note            = %u\n", split.endNote);
				printfDepth(3, "Base note           = %u\n", split.baseNote);
				printfDepth(3, "Fine tune           = %u\n", split.fineTune);

				printfDepth(3, "Pan pot             = %d\n", split.panPot);
				printfDepth(3, "Direct level        = %u\n", split.directLevel);

				printfDepth(3, "Loop?               = %s\n", BOOLSTR(split.loop));
				printfDepth(3, "Loop start          = %u samples\n", split.loopStart);
				printfDepth(3, "Loop end            = %u samples\n", split.loopEnd);

				if (split.tone.data) {
					printfDepth(3, "Tone format         = %s\n", manatools::tone::formatName(split.tone.format));
					printfDepth(3, "Tone data size      = %zu samples\n", split.tone.samples());
					printfDepth(3, "Tone data pointer   = %x\n", split.ptrToneData_);
					printfDepth(3, "Tone data use count = %lu\n", split.tone.data.use_count());
				} else {
					printfDepth(3, "No tone data\n");
				}

				printfDepth(3, "Velocity curve ID   = %u\n", split.velocityCurveID);
				printfDepth(3, "Velocity low        = %u\n", split.velocityLow);
				printfDepth(3, "Velocity high       = %u\n", split.velocityHigh);

				printfDepth(3, "Drum mode?          = %s\n", BOOLSTR(split.drumMode));
				printfDepth(3, "Drum group ID       = %u\n", split.drumGroupID);

				// TODO: Checksum?
			}
		}
	}
	fputs("Currently, not all MPB information is listed.\n", stderr);
}
