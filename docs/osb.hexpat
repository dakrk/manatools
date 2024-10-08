#pragma author DarkOK
#pragma description Dreamcast One Shot Bank file

#pragma endian little

import std.sys;

#define unk padding

namespace osb {
    u32 g_currentVersion;

    enum LFOWaveType : u8 {
        Saw = 0,
        Square = 1,
        Triangle = 2,
        Noise = 3
    };

    bitfield ProgramFlags {
        ADPCM : 1;
        Loop : 1;
        Noise : 1;
        unk : 3;
        KYONB : 1; // Found in some version 1 files
        KYONEX : 1; // ^^
    };

    bitfield ProgramAmpEnvelope {
        attackRate : 5;     // [0 -> 31] [def. 31]
        unk : 1;
        decayRate1 : 5;     // [0 -> 31] [def. 0]
        decayRate2 : 5;     // [0 -> 31] [def. 0]    AICA docs say sustain?
        releaseRate : 5;    // [0 -> 31] [def. 31]
        decayLevel : 5;     // [0 -> 31] [def. 0]
        keyRateScaling : 4; // [0 -> 15] [def. 0]
        LPSLNK : 1;
        unk : 1;
    };

    std::assert(sizeof(ProgramAmpEnvelope) == 4, "ProgramAmpEnvelope must be 4 bytes");
    
    bitfield ProgramLFO {
        ampDepth : 3;              // [0 -> 7] [def. 0]
        LFOWaveType ampWave : 2;   // [0 -> 3] [def. 0 (Saw)]

        pitchDepth : 3;            // [0 -> 7] [def. 0]
        LFOWaveType pitchWave : 2; // [0 -> 3] [def. 0 (Saw)]

        frequency : 5; // [0 -> 31] [def. 0]
        sync : 1;      // [def. false]
    };

    std::assert(sizeof(ProgramLFO) == 2, "ProgramLFO must be 2 bytes");

    // Haven't quite tested this yet, but it is in AICA docs
    bitfield ProgramPitch {
        FNS : 11;
        OCT : 4;
        unk : 1;
    };

    std::assert(sizeof(ProgramPitch) == 2, "ProgramPitch must be 2 bytes");

    bitfield ProgramFX {
        inputCh : 4; // [0 -> 15] [def. 0]
        level : 4;   // [0 -> 15] [def. 0]
    };

    std::assert(sizeof(ProgramFX) == 1, "ProgramFX must be 1 byte");

    bitfield ProgramFilterPre {
        resonance : 5; // [0 -> 31] [def. 0]
        off : 1;       // [def. false]
        voff : 1;
        unk : 1;
    };

    std::assert(sizeof(ProgramFilterPre) == 1, "ProgramFilterPre must be 1 byte");

    /**
     * Unlike MPB, OSB pan values are usually the same between version 1 and 2.
     * However, due to an SDK tool quirk, panning fully left isn't possible.
     * (It considers 0x10 to be -1, and not 0.)
     */
    fn calcPanPot(u8 panIn) {
        return (panIn & 0x10) ? -s8(panIn & 0xF) : (panIn & 0xF);
    };

    fn calcChecksum(u32 start, u32 end) {
        u32 newChecksum;
        
        for (u32 i = start, i < end, i += sizeof(u8)) {
            u8 d @ i;
            newChecksum += d;
        }
        
        return newChecksum;
    };

    struct Header {
        char fourCC[4];
        u32 version;
        u32 fileSize;
        u32 numPrograms;

        g_currentVersion = version;
    };
    
    /**
     * Well, I can assume they're called programs. The FourCC ends with 'P'.
     * Just like MPB, maps directly to AICA registers, up until loopTime.
     */
    struct Program {
        char fourCC[4];
        u8 jump;
        ProgramFlags flags;
        
        u16 ptrToneData; // Pointer starts after SOSD FourCC

        /**
         * In OSB, loopEnd should effectively always be full length, 
         * as seemingly no SDK tools exposed changing these.
         */
        u16 loopStart; // [0 -> 65535]
        u16 loopEnd;   // [0 -> 65535]

        ProgramAmpEnvelope ampEnvelope;

        ProgramPitch pitch;
        ProgramLFO lfo;
        ProgramFX fx;
        
        unk[1];

        u8 panPot;      // [0 -> 30] [def. 0 (Center)]    See calcPanPot for more information.
        u8 directLevel; // [0 -> 15] [def. 15]

        ProgramFilterPre filterPre;

        /**
         * This is an attenuation level, so 0 means full volume, while 255 is
         * lowest. Tools invert this to be clearer (a binary NOT should work). 
         */
        u8 oscillatorLevel; // [0 -> 255] [def. 0]

        u16 filterStartLevel;   // [8 -> 8184] [def. 8184]
        u16 filterAttackLevel;  // [8 -> 8184] [def. 8184]
        u16 filterDecayLevel1;  // [8 -> 8184] [def. 8184]
        u16 filterDecayLevel2;  // [8 -> 8184] [def. 8184]
        u16 filterReleaseLevel; // [8 -> 8184] [def. 8184]

        u8 filterDecayRate1;  // [0 -> 31] [def. 25]
        u8 filterAttackRate;  // [0 -> 31] [def. 25]
        u8 filterReleaseRate; // [0 -> 31] [def. 25]
        u8 filterDecayRate2;  // [0 -> 31] [def. 25]

        match (g_currentVersion) {
            (1): {
                /**
                 * TODO: Not sure what this is measured in.
                 * Same as version 2? Haven't yet tested.
                 */
                u8 loopTime;

                unk[1];

                /**
                 * Seems very likely, 60 is 44100Hz and going up an octave to
                 * 72 is 22050Hz.
                 */
                u8 baseNote;

                /**
                 * Not sure what this is representing yet or if it's only 1
                 * byte, but this is present when a program uses an unusual
                 * sample rate as so frequency can be adjusted to finer values
                 * than what just baseNote can represent.
                 * See issue #16.
                 */
                u8 freqAdjust;

                /**
                 * Same as version 2, although I haven't yet tried testing any
                 * differences.
                 */
                unk[12];
            }
            
            (2): {
                /**
                 * Time in (msecs / 4) until a note off is fired.
                 * Amp envelope is effective.
                 */
                u32 loopTime;

                /**
                 * These are the same as version 1, except that freqAdjust is
                 * represented differently.
                 */
                u8 baseNote;
                u8 freqAdjust;

                /**
                 * I've only observed a repeat of loopEnd being here so far,
                 * with the rest being zeroes. Setting these to other values
                 * does not seem to have any audible difference.
                 */
                unk[18];
            }
        }

        // u8 toneData[12] @ ptrToneData + ((jump & 0x7F) << 16);
    };

    struct ProgramPtr {
        Program* data : u32;
    };
    
    struct OSB {
        Header header;
        ProgramPtr programs[header.numPrograms];

        if (header.version >= 2) {
            //std::print("{0:x} ({0})", osb::calcChecksum(4, header.fileSize - 8));
            u32 checksum @ header.fileSize - 8;
        }
    };
}

osb::OSB osb @ 0x00;
