# mpbgui

GUI for editing and converting Sega MIDI Program Bank and MIDI Drum Bank files used by Dreamcast titles.

## Development

The `.ui` files are editable in Qt Designer, however for some changes (like moving children from one parent to another if Designer doesn't let you, max width of a section in a QSplitter without extra steps, etc) you may have to manually edit them in a text editor because Qt Designer kind of sucks in this regard (but I think it's more tolerable than having even messier source files).