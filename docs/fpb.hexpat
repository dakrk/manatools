#pragma author DarkOK
#pragma description Dreamcast FX Program Bank file

#pragma endian little

import std.io;

#define unk padding

namespace fpb {
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
    };

    struct Program {
        u8 bytes[0xC40]; // TODO
    };

    struct ProgramPtr {
        Program* program : u32;
    };

    struct FPB {
        Header header;
        ProgramPtr programs[header.numPrograms];

        if (header.version >= 2) {
            // std::print("{0:x} ({0})", fpb::calcChecksum(4, header.fileSize - 8));
            u32 checksum @ header.fileSize - 8;
        }
    };
}

fpb::FPB fpb @ 0x00;
