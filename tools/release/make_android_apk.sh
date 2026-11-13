#!/usr/bin/env bash
# Keep the APK's ABI, signing and release name the same for local and CI releases.
#   tools/release/make_android_apk.sh <out-dir>
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out="$(mkdir -p "$1" && cd "$1" && pwd)"
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
export CCACHE_DIR="${CCACHE_DIR:-/rv/cnaccache}" CCACHE_BASEDIR="${CCACHE_BASEDIR:-/rv}"
export CMAKE_BUILD_PARALLEL_LEVEL="${GALAXY_BUILD_JOBS:-4}"
(cd "$repo/android" &&
 JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}" ./gradlew --no-daemon assembleRelease \
     -PgalaxyAbis=arm64-v8a) # not android.injected.build.abi: that makes a test-only APK
cp "$repo/android/app/build/outputs/apk/release/app-release.apk" \
   "$out/GalaxyEggbert-$version-android-arm64.apk"
