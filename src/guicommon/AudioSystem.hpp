#pragma once

/**
 * just so things don't have to touch and link to portaudio themselves, and
 * to make it easier to free its stuff up
 */
class AudioSystem {
public:
	AudioSystem();
	~AudioSystem();
};
