#!/usr/bin/env bash
# The web package of Galaxy Eggbert: the Emscripten build's page (GalaxyEggbert.html, .js, .wasm,
# .data) with the music it fetches (Content/music/), LICENSE, THIRD_PARTY.txt, RELEASE_NOTES.md and
# a README, zipped. It is served as it is from any web server.
#
#   tools/release/make_web_zip.sh <web-build-dir> <out-dir>
#   e.g. tools/release/make_web_zip.sh build-cna-webgl2 build-probe/release/0.1.0
#
# The build dir is the Emscripten build (docs/handbook/03-build-and-run.md, "Web"); it is built here
# first (an up-to-date one only links nothing), with the emsdk's environment when EMSDK_ENV points at
# its emsdk_env.sh (default ~/emsdk/emsdk_env.sh).
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
export CCACHE_DIR="${CCACHE_DIR:-/rv/cnaccache}" CCACHE_BASEDIR="${CCACHE_BASEDIR:-/rv}"
grep -q 'Emscripten' "$build/CMakeCache.txt" || { echo "$build is not an Emscripten build" >&2; exit 1; }
emsdk_env="${EMSDK_ENV:-$HOME/emsdk/emsdk_env.sh}"
# shellcheck disable=SC1090
[[ -f "$emsdk_env" ]] && source "$emsdk_env" > /dev/null 2>&1
cmake --build "$build" --target GalaxyEggbertCNA --parallel "${GALAXY_BUILD_JOBS:-4}"
name="GalaxyEggbert-$version-web"
stage="$out/$name"
rm -rf "$stage" "$out/$name.zip"
mkdir -p "$stage/Content"
cp "$build"/GalaxyEggbert.{html,js,wasm,data} "$stage/"
cp -rL "$build/Content/music" "$stage/Content/"
cp "$repo/LICENSE" "$repo/packaging/THIRD_PARTY.txt" "$repo/packaging/RELEASE_NOTES.md" "$stage/"
cp "$repo/packaging/README-web-bundle.md" "$stage/README.md"
(cd "$out" && zip -qr9 "$name.zip" "$name")
rm -rf "$stage"
(cd "$out" && sha256sum "$name.zip")
echo "== $out/$name.zip"
