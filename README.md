# QTV: a QuakeWorld match broadcasting tool 


## Supported architectures

The following architectures are fully supported by **[QTV][qtv]** and are available as prebuilt binaries:
* Linux amd64 (Intel and AMD 64-bits processors)
* Linux i686 (Intel and AMD 32-bit processors)
* Linux aarch (ARM 64-bit processors)
* Linux armhf (ARM 32-bit processors)
* Windows x64 (Intel and AMD 64-bits processors)
* Windows x86 (Intel and AMD 32-bit processors)

## Prebuilt binaries
You can find the prebuilt binaries on [this download page][qtv_builds].

## Prerequisites

None at the moment.

## Building binaries

### Build from source with CMake

Assuming you have installed essential build tools and ``CMake``
```bash
mkdir build && cmake -B build . && cmake --build build
```
Build artifacts would be inside ``build/`` directory, for unix like systems it would be ``qtv``.

You can also use ``build_cmake.sh`` script, it mostly suitable for cross compilation
and probably useless for experienced CMake user.
Some examples:
```
./build_cmake.sh linux-amd64
```
should build QTV for ``linux-amd64`` platform, release version, check [cross-cmake](tools/cross-cmake) directory for all platforms

```
B=Debug ./build_cmake.sh linux-amd64
```
should build QTV for linux-amd64 platform with debug

```
V=1 B=Debug ./build_cmake.sh linux-amd64
```
should build QTV for linux-amd64 platform with debug, verbose (useful if you need validate compiler flags)

```
V=1 B=Debug BOT_SUPPORT=OFF ./build_cmake.sh linux-amd64
```

same as above but compile without bot support

```
G="Unix Makefiles" ./build_cmake.sh linux-amd64
```

force CMake generator to be unix makefiles

```
./build_cmake.sh linux-amd64
```

build QTV for ``linux-amd64`` version, you can provide
any platform combinations.

## Versioning

For the versions available, see the [tags on this repository][qtv-tags].

## Authors

  Cokeman
  deurk
  JohnNy_cz
  qqshka
  VVD

Based on Spike's FTE QTV http://www.fteqw.com/

## Code of Conduct

We try to stick to our code of conduct when it comes to interaction around this project. See the [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) file for details.

## License

This project is licensed under the GPL-2.0 License - see the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgments

* Thanks to the fine folks on [Quakeworld Discord][discord-qw] for their support and ideas.

[qtv]: https://github.com/QW-Group/qtv
[qtv-tags]: https://github.com/QW-Group/qtv/tags
[qtv_builds]: https://builds.quakeworld.nu/qtv
[discord-qw]: http://discord.quake.world/
