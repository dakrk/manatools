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
    
    /**
     * "Every DSP register described here is a 32-bit register, but only the lowest 16 bits are used."
     * So in this bitfield we just place 16 bits of padding every other 16 bits.
     * 
     * Corlett's docs say that TRA should be up to the 64th bit, however having the bits in that order in
     * this bitfield makes the `unused` fields be non-zero. Endianness fun? Not sure if this is correct.
     */
    bitfield DSPInstruction {
        unused3 : 1;
        TWA : 7;
        TWT : 1;
        TRA : 7;
        
        padding : 16;
        
        unused2 : 1;
        IWA : 5;
        IWT : 1;
        IRA : 6;
        YSEL : 2;
        XSEL : 1;
        
        padding : 16;

        BSEL : 1;
        ZERO : 1;
        NEGB : 1;
        YRL : 1;
        SHFT : 2;
        FRCL : 1;
        ADRL : 1;
        EWA : 4;
        EWT : 1;
        MRD : 1;
        MWT : 1;
        TABLE : 1;
        
        padding : 16;
        
        unused : 7;
        NXADR : 1;
        ADREB : 1;
        MASA : 6;
        NOFL : 1;
        
        padding : 16;

        // These should be zero because they're apparently unused
        if (unused || unused2 || unused3) {
            std::warning(std::format("Possible invalid instruction at 0x{0:x}", $ - 16));
        }
    };

    struct Header {
        char fourCC[4];
        u32 version;
        u32 fileSize;
        u32 numPrograms;
    };

    struct Program {
        u8 header[0x40]; // Not sure what else can be in here other than the FPD name and some rogue number
        u32 COEF[128];
        u32 MADRS[64];
        unk[0x100];
        DSPInstruction MPRO[128];
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
