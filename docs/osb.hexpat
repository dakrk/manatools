#pragma author DarkOK
#pragma description Dreamcast One Shot Bank file

#pragma endian little

import std.sys;

#define unk padding

namespace osb {
    enum LFOWaveType : u8 {
        Saw = 0,
        Square = 1,
        Triangle = 2,
        Noise = 3
    };

    bitfield ProgramFlags {
        ADPCM : 1;
        Loop : 1;
    };

    bitfield ProgramAmpEnvelope {
        attackRate : 5;     // [0 -> 31] [def. 31]
        unk : 1;
        decayRate1 : 5;     // [0 -> 31] [def. 0]
        decayRate2 : 5;     // [0 -> 31] [def. 0]
        releaseRate : 5;    // [0 -> 31] [def. 31]
        decayLevel : 5;     // [0 -> 31] [def. 0]
        keyRateScaling : 4; // [0 -> 15] [def. 0]
        unk : 2;
    };

    std::assert(sizeof(ProgramAmpEnvelope) == 4, "ProgramAmpEnvelope must be 4 bytes");
    
    bitfield ProgramLFO {
        ampDepth : 3;              // [0 -> 7] [def. 0]
        LFOWaveType ampWave : 2;   // [0 -> 3] [def. 0 (Saw)]

        pitchDepth : 3;            // [0 -> 7] [def. 0]
        LFOWaveType pitchWave : 2; // [0 -> 3] [def. 0 (Saw)]

        frequency : 5; // [0 -> 31] [def. 0]
        on : 1;        // [def. false]
    };

    std::assert(sizeof(ProgramLFO) == 2, "ProgramLFO must be 2 bytes");
    
    bitfield ProgramFX {
        inputCh : 4; // [0 -> 15] [def. 0]
        level : 4;   // [0 -> 15] [def. 0]
    };

    std::assert(sizeof(ProgramFX) == 1, "ProgramFX must be 1 byte");

    bitfield ProgramFilterPre {
        resonance : 5; // [0 -> 31] [def. 0]
        off : 1;       // [def. false]
        unk : 2;
    };

    std::assert(sizeof(ProgramFilterPre) == 1, "ProgramFilterPre must be 1 byte");

    struct Header {
        char fourCC[4];
        u32 version;
        u32 fileSize;
        u32 numPrograms;
    };
    
    // Well, I can assume they're called programs. The FourCC ends with 'P'.
    // For the most part, this is similar to MPB.
    struct Program {
        char fourCC[4];
        u8 jump;
        ProgramFlags flags;
        
        u16 ptrToneData; // Pointer starts after SOSD FourCC

        unk[4];

        ProgramAmpEnvelope ampEnvelope;

        unk[2];

        ProgramLFO lfo;
        ProgramFX fx;
        
        unk[1];

        u8 panPot;      // TODO: Not quite sure yet
        u8 directLevel; // [0 -> 15] [def. 15]

        ProgramFilterPre filterPre;

        u8 oscillatorLevel; // [0 -> 255] [def. 255]    Bits inverted, 255 is actually 0. Binary NOT (~) it.

        u16 filterStartLevel;   // [8 -> 8184] [def. 8184]
        u16 filterAttackLevel;  // [8 -> 8184] [def. 8184]
        u16 filterDecayLevel1;  // [8 -> 8184] [def. 8184]
        u16 filterDecayLevel2;  // [8 -> 8184] [def. 8184]
        u16 filterReleaseLevel; // [8 -> 8184] [def. 8184]

        u8 filterDecayRate1;  // [0 -> 31] [def. 25]
        u8 filterAttackRate;  // [0 -> 31] [def. 25]
        u8 filterReleaseRate; // [0 -> 31] [def. 25]
        u8 filterDecayRate2;  // [0 -> 31] [def. 25]
        
        unk[20];

        // u8 toneData[12] @ ptrToneData + 0x10000 * (jump & 0xF);
    };

    struct ProgramPtr {
        Program* data : u32;
    };
    
    struct OSB {
        Header header;
        ProgramPtr programs[header.numPrograms];
    };
}

osb::OSB osb @ 0x00;
