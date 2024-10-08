#pragma author DarkOK
#pragma description Dreamcast MIDI Program Bank file (rather incomplete)

#pragma endian little
#pragma loop_limit 1000000

import std.ptr;
import std.sys;

#define unk padding

namespace mpb {
    enum LFOWaveType : u8 {
        Saw = 0,
        Square = 1,
        Triangle = 2,
        Noise = 3
    };
    
    bitfield SplitFlags {
        ADPCM : 1;
        Loop : 1;
        Noise : 1;
        unk : 3;
        KYONB : 1; // Found in some version 1 files
        KYONEX : 1; // ^^
    };

    bitfield SplitAmpEnvelope {
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

    std::assert(sizeof(SplitAmpEnvelope) == 4, "SplitAmpEnvelope must be 4 bytes");
    
    bitfield SplitLFO {
        ampDepth : 3;              // [0 -> 7] [def. 0]
        LFOWaveType ampWave : 2;   // [0 -> 3] [def. 0 (Saw)]

        pitchDepth : 3;            // [0 -> 7] [def. 0]
        LFOWaveType pitchWave : 2; // [0 -> 3] [def. 0 (Saw)]

        frequency : 5; // [0 -> 31] [def. 0]
        sync : 1;      // [def. false]
    };

    std::assert(sizeof(SplitLFO) == 2, "SplitLFO must be 2 bytes");

    // Haven't quite tested this yet, but it is in AICA docs
    bitfield SplitPitch {
        FNS : 11;
        OCT : 4;
        unk : 1;
    };

    std::assert(sizeof(SplitPitch) == 2, "SplitPitch must be 2 bytes");

    bitfield SplitFX {
        inputCh : 4; // [0 -> 15] [def. 0]
        level : 4;   // [0 -> 15] [def. 0]
    };

    std::assert(sizeof(SplitFX) == 1, "SplitFX must be 1 byte");
    
    bitfield SplitFilterPre {
        resonance : 5; // [0 -> 31] [def. 4]
        off : 1;       // [def. false]
        voff : 1;
        unk : 1;
    };

    std::assert(sizeof(SplitFilterPre) == 1, "SplitFilterPre must be 1 byte");

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
        u32 version; // only supporting 2 at the moment
        u32 fileSize; // size of file, including header
        unk[4]; // or is the size a u64? (unlikely, not a 64-bit platform)
        
        u32 ptrPrograms;
        u32 numPrograms;
        
        u32 ptrVelocities;
        u32 numVelocities;
        
        u32 ptrUnk1;
        u32 numUnk1;
        u32 ptrUnk2;
        u32 numUnk2;
    };
    
    /**
     * Max 128 Splits per Layer.
     * Everything up until startNote maps directly to AICA registers.
     */
    struct Split {
        u8 jump; // `(jump & 0x80)` if the data is 8-bit PCM. Otherwise 16-bit if not ADPCM or 8-bit PCM.
        SplitFlags flags; // [def. ADPCM]

        u16 ptrToneData;

        /**
         * loopEnd is treated as the tone data length, even with loop disabled.
         * Setting it to 0 or FFFF seems like it tries to guess the size, but 
         * there can be noise at the end of the sample.
         */
        u16 loopStart; // [0 -> 65535] [def. 0]             Minimum seems to be bumped to 4 after exporting when looping is enabled.
        u16 loopEnd;   // [0 -> 65535] [def. (tone end)]

        SplitAmpEnvelope ampEnvelope;
        
        SplitPitch pitch;
        SplitLFO lfo;
        SplitFX fx;
        
        unk[1];
        
        /**
         * Pan pot ranges are usually different between versions, presumably
         * due to a quirk in the SDK tools. Both versions' variations are
         * compatible with each other though. See calcPanPot.
         */
        u8 panPot;      // [16 -> 47] [def. 32 (Center)] (typical version 2 range)
        u8 directLevel; // [0 -> 15] [def. 15]
        
        SplitFilterPre filterPre;
        
        /**
         * This is an attenuation level, so 0 means full volume, while 255 is
         * lowest. Tools invert this to be clearer (a binary NOT should work). 
         */
        u8 oscillatorLevel; // [0 -> 255] [def. 255]
        
        u16 filterStartLevel;   // [0 -> 8184] [def. 8184]    The slider in the editor only goes down to 8, but typing can go down to 0.
        u16 filterAttackLevel;  // [0 -> 8184] [def. 8184]    ^^
        u16 filterDecayLevel1;  // [0 -> 8184] [def. 8184]    ^^
        u16 filterDecayLevel2;  // [0 -> 8184] [def. 8184]    ^^
        u16 filterReleaseLevel; // [0 -> 8184] [def. 8184]    ^^
        
        u8 filterDecayRate1;  // [0 -> 31] [def. 25]
        u8 filterAttackRate;  // [0 -> 31] [def. 25]
        u8 filterReleaseRate; // [0 -> 31] [def. 25]
        u8 filterDecayRate2;  // [0 -> 31] [def. 25]
        
        u8 startNote; // [0 -> 127] [def. 0 (C-2)]
        u8 endNote;   // [0 -> 127] [def. 127 (G8)]
        u8 baseNote;  // [0 -> 127] [def. 60 (C3)]
        
        /**
         * Fine tune is represented in the editor as -64 -> 63, half that of 
         * the file representation.
         * This seems to map to approximately -48 -> 47 cents.
         * TODO: Check what's in AICA RAM, as the calculation there is known.
         */
        s8 fineTune; // [-128 -> 127] [def. 0]
        unk[2];
        
        u8 velocityCurveID;
        u8 velocityLow;  // [0 -> 127] [def. 1]    Oddly, the default in the editor is 0, but exported is 1.
        u8 velocityHigh; // [0 -> 127] [def. 127]  Same here, but the exported value is 127 instead of 0.
        
        bool drumMode;  // [def. false]
        u8 drumGroupID; // [0 -> 255] [def. 0]

        unk[1];

        // I don't think I can do the length calculations in just an ImHex pattern, gah
        // u8 toneData[12] @ ptrToneData + ((jump & 0x7F) << 16);
    };
    
    // Max 4 Layers per Program
    struct Layer {
        u32 numSplit;
        u32 ptrSplit;
        
        /**
         * Layer delay is represented as 0 -> 4096, which means you have to 
         * quarter it to fit in the file's 0 -> 1024 range. Strange.
         */ 
        u16 layerDelay; // [0 -> 1024] [def. 0]

        unk[2];
        
        u8 bendRangeHigh; // [0 -> 24] [def. 2]
        u8 bendRangeLow;  // [24 <- 0] [def. 2]

        unk[2];
        
        Split splits[numSplit] @ ptrSplit;
    };
    
    // ugh
    struct LayerPtr {
        //std::ptr::NullablePtr<Layer, u32> layer;
        u32 ptr [[no_unique_address, hidden]];

        // Manatee doesn't stop reading upon a nullptr, it continues reading layers (i.e., you can start a layer at index 2)
        if (ptr)
            Layer *data : u32;
        else
            padding[4];
    };
    
    // Max 128 Programs per Bank
    struct Program {
        LayerPtr layers[4];
    };

    struct ProgramPtr {
        Program* data : u32;
    };

    struct Velocity {
        u8 data[128];
    };
    
    struct MPB {
        Header header;
        
        ProgramPtr programs[header.numPrograms] @ header.ptrPrograms;
        Velocity velocities[header.numVelocities] @ header.ptrVelocities;
        
        if (header.version >= 2) {
            // std::print("{0:x} ({0})", mpb::calcChecksum(4, header.fileSize - 8));
            u32 checksum @ header.fileSize - 8;
        }
    };
}

mpb::MPB mpb @ 0x00;
