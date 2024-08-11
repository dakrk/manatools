#pragma once
#include <QString>
#include <manatools/note.hpp>

inline QString noteToString(u8 note) {
	return QString("%1%2").arg(manatools::noteName(note)).arg(manatools::noteOctave(note));
}
