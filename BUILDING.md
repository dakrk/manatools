# Building

## Linux

You will need at least GCC 12 or Clang 14.

### Installing dependencies

#### Arch Linux

```bash
sudo pacman -S base-devel git cmake qt6-base qt6-svg portaudio
```

#### Debian & Ubuntu

You should be using at least Debian 12 or Ubuntu 23 as to get Qt 6 and a recent GCC version.

```bash
sudo apt install build-essential git cmake qt6-base-dev libqt6svg6 portaudio19-dev
```

If you're on Ubuntu and packages like `qt6-base-dev` are missing, you should add the `universe` repository. To do this, try the following commands:

```bash
sudo add-apt-repository universe
sudo apt update
```

**Note:** The Qt 6 versions available as of writing have some minor visual issues due to a slight breaking change in newer versions of Qt Designer (e.g, UI elements might be more squashed than they should be).

### Compiling

```bash
git clone --recursive https://github.com/dakrk/manatools
cd manatools
cmake --preset release
cmake --build --preset release -j

# And if successful, you should be able to run:
./build/mpbgui
```

On systems with a lot of threads but not a lot of remaining RAM, consider specifying less threads after the `-j` in the CMake command (e.g., like `-j6`).

## Windows (MSYS2 UCRT64)

### Installing dependencies

```bash
pacman -Syu
pacman -S base-devel git mingw-w64-ucrt-x86_64-{toolchain,cmake,qt6-base,portaudio}
```

### Compiling

```bash
git clone --recursive https://github.com/dakrk/manatools
cd manatools
cmake --preset release
cmake --build --preset release -j

# And if successful, you should be able to run:
./build/mpbgui.exe
```

## Windows (Visual Studio)

You will need at least Visual Studio 2022, with C++ and CMake support enabled (which should be default).

### Installing dependencies

Installing Qt 6 using the official installer requires giving your personal information to The Qt Company. Instead, I use aqt, which you can get at https://github.com/miurahr/aqtinstall/releases/latest.

After downloading aqt, move it to a folder you wish for Qt to be installed to, and then open a command prompt in the same folder. You should then run:

```bat
aqt install-qt windows desktop 6.7.2 win64_msvc2019_64
setx QTDIR "%CD%\6.7.2\msvc2019_64"
```

### Compiling

You can then open Visual Studio, and either try the "Clone a repository" option, or try opening a command prompt with Git available and running the following:

```shell
git clone --recursive https://github.com/dakrk/manatools
```

Then in Visual Studio, click the "Open a local folder" option and navigate to the folder you cloned to and select it.

Once the main IDE window opens, CMake should start generating. At the top you should be able to select between a debug and release build. You can then find the "Build" menu item at the top, and click "Build All" which should show after a successful generation.

**Note:** To run the GUI tools from Visual Studio, you should add the "bin" folder of the Qt directory to your PATH, then restart Visual Studio. (Or a hackier way is to manually copy those DLLs and the plugins folder to the build folder.)

## macOS

Haven't tested yet, sorry. For the most part, presumably it'd be similar to the Linux installations, except using Homebrew as the package manager, and potentially needing to fiddle with the CMakeLists for Qt.

# Packaging

Not sure yet, sorry. Instructions shall be updated and a Windows release put on the GitHub releases page once I'm sure. (Moreso Qt raising issues here.)
