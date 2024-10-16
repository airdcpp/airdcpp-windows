| AirDC++ version | Total downloads | AppVeyor
|:---:|:---:|:---:|
| [![version](https://img.shields.io/github/release/airdcpp/airgit.svg?label=latest%20version)](https://github.com/airdcpp/airdcpp-windows/releases) | [![GitHub all releases](https://img.shields.io/github/downloads/airdcpp/airdcpp-windows/total "Download AirDC++")](https://github.com/airdcpp/airdcpp-windows/releases) | [![Build status](https://ci.appveyor.com/api/projects/status/o34twjd29dntvme3?svg=true)](https://ci.appveyor.com/project/maksis/airgit)

# AirDC++ for Windows

## Supported operating systems

Windows 10/11

[AirDC++ Web Client](https://airdcpp.net/) is available for other operating systems.

## Current development status

| Component  | Status |
| ------------- | ------------- |
| Core  | Active  |
| Web API  | Active  |
| Web UI  | Active  |
| Windows GUI  | Bug fixes only  |

New maintainers and contributions are welcome for the Windows GUI

## Compiling

### Required tools

- [Visual Studio 2022](https://visualstudio.microsoft.com/)
- [Python](https://www.python.org/)
- [vcpkg](https://vcpkg.io/en/) with `VCPKG_ROOT` env variable pointing to the vcpkg installation directory
- [CMake](https://cmake.org/)

## Building from command line

Run the following commands in the repository root:

`cmake --preset=<preset-name>`

`cmake --build --preset=<preset-name>`

Replace `<preset-name>` with one of the presets listed under `cmake --list-presets`

## Opening in Visual Studio

You can simply use the `Open a local folder` option in Visual Studio. 

Alternatively you may also generate the Visual Studio project files with the following command:

`cmake "-DCMAKE_VS_GLOBALS=UseMultiToolTask=true;EnforceProcessCountAcrossBuilds=true" -B msvc -G "Visual Studio 17 2022" --preset=<preset-name>_`