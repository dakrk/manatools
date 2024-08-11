#pragma once
#include "filesystem.hpp"

enum class ToneExportType {
	WAV,
	DAT_TXTH
};

void mpbExportSF2(const fs::path& mpbPath, const fs::path& sf2Path);
void mpbExtractTones(const fs::path& mpbPath, const fs::path& wavOutPath, ToneExportType exportType);
void mpbListInfo(const fs::path& mpbPath);
