# Terminal++

[![Windows Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/1?label=windows&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=1)
[![Linux Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/2?label=linux&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=2)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/fd4f07b095634b9d90bbb9edb11fc12c)](https://www.codacy.com/manual/zduka/tpp?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=terminalpp/tpp&amp;utm_campaign=Badge_Grade)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/zduka/tpp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/zduka/tpp/context:cpp)

*Terminal++* or `tpp` is a minimalist cross-platform terminal emulator that is capable of emulating much of the commonly used `xterm` supported control sequences. 

> Please note that `tpp` is in an  alpha stage and there may (and will be) rough edges. That said, it has been used by a few people as their daily driver with only minor issues. Notably the font detection is lacking and unless you have the Iosevka font tpp uses by default the terminal might look very ugly before you edit the settings file. 

# Configuration

The terminal settings can either be specified in the settings JSON file (it is a pseudo JSON with comments for clarity). Some of these can then be overriden on the commandline for each particular instance. Keyboard shortcuts cannot be configured at the moment.

The settings are stored under the following paths on different platforms:

- Windows: `%APP_DATA%\\Local\\terminalpp\\settings.json`
- Linux: `~/.config/terminalpp/settings.json`

Upon first execution of `tpp`, default settings will be created, which can later be edited by hand. 

## Keybindings

These are default keybinding settings:

`Ctrl +` and `Ctrl -` to increase / decrease zoom. 
`Ctrl Shift V` to paste from clipboard.
`Alt Space` to toggle full-screen mode.
`Ctrl+F1` displays the about box with version information

## Command-line options

To show a list of command-line options and their meanings, run the terminal from a console:

    tpp --help

## Fonts

`tpp` should work with any font the underlying renderer can work with, but has been tested extensively with `Iosevka` which was (for now) copied in the repo (see the `fonts/LICENSE.md` for the fonts license.

Iosevka is also the first font `tpp` will try to use. If not found, defaults to `Consolas` on Windows and system font on Linux. This can be changed at any time either on the commandline, or in the settings.json

# Platform Specific Notes

`tpp` is supported natively on Windows and Linux. It runs on Apple computers as well, although the support is experimental and an X server is required for `tpp` to work at all. 

## Windows

On Windows, Windows 10 1903 and above are supported. `tpp` works with default `cmd.exe` prompt, Powershell and most importantly the Windows Subsystem for Linux (WSL). Accelerated rendering via DirectWrite is used. An `msi` installer is available in the release assets.

### ConPTY Bypass

If `tpp` is used with WSL, the [`tpp-bypass`](https://github.com/zduka/tpp-bypass) app can be used inside WSL to bypass the ConPTY terminal driver in Windows. This has numerous benefits such as full support for all escape sequences (i.e. mouse works) and speed (with the bypass enabled, `tpp` is the fastest terminal emulator on windows*). 

If `wsl` is detected, `tpp` will attempt to install the bypass upon its first execution by downloading the binary appropriate for given `wsl` distribution. 

> Alternatively, the bypass can be downloaded from the repository above, built and installed manually as `~/.local/bin/tpp-bypass` where `tpp` will find it. Note that bypass check is only performed on first execution (i.e. when no settings.json exists).

### Building from sources

To build under Windows, [Visual Studio](https://visualstudio.microsoft.com) and [cmake](https://cmake.org) must be installed. Open the Visual Studio Developer's prompt and get to the project directory. Then type the following commands:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

Then type the following to start the terminal:

    Release\tpp.exe

### Running `tpp` in WSL

This is entirely possible if X server is installed on Windows, but should not be necessary. If you really need to, follow the linux installation guide.

## Linux

Although tested only on Ubuntu and OpenSUSE Leap (native and WSL), all linux distributions should work just fine since `tpp` has minimal dependencies. On Linux, `X11` with `xft` are used for the rendering. This can be pretty fast still due to `render` X server extension. 

`deb`, `rpm` and `snap` packages are provided from the release assets.

> Currently, the snap package can only be downloaded and installed with the `--dangerous --classic` options as the app has not yet been approved for the snapstore. 

### Building from sources

> On ubuntu systems, you can install all prerequisites by executing:

    sudo apt install cmake libx11-dev libxft-dev

Create the build directory and run `cmake`:

    mkdir build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make

You can now execute the terminal by typing in the `build` directory:

    ./tpp

## Apple

When X server is installed, follow the Linux guide above. 

> Note that unlike Windows and Linux, Apple version is untested. Also, since std::filesystem is available only since 10.15 and there are no 10.15 workers on azure. Not even build on OSX is tested at the moment.

# Performance (*)

> Take the following section with a *big* grain of salt since the benchmark used is hardly representative or proper terminal workload. All it shows is peak performance of some terminal parts under very atrificial conditions. We are only using it because alacritty, which claims to be the fastest terminal out there uses it. 

Although raw peformance is not the main design goal of `tpp`, it is reasonably performant under both Windows and Linux. The terminal framerate is limited (configurable, but defaults to 60 FPS).

For lack of proper benchmarks, the [`vtebench`](https://github.com/jwilm/vtebench) from `alacritty` was used to test the basic performance. The `alt-screen-random-write`, `scrolling` and `scrolling-in-region` benchmarks are used because `tpp` does not support wide unicode characters yet. 

Informally, `tpp` with bypass trashes everyone else on Windows being at least 25x faster than the second best terminal (Alacritty). Note that the bypass is the key here since it seems that for many other terminals ConPTY is the bottleneck. 

On Linux, `tpp` comparable to `alacritty` and they swap places as the fastest terminal depending on the benchmark used. 
