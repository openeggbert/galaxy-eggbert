#!/usr/bin/env bash
# Cover all packages of this version, including zips made before make_release.sh.
#   tools/release/make_checksums.sh <out-dir>
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="$(head -n1 "$repo/VERSION.txt" | tr -d '[:space:]')"
cd "$1"
shopt -s nullglob
packages=(GalaxyEggbert-"$version"-*.tar.gz GalaxyEggbert-"$version"-*.zip GalaxyEggbert-"$version"-*.apk)
if (( ${#packages[@]} == 0 )); then
    echo "No packages for $version in $PWD" >&2
    exit 1
fi
sha256sum "${packages[@]}" > SHA256SUMS.txt
cat SHA256SUMS.txt
