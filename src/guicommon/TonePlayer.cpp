#include "TonePlayer.hpp"

TonePlayer::TonePlayer(double sampleRate, QObject* parent) :
	QObject(parent),
	maxPadFrames(sampleRate * MAX_PAD_DURATION),
	stream(nullptr),
	padFrames(0),
	endPadFrames(0)
{
	PaError err;

	err = Pa_OpenDefaultStream(
		&stream,
		0,
		1,
		paInt16,
		sampleRate,
		paFramesPerBufferUnspecified,
		&TonePlayer::paStreamCallbackThunk,
		this
	);

	if (err != paNoError)
		return;

	err = Pa_SetStreamFinishedCallback(stream, &TonePlayer::paStreamFinishedThunk);

	if (err != paNoError) {
		Pa_CloseStream(stream);
		stream = nullptr;
	}
}

TonePlayer::~TonePlayer() {
	if (stream) {
		Pa_CloseStream(stream);
	}
}

void TonePlayer::setTone(const Tone& tone) {
	tone_ = tone;
	decoder.setTone(&tone_);
	emit toneChanged();
}

void TonePlayer::play() {
	if (!stream || !decoder.tone())
		return;

	bool wasPlaying = isPlaying();

	decoder.reset();
	padFrames = 0;
	endPadFrames = 0;

	if (!wasPlaying) {
		Pa_StopStream(stream);
	}
	
	PaError err = Pa_StartStream(stream);
	if (err == paNoError && !wasPlaying) {
		emit playingChanged();
	}
}

void TonePlayer::stop() {
	if (stream) {
		Pa_StopStream(stream);
	}
}

int TonePlayer::paStreamCallback(void* output, unsigned long frameCount) {
	s16* out = static_cast<s16*>(output);

	/**
	 * Pad the beginning of the stream to try and prevent PortAudio from cutting it off
	 * depending on system and backend
	 */
	if (padFrames < maxPadFrames) {
		std::fill(out, out + frameCount, 0);
		padFrames += frameCount;
		return paContinue;
	}

	size_t samples = decoder.decode(out, frameCount);

	// Also pad the end, too
	if (samples < frameCount) {
		std::fill(out + samples, out + frameCount, 0);
		endPadFrames += frameCount - samples;

		if (endPadFrames < maxPadFrames) {
			return paContinue;
		}

		return paComplete;
	}

	return paContinue;
}

void TonePlayer::paStreamFinished() {
	emit playingChanged();
}

int TonePlayer::paStreamCallbackThunk(const void* input, void* output, unsigned long frameCount,
	                                  const PaStreamCallbackTimeInfo* timeInfo,
	                                  PaStreamCallbackFlags statusFlags, void* userData)
{
	(void)input;
	(void)timeInfo;
	(void)statusFlags;
	return static_cast<TonePlayer*>(userData)->paStreamCallback(output, frameCount);	
}

void TonePlayer::paStreamFinishedThunk(void* userData) {
	return static_cast<TonePlayer*>(userData)->paStreamFinished();
}
