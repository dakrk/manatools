#pragma once
#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "filesystem.hpp"
#include "io.hpp"
#include "types.hpp"

namespace manatools::wav {
	struct SamplerChunk {
		struct Loop {
			enum class Type : u32 {
				Forward     = 0,
				Alternate   = 1,
				Backward    = 2
			};

			Type type     = Type::Forward;
			u32 start     = 0; // In samples
			u32 end       = 0; // In samples
			u32 playCount = 0; // 0 is infinite
		};

		u32 midiUnityNote     = 60; // Seemingly baseNote
		u32 midiPitchFraction = 0;  // Seemingly fineTune

		std::vector<Loop> loops;
	};

	/**
	 * Only supports PCM. ADPCM can't loop seamlessly with WAV :(
	 * Unfortunate, as software I've tried can straight up play Yamaha ADPCM so long as the WAV 
	 * format tag is 0x20, including FL Studio, therefore making re-encoding less necessary.
	 */
	template <std::integral T>
	class WAV {
	public:
		WAV(u16 channels, u32 sampleRate) : channels(channels), sampleRate(sampleRate) {
			// no need for stereo at the moment
			if (channels < 1)
				throw std::invalid_argument("WAV channels cannot be less than 1");
			else if (channels > 1)
				throw std::invalid_argument("Only mono WAVs are currently supported");
		}

		void save(const fs::path& path, bool swapBytes = true);

		size_t bitdepth() const { return sizeof(T) * 8; }

		u16 channels;
		u32 sampleRate;
		std::vector<T> data;

		std::optional<SamplerChunk> sampler;
	};

	template <std::integral T>
	inline void WAV<T>::save(const fs::path& path, bool swapBytes) {
		io::FileIO io(path, "wb");
		
		io.writeStr("RIFF");

		// Not worth it creating a proper RIFF implementation until loading WAVs is a feature.
		// Just hopping back and forth will do for now.
		auto riffChunkSizePos = io.tell();
		io.writeU32LE(0); // Will be replaced with proper size

		io.writeStr("WAVE");
		{
			io.writeStr("fmt ");                       //    chunkId:
			io.writeU32LE(16);                         //  chunkSize:
			io.writeU16LE(1);                          //     format: PCM = 1
			io.writeU16LE(1);                          //   channels: Only support mono (1) currently
			io.writeU32LE(sampleRate);                 // sampleRate: 22050 Hz, 44100 Hz, etc
			io.writeU32LE(sampleRate * 1 * sizeof(T)); //   byteRate: sampleRate * channels * (bitdepth / 8)
			io.writeU16LE(1 * sizeof(T));              // blockAlign: channels * (bitdepth / 8)
			io.writeU16LE(bitdepth());                 //   bitdepth: sizeof(SampleType) * 8

			io.writeStr("data");                       //    chunkId:
			io.writeU32LE(data.size() * sizeof(T));    //  chunkSize: Size in bytes

			if (!swapBytes || std::endian::native == std::endian::little || sizeof(T) == 1) {
				io.writeVec(data);
			} else {
				// byte swapping fun time
				// buffer it as to not hammer fwrite (even though that should buffer stuff already)
				T buf[512];

				size_t dataI = 0;
				while (dataI != data.size()) {
					size_t bufI = 0;
					while (bufI < std::size(buf) && dataI < data.size()) {
						buf[bufI++] = LE(data[dataI++]);
					}

					io.write(buf, sizeof(T), bufI);
				}
			}

			// blocks must be padded to be even, apparently
			if ((data.size() * sizeof(T)) & 1)
				io.writeU8(0);

			if (sampler) {
				io.writeStr("smpl");                                //           chunkId:
				io.writeU32LE(36 + (sampler->loops.size() * 24));   //         chunkSize:
				io.writeU32LE(0);                                   //      manufacturer:
				io.writeU32LE(0);                                   //           product:
				io.writeU32LE((1. / sampleRate) * 1e9);             //      samplePeriod: Period of 1 sample in nanoseconds
				io.writeU32LE(sampler->midiUnityNote);              //     midiUnityNote: Seemingly baseNote
				io.writeU32LE(sampler->midiPitchFraction);          // midiPitchFraction: Seemingly fineTune
				io.writeU32LE(0);                                   //       smpteFormat:
				io.writeU32LE(0);                                   //       smpteOffset:
				io.writeU32LE(sampler->loops.size());               //       sampleLoops: Number of loops
				io.writeU32LE(0);                                   //       samplerData:
				for (size_t i = 0; i < sampler->loops.size(); i++) {
					const auto& loop = sampler->loops[i];
					io.writeU32LE(i);                           // identifier: (hence the more verbose for loop)
					io.writeU32LE(static_cast<u32>(loop.type)); //       type:
					io.writeU32LE(loop.start);                  //      start: In samples
					io.writeU32LE(loop.end);                    //        end: In samples
					io.writeU32LE(0);                           //   fraction:
					io.writeU32LE(loop.playCount);              //  playCount: Number of times to play the loop (0 is infinite)
				}
			}
		}

		auto size = io.tell() - (riffChunkSizePos + 4);
		io.jump(riffChunkSizePos);
		io.writeU32LE(size);
	}
} // namespace manatools::wav
