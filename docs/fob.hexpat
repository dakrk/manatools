#pragma author DarkOK
#pragma description Dreamcast FX Output Bank file

#pragma endian little

import std.io;

#define unk padding

// Internally, this is called a mixer file, which contains mixer data
namespace fob {
    const u32 CHANNELS = 16;

    // Panning seemingly same between versions
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
        u32 numMixers;
    };

    struct LevelPanPair {
        u8 level;
        u8 pan;
    };

    struct OutputData {
        LevelPanPair data[CHANNELS];
    };

    struct FOB {
        Header header;
        OutputData* mixers[header.numMixers] : u32;

        if (header.version >= 2) {
            // std::print("{0:x} ({0})", fob::calcChecksum(4, header.fileSize - 8));
            u32 checksum @ header.fileSize - 8;
        }
    };
}

fob::FOB fob @ 0x00;
