#include <QMessageBox>
#include <portaudio.h>
#include "AudioSystem.hpp"

AudioSystem::AudioSystem() {
	PaError err = Pa_Initialize();
	if (err != paNoError) {
		QMessageBox::warning(nullptr, "Failed to initialize audio", Pa_GetErrorText(err));
	}
}

AudioSystem::~AudioSystem() {
	Pa_Terminate();
}
