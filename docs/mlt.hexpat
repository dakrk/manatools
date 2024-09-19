#pragma author DarkOK
#pragma description Dreamcast Multi Unit file

#pragma endian little

#define unk padding

namespace mlt {
    struct Header {
        char fourCC[4];
        u32 version; // Weirdly, version 1 is 0x0101, not 0x1
        u32 numUnits;
        unk[20]; // Usually filled with FFs, but SGGG doesn't?
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
        
        /**
         * Some units are there only as a mapping to where stuff shall be in AICA RAM,
         * and as such have no data in the file.
         * SFPW units (DSP work RAM) are expected to have no file data. 
         */
        if (fileDataPtr != 0xFFFFFFFF && fileDataSize != 0xFFFFFFFF) {
            u8 data[fileDataSize] @ fileDataPtr [[no_unique_address]];
        }
    };

    struct MLT {
        Header header;
        Unit units[header.numUnits];
    };
}

mlt::MLT mlt @ 0x00;
