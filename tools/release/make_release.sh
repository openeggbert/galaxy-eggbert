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
# on one. The Windows package is not made here: its cross-build has not been run yet
# (plans/audit/owner_questions.md).
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
android=1
[[ "${3:-}" == "--no-android" ]] && android=0
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv

echo "== Galaxy Eggbert $version"
grep -q '^CMAKE_BUILD_TYPE:STRING=Release$' "$build/CMakeCache.txt" ||
    { echo "$build is not a Release build" >&2; exit 1; }

echo "== Linux archive"
cmake --build "$build" --target GalaxyEggbertCNA --parallel 6
cmake --build "$build" --target package --parallel 6
linux="GalaxyEggbert-$version-linux-x86_64.tar.gz"
mv "$build/$linux" "$out/$linux"
need="$( (objdump -T "$build/GalaxyEggbert"
          find "$build/_CPack_Packages" -path '*/lib/*.so*' -type f -exec objdump -T {} \;) 2>/dev/null |
         grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -n1)"
echo "   $linux: needs glibc $need or newer"

if (( android )); then
    echo "== Android APK (arm64-v8a)"
    (cd "$repo/android" &&
     JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}" ./gradlew --no-daemon assembleRelease \
         -PgalaxyAbis=arm64-v8a) # not android.injected.build.abi: that makes a test-only APK
    apk="$repo/android/app/build/outputs/apk/release/app-release.apk"
    cp "$apk" "$out/GalaxyEggbert-$version-android-arm64.apk"
fi

echo "== Notes and checksums"
cp "$repo/packaging/RELEASE_NOTES.md" "$out/RELEASE_NOTES.md"
(cd "$out" && sha256sum GalaxyEggbert-"$version"-*.tar.gz GalaxyEggbert-"$version"-*.apk 2>/dev/null > SHA256SUMS.txt; cat SHA256SUMS.txt)
echo "== Done: $out"
