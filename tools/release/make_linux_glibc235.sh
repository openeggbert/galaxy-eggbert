#!/usr/bin/env bash
# The Linux archive built on Ubuntu 22.04, so that it runs on glibc 2.35 and newer (the owner's choice,
# 2026-11-12; Q-018). The build runs in a rootless Ubuntu 22.04 root (~/deps/ubuntu-22.04-root, entered by
# ~/deps/ubuntu-22.04-enter.sh: a user namespace, no sudo) with GCC 13 (ppa:ubuntu-toolchain-r/test),
# libstdc++ and libgcc linked statically, CNA's FFmpeg features off (the game's music is Ogg Vorbis,
# decoded by stb_vorbis) and its own SDL3 build (CNA_SDL_PREBUILT_ROOT: never the host's, which needs the
# host's glibc). CMake is the Android SDK's 4.1.2 (Ubuntu 22.04's 3.22 is older than easy-gl asks).
#
# Usage: tools/release/make_linux_glibc235.sh <out-dir>
#   Configures build-probe/glibc235/tree on its first run, then builds the package target there and puts
#   GalaxyEggbert-<version>-linux-x86_64.tar.gz into <out-dir>, with the newest glibc symbol it needs.
#
# No ccache: the root's ccache (4.5) is far older than the host's shared cache (4.11) and must not write
# into it; the tree is kept, so a release rebuilds only what changed.
set -euo pipefail

if [[ $# -lt 1 ]]; then
    sed -n '2,16p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out="$(mkdir -p "$1" && cd "$1" && pwd)"
enter="$HOME/deps/ubuntu-22.04-enter.sh"
cmake="$HOME/Android/Sdk/cmake/4.1.2/bin/cmake"
tree="$repo/build-probe/glibc235/tree"
[[ -x "$enter" ]] || { echo "missing $enter (the Ubuntu 22.04 root)" >&2; exit 1; }
[[ -x "$cmake" ]] || { echo "missing $cmake (the Android SDK's CMake 4.1.2)" >&2; exit 1; }
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
cna="$(sed -n 's/^CNA_HOME:PATH=//p' "$repo/build-cna-opengl3/CMakeCache.txt")"
engine="$(sed -n 's/^CNA_ENGINE_HOME:PATH=//p' "$repo/build-cna-opengl3/CMakeCache.txt")"

if [[ ! -f "$tree/build.ninja" ]]; then
    "$enter" "$cmake" -S "$repo" -B "$tree" -G Ninja -DCMAKE_MAKE_PROGRAM="$(dirname "$cmake")/ninja" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13 \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCNA_HOME="$cna" -DCNA_ENGINE_HOME="$engine" -DCNA_GRAPHICS_RENDERER=OPENGL33 \
        -DCNA_ENABLE_VIDEO=OFF -DCNA_ENABLE_WAVEBANK_CODECS=OFF \
        -DCNA_SDL_PREBUILT_ROOT="$repo/build-probe/glibc235/sdl-prebuilt" \
        -DGALAXY_EGGBERT_ENABLE_LINUX_BUNDLE=ON -DBUILD_TESTING=OFF \
        -DCMAKE_EXE_LINKER_FLAGS='-static-libstdc++ -static-libgcc' \
        -DCMAKE_SHARED_LINKER_FLAGS='-static-libstdc++ -static-libgcc' \
        -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
fi
"$enter" "$cmake" --build "$tree" --target GalaxyEggbertCNA --parallel 6
"$enter" "$cmake" --build "$tree" --target package --parallel 6
linux="GalaxyEggbert-$version-linux-x86_64.tar.gz"
mv "$tree/$linux" "$out/$linux"
need="$( (objdump -T "$tree/GalaxyEggbert"
          find "$tree/_CPack_Packages" -path '*/lib/*.so*' -type f -exec objdump -T {} \;) 2>/dev/null |
         grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -n1)"
echo "== $out/$linux: needs glibc $need or newer"
