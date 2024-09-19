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

        /**
         * Converted to and from MSD and MIDI by doing (0x10000 / TPQN), albeit with a loss
         * of precision just like tempo.
         */
        u32 tpqn;

        /**
         * Initial tempo...? Official converter puts 500 here (which matches the standard MIDI
         * 120 BPM default, except in milliseconds per quarter note) if it can't find a tempo
         * message, otherwise (MPQN / 1000) sometimes.
         */
        u32 initialTempo;
    };
    
    enum Status : u8 {
        Note             = 0x00 ... 0x3F,
        
        Reference        = 0x81,
        Loop             = 0x82,
        EndOfTrack       = 0x83,
        TempoChange      = 0x84,
        GateExtend       = 0x88 ... 0x8B,
        StepExtend       = 0x8C ... 0x8F,

        ControlChange    = 0xB0 ... 0xBF,
        ProgramChange    = 0xC0 ... 0xCF,
        ChannelPressure  = 0xD0 ... 0xDF,
        PitchWheelChange = 0xE0 ... 0xEF
    };
    
    /**
     * For the most part, all the control changes match MIDI spec, however some general use or
     * lesser used controls are reused for more Dreamcast specific controls.
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
        Undefined_MSB           = 20 ... 31, // used in Dreamcast to indicate loops?

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

        // mmh, see msd.hpp for more
        
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
        program : 7;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];
    
    bitfield ChannelPressureBits {
        step16Bit : 1;
        pressure : 7;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];

    bitfield PitchWheelChangeBits {
        step16Bit : 1;
        pitch : 7;
    } [[bitfield_order(BITFIELD_ORDER, 8)]];

    // BPM = 60 * 1e3 / tempo
    // Tempo in MIDI is usually from usecs/quarter note, but in MSD it's msecs? (1e6 vs 1e3)
    struct Message {
        Status status;
        StatusBits statusBits @ $ - 1;
        
        // Messages that associate to a channel
        bool messageHandled = true;
        match (u8(status) & 0xF0) {

            /**
             * Unlike MIDI, which has delta time, MSD has step time, which is the opposite to
             * what MIDI delta time is.
             * MIDI delta time is the time before an event, while MSD step time is the time
             * after an event.
             * MSD also doesn't have note off commands, and instead relies on gate time, which
             * is the time until the note should be turned off.
             */
            (Status::Note | 0x00 ... 0x3F): {
                u8 note;
                u8 velocity;
                
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
            
            (Status::ChannelPressure): {
                ChannelPressureBits cpData;
                 
                 if (cpData.step16Bit)
                    u16 step;
                 else
                    u8 step;
            }
            
            (Status::PitchWheelChange): {
                PitchWheelChangeBits pwcData;
                 
                 if (pwcData.step16Bit)
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
            
        // Messages that don't associate to a channel
        match (u8(status)) {
            (Status::Reference): {
                u16 offset;
                u8 length;
            }
            
            (Status::Loop): {
                // Seems to be the same data from the original CC31
                u8 data;
                
                /**
                 * The way this works is like MIDI variable-length quantity, but mode and step are unrelated
                 * so we can't just use that
                 */
                if (data & 0x80)
                    u16 step;
                else
                    u8 step;
            }
            
            (Status::EndOfTrack): {
                _gMoreMessages = false;
            }
            
            (Status::TempoChange): {
                u16 tempo;
                u8 step;
            }

            (Status::GateExtend | 0x88 ... 0x8B): {
                /* match (u8(status) & 0xF) {
                    (0x8): { extendBy = 0x200;  }
                    (0x9): { extendBy = 0x800;  }
                    (0xA): { extendBy = 0x1000; }
                    (0xB): { extendBy = 0x2000; }
                } */
            }

            (Status::StepExtend | 0x8C ... 0x8F): {
                /* match (u8(status) & 0xF) {
                    (0xC): { extendBy = 0x100;  }
                    (0xD): { extendBy = 0x200;  }
                    (0xE): { extendBy = 0x800;  }
                    (0xF): { extendBy = 0x1000; }
                } */
            }

            // Cannot reliably continue, message would be of unknown length
            (_): {
                // _gMoreMessages = false;
                std::error(std::format("Unknown/unhandled MSD message at 0x{0:x}: 0x{1:x} ({1})", $, u8(status)));
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
        
        if (header.version >= 2) {
            // std::print("{0:x} ({0})", msb::calcChecksum(4, header.fileSize - 8));
            le u32 checksum @ header.fileSize - 8;
        }
    };
}

msb::MSB msb @ 0x00;
