# manatools

Manatools aims to be a tool that can export, convert, and manipulate audio and music files for the Sega Dreamcast.

Its name comes from Manatee, the name of an audio driver (and that of a marine mammal), and the word "tool".<br/>
<sub><sup><s>Not the most creative name, but better than none.</s> [Turns out it's took by another project](https://github.com/manatools), gah</sup></sub>

## Download

Releases for Windows are published at https://github.com/dakrk/manatools/releases.

## Building

Please view [BUILDING.md](BUILDING.md).

## File formats

<!--
	is there really no way to make a table cell span multiple lines in markdown source code? ugh
	maybe I shouldn't use a table for data this long
-->
| Name                   | Extension      | Description                                                                                                    | Current state                                             |
| ---------------------- | -------------- | -------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- |
| MIDI Program/Drum Bank | .mpb, .mdb     | MPB/MDB files store instrument and sample data for music, although sometimes they are also used for SFX.       | Extractable, modifiable.                                  |
| MIDI Sequence Bank     | .msb           | MSB files store data for MIDI sequences. Despite its name, the data doesn't seem very MIDI compliant.          | Extractable, cannot modify yet.                           |
| Multi-Unit             | .mlt           | MLT files are a collection of these file formats, to get mapped into the AICA sound processor's RAM.           | Extractable, modifiable.                                  |
| One Shot Bank          | .osb           | OSB files are used for SFX. They share similar parameters with MPB, minus the MIDI note/velocity stuff.        | Extractable, modifiable.                                  |
| FX Output Bank         | .fob           | FOB files are a bank of output/mixer data. Mixer data stores level and panning values for up to 16 channels.   | Viewable, modifiable.                                     |
| FX Program Bank        | .fpb           | FPB files store DSP programs for FX.                                                                           | Not enough information yet.                               |

## Acknowledgements

- kingshriek's old utilities: http://snesmusic.org/hoot/kingshriek/ssf/
- madrxx's MPB ImHex pattern: https://gist.github.com/madrxx/0a60182074084a48345d1016da8c5d1f
- X-Hax's utilities: https://github.com/X-Hax/sa_tools_research (discovered this quite late, referencing could've saved time with earlier work, d'oh)
