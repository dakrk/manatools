#pragma once
#include <QObject>
#include <portaudio.h>
#include <manatools/tone.hpp>
#include <manatools/tonedecoder.hpp>
#include <manatools/utils.hpp>

class TonePlayer : public QObject {
	Q_OBJECT
public:
	typedef manatools::tone::Tone Tone;

	TonePlayer(double sampleRate, QObject* parent = nullptr);
	~TonePlayer();

	bool isPlaying() const {
		return Pa_IsStreamActive(stream) == 1;
	}

	const Tone& tone() const {
		return tone_;
	}

	void setTone(const Tone& tone);

signals:
	void playingChanged();
	void toneChanged();

public slots:
	void play();
	void stop();

private:
	MT_DISABLE_COPY(TonePlayer)

	static constexpr float MAX_PAD_DURATION = 0.05; // 50ms

	int paStreamCallback(void* output, unsigned long frameCount);
	void paStreamFinished();

	static int paStreamCallbackThunk(const void* input, void* output, unsigned long frameCount,
	                                 const PaStreamCallbackTimeInfo* timeInfo,
	                                 PaStreamCallbackFlags statusFlags, void* userData);

	static void paStreamFinishedThunk(void* userData);

	Tone tone_;
	manatools::tone::Decoder decoder;

	unsigned long maxPadFrames;
	PaStream* stream;
	unsigned long padFrames;
	unsigned long endPadFrames;
};
