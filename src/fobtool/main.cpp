#include <cstdio>
#include <cstring>

#include <manatools/fob.hpp>
#include <manatools/version.hpp>

namespace fs = manatools::fs;

void fobListMixers(const fs::path& path) {
	auto fob = manatools::fob::load(path);

	for (size_t m = 0; m < fob.mixers.size(); m++) {
		const auto& mixer = fob.mixers[m];
		printf("Mixer %zu\n", m);
		printf("    %7s    %5s    %3s\n", "Channel", "Level", "Pan");

		for (size_t c = 0; c < manatools::fob::CHANNELS; c++) {
			printf("    %7zu    %5u    %3d\n", c, mixer.level[c], mixer.pan[c]);
		}

		putchar('\n');
	}
}

int main(int argc, char** argv) {
	try {
		if (argc < 3)
			goto invalid;

		if (!strcmp(argv[1], "list")) {
			fobListMixers(argv[2]);
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
		"fobtool - Dreamcast FX Output/Mixer Bank file tool [version %s]\n"
		"https://github.com/dakrk/manatools\n"
		"\n"
		"Usage: %s list <in.fob>\n"
		"\n"
		"An FOB file holds a collection of mixers, which specify the level and panning\n"
		"that up to 16 FX channels shall be mixed to.\n"
		"\n"
		"The aforementioned usage syntax is not final and will be revised.\n",
		manatools::versionString,
		argv[0]
	);

	return 1;
}
