#pragma once
#include "common.hpp"

/**
 * just so things don't have to touch and link to portaudio themselves, and
 * to make it easier to free its stuff up
 */
class GUICOMMON_EXPORT AudioSystem {
public:
	AudioSystem();
	~AudioSystem();
};
