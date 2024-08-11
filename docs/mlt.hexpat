#pragma author DarkOK
#pragma description Dreamcast Multi Unit file

#pragma endian little

#define unk padding

namespace mlt {
    struct Header {
        char fourCC[4];
        unk[4]; // I'd *think* this to be a version number, but Sonic Adventure's MLTs begin with 0x0101 and not just 0x1 or 0x2???
        u32 numUnits;
        unk[20];
    };
    
    // Each unit header, including that of the MLT file, should be 32 bytes each.
    struct Unit {
        char fourCC[4];
        u32 bank;         // presumably
        u32 aicaDataPtr;  // location of unit in AICA RAM
        u32 aicaDataSize; // size of unit in AICA RAM
        u32 fileDataPtr;  // location of unit in MLT file
        u32 fileDataSize; // size of unit in MLT file
        unk[8];
        
        // usually on FourCC "SFPW" (DSP work RAM)
        if (fileDataPtr != 0xFFFFFFFF && fileDataSize != 0xFFFFFFFF)
            u8 data[fileDataSize] @ fileDataPtr [[no_unique_address]];
    };

    struct MLT {
        Header header;
        Unit units[header.numUnits];
    };
}

mlt::MLT mlt @ 0x00;
