#pragma author DarkOK
#pragma description Dreamcast MIDI Sequence Bank and Data [INCOMPLETE]

// Sequence bank is little endian, sequence data is big endian (which matches MIDI)
#pragma endian big
#pragma pattern_limit 1000000

import std.core;
import std.io;
import std.string;

#define unk padding
#define BITFIELD_ORDER std::core::BitfieldOrder::MostToLeastSignificant

namespace msd {
    bool _gMoreMessages = true;

    struct Header {
        char fourCC[4];
        unk[4]; // Seems to consistently be the same value
        unk[4]; // Sometimes equal to tempo???
    };
    
    // TODO
    enum Status : u8 {
        Note          = 0x00 ... 0x3F,
        
        Reference     = 0x81,
        Loop          = 0x82,
        EndOfTrack    = 0x83,
        TempoChange   = 0x84,
        
        ControlChange = 0xB0 ... 0xBF,
        ProgramChange = 0xC0 ... 0xCF
    };
    
    /**
     * The control changes I've seen so far map to what MIDI has, so it's safe to presume that so
     * does everything else.
     */
    enum ControlChange : u8 {
        BankSelect_MSB          = 0,
        ModulationWheel_MSB     = 1,
        BreathController_MSB    = 2,
        Undefined_MSB           = 3,
        FootController_MSB      = 4, // pedal?
        PortamentoTime_MSB      = 5,
        DataEntry_MSB           = 6,
        Volume_MSB              = 7,
        Balance_MSB             = 8,
        Undefined_MSB           = 9,
        Pan_MSB                 = 10,
        Expression_MSB          = 11,
        EffectController1_MSB   = 12, // ignored in Dreamcast?
        EffectController2_MSB   = 13, // ignored in Dreamcast?
        Undefined_MSB           = 14 ... 15,
        GeneralPurpose_MSB      = 16 ... 19,
        Undefined_MSB           = 20 ... 31, // used in Dreamcast?

        BankSelect_LSB          = 32, // important!
        ModulationWheel_LSB     = 33,
        BreathController_LSB    = 34,
        Undefined_LSB           = 35,
        FootController_LSB      = 36, // pedal?
        PortamentoTime_LSB      = 37,
        DataEntry_LSB           = 38,
        Volume_LSB              = 39,
        Balance_LSB             = 40,
        Undefined_LSB           = 41,
        Pan_LSB                 = 42,
        Expression_LSB          = 43,
        EffectController1_LSB   = 44, // ignored in Dreamcast?
        EffectController2_LSB   = 45, // ignored in Dreamcast?
        Undefined_LSB           = 46 ... 47,
        GeneralPurpose_LSB      = 48 ... 51, // used in Dreamcast?
        Undefined_LSB           = 52 ... 63, // used in Dreamcast?

        // TODO: There's a lot more
        
        ResetAllControllers     = 121
    };
    
    bitfield StatusBits {
        type : 4;
        channel : 4;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];
    
    bitfield ControlChangeBits {
        step16Bit : 1;
        ControlChange type : 7;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];
    
    bitfield BankSelectBits {
        unk : 4;
        bank : 4;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];
    
    bitfield ProgramChangeBits {
        step16Bit : 1;
        voice : 7;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];
    
    // BPM = 60 * 1e3 / tempo
    // Tempo in MIDI is usually from usecs/quarter note, but in MSD it's msecs? (1e6 vs 1e3)
    struct Message {
        Status status;
        StatusBits statusBits @ $ - 1;
        
        // Messages that associate to a channel (the ones with ranges in the enum)
        bool messageHandled = true;
        match (u8(status) & 0xF0) {
            // TODO: Confirm how note off is handled
            (Status::Note | 0x00 ... 0x3F): {
                u8 key;
                u8 volume;
                
                match (statusBits.type) {
                    (0x0): {
                        u8 gate;
                        u8 step;
                    }
                    
                    (0x1): {
                        u8 gate;
                        u16 step;
                    }
                    
                    (0x2): {
                        u16 gate;
                        u8 step;
                    }
                    
                    (0x3): {
                        u16 gate;
                        u16 step;
                    }
                    
                    (_): {
                        std::error("How did I get here?!?!");
                    }
                }
            }
            
            (Status::ControlChange): {
                ControlChangeBits ccData;
                
                match (ccData.type) {
                    (ControlChange::BankSelect_LSB): {
                        BankSelectBits bsData;
                    }
                    
                    (_): {
                        u8 value;
                        // _gMoreMessages = false;
                        // I think Control Change messages would always be 1 byte so warn instead of error
                        std::warning(std::format("Unhandled MSD CC message at 0x{0:x}: 0x{1:x} ({1})", $ - 3, u8(ccData.type)));
                    }
                }
                
                if (ccData.step16Bit)
                    u16 step;
                else
                    u8 step;
            }
            
            (Status::ProgramChange): {
                ProgramChangeBits pcData;
                
                if (pcData.step16Bit)
                    u16 step;
                else
                    u8 step;
            }
            
            (_): {
                messageHandled = false;
            }
        }
        
        if (messageHandled)
            return;
            
        // Messages that don't associate to a channel (the ones that don't have a range in the enum)
        match (u8(status)) {
            (Status::Reference): {
                u16 offset;
                u8 length;
            }
            
            (Status::Loop): {
                u8 mode;
                
                /**
                 * The way this works is like MIDI variable-length quantity, but mode and step are unrelated
                 * so we can't just use that
                 */
                if (mode < 0x80)
                    u8 step;
                else
                    u16 step;
            }
            
            (Status::EndOfTrack): {
                _gMoreMessages = false;
            }
            
            (Status::TempoChange): {
                u16 tempo;
                unk[1];
            }

            // Cannot reliably continue, message would be of unknown length
            (_): {
                // _gMoreMessages = false;
                std::error(std::format("Unknown/unhandled MSD message: 0x{0:x} ({0})", u8(status)));
            }
        }
    };

    struct MSD {
        Header header;
        Message messages[while (_gMoreMessages)];

        // Reset for next MSD
        _gMoreMessages = true;
    };
}

namespace msb {
    struct Header {
        char fourCC[4];
        le u32 version;
        le u32 fileSize;
        le u32 numSequences;
    };

    struct MSDPtr {
        msd::MSD* data : le u32;
    };

    struct MSB {
        Header header;
        MSDPtr sequences[header.numSequences];
    };
}

msb::MSB msb @ 0x00;
