#!/usr/bin/env bash
# Builds the release packages of Galaxy Eggbert (the owner's goal of 2026-11-10) into one folder:
#
#   tools/release/make_release.sh <build-dir> <out-dir> [--no-android]
#   e.g. tools/release/make_release.sh build-cna-opengl3 build-probe/release
#
# - the Linux archive: CPack's `package` target of <build-dir> (a Release build of
#   GalaxyEggbertCNA; Content/ with the 2x sheets, the worlds, the models, the SDL3 libraries,
#   LICENSE, THIRD_PARTY.txt, RELEASE_NOTES.md, VERSION.txt, a launcher);
# - the Android APK: android/'s `assembleRelease` for arm64-v8a, signed with the release key of
#   android/key.properties (skipped with --no-android);
# - RELEASE_NOTES.md and SHA256SUMS.txt beside them.
#
# The version is VERSION.txt's. The glibc the Linux program needs is printed (the highest GLIBC_
# version it and its bundled libraries ask for): an archive for an older system has to be built
# on one. The Windows package is made by tools/release/make_windows_zip.sh from a MinGW cross-build
# (WINDOWS.md), the web package by tools/release/make_web_zip.sh from the Emscripten build: make
# them into the same <out-dir> first, and SHA256SUMS.txt covers them too.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
android=1
[[ "${3:-}" == "--no-android" ]] && android=0
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
export CCACHE_DIR="${CCACHE_DIR:-/rv/cnaccache}" CCACHE_BASEDIR="${CCACHE_BASEDIR:-/rv}"

echo "== Galaxy Eggbert $version"
grep -q '^CMAKE_BUILD_TYPE:STRING=Release$' "$build/CMakeCache.txt" ||
    { echo "$build is not a Release build" >&2; exit 1; }

echo "== Linux archive"
cmake --build "$build" --target GalaxyEggbertCNA --parallel "${GALAXY_BUILD_JOBS:-4}"
cmake --build "$build" --target package --parallel "${GALAXY_BUILD_JOBS:-4}"
linux="GalaxyEggbert-$version-linux-x86_64.tar.gz"
mv "$build/$linux" "$out/$linux"
need="$( (objdump -T "$build/GalaxyEggbert"
          find "$build/_CPack_Packages" -path '*/lib/*.so*' -type f -exec objdump -T {} \;) 2>/dev/null |
         grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -n1)"
echo "   $linux: needs glibc $need or newer"

if (( android )); then
    echo "== Android APK (arm64-v8a)"
    "$repo/tools/release/make_android_apk.sh" "$out"
fi

echo "== Notes and checksums"
cp "$repo/packaging/RELEASE_NOTES.md" "$out/RELEASE_NOTES.md"
"$repo/tools/release/make_checksums.sh" "$out"
echo "== Done: $out"
