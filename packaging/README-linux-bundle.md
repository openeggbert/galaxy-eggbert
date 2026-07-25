# Galaxy Eggbert CNA — Linux bundle

Run the game from this extracted directory:

```sh
./run-galaxy-eggbert
```

The launcher sets the working directory required by the game's bundled data.
Do not move `GalaxyEggbertCNA` away from `Content/`, `worlds3d/`, `textures3d/`,
or `avatars3d/`.

This x86-64 Linux bundle includes the SDL3, SDL3_image, and SDL3_mixer shared
libraries used to build it. It still uses standard libraries supplied by the
host distribution, including its graphics stack, FFmpeg, and C++ runtime. If
the game cannot start because a system library is absent, install the matching
runtime package from your distribution and try again.

`LICENSE` applies to Galaxy Eggbert. Licenses for the bundled SDL libraries
are in `licenses/SDL/`. This archive is a runtime bundle, not the full source
release; the source tree contains the build instructions and GPLv3 license.
