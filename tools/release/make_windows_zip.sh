#!/usr/bin/env bash
# The Windows package of Galaxy Eggbert (Q-018, Q-022): the MinGW cross-build's GalaxyEggbert.exe
# and its DLLs with the game's data, LICENSE, THIRD_PARTY.txt, RELEASE_NOTES.md and a README, zipped.
#
#   tools/release/make_windows_zip.sh <windows-build-dir> <out-dir>
#   e.g. tools/release/make_windows_zip.sh build-probe/windows build-probe/release
#
# The build dir is a MinGW cross-build of GalaxyEggbertCNA (its commands in WINDOWS.md); it is built
# here first (an up-to-date one only links nothing).
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
export CCACHE_DIR="${CCACHE_DIR:-/rv/cnaccache}" CCACHE_BASEDIR="${CCACHE_BASEDIR:-/rv}"
grep -q 'mingw' "$build/CMakeCache.txt" || { echo "$build is not a MinGW cross-build" >&2; exit 1; }
cmake --build "$build" --target GalaxyEggbertCNA --parallel "${GALAXY_BUILD_JOBS:-4}"
name="GalaxyEggbert-$version-windows-x86_64"
stage="$out/$name"
rm -rf "$stage" "$out/$name.zip"
mkdir -p "$stage"
cp "$build/GalaxyEggbert.exe" "$build"/*.dll "$stage/"
for d in Content worlds worlds3d textures3d avatars3d; do cp -rL "$repo/$d" "$stage/"; done
cp "$repo/LICENSE" "$repo/packaging/THIRD_PARTY.txt" "$repo/packaging/RELEASE_NOTES.md" "$stage/"
cp "$repo/packaging/README-windows-bundle.md" "$stage/README.md"
(cd "$out" && zip -qr9 "$name.zip" "$name")
rm -rf "$stage"
(cd "$out" && sha256sum "$name.zip")
echo "== $out/$name.zip"
