#!/usr/bin/env bash
# Read a tag's files without changing the checkout, so old releases can be checked locally too.
# Without a tag, check the working files for a workflow_dispatch dry run.
#   tools/release/check_release_metadata.sh [vX.Y.Z]
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo"
tag="${1:-}"
fail() { echo "Release metadata: $*" >&2; exit 1; }
read_file() {
    if [[ -n "$tag" ]]; then git show "refs/tags/$tag:$1"; else cat "$1"; fi
}
if [[ -n "$tag" ]]; then
    [[ "$tag" == v* ]] || fail "expected a version tag beginning with v"
    git rev-parse --verify "refs/tags/$tag^{commit}" > /dev/null
    [[ "$(git cat-file -t "refs/tags/$tag")" == tag ]] || fail "$tag must be annotated"
fi
version="$(read_file VERSION.txt | head -n1 | tr -d '\r')"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?$ ]] || fail "invalid VERSION.txt: $version"
[[ -z "$tag" || "$tag" == "v$version" ]] || fail "$tag does not match VERSION.txt ($version)"
changelog="$(read_file CHANGELOG.md)"
[[ $'\n'"$changelog" == *$'\n'"## [$version] — "* ]] || fail "CHANGELOG.md lacks ## [$version] — date"
notes="$(read_file packaging/RELEASE_NOTES.md)"
[[ "$notes" == "# Galaxy Eggbert $version"$'\n'* ]] || fail "release notes do not name $version"
version_code() {
    sed -nE 's/^[[:space:]]*versionCode[[:space:]]+([0-9]+)([[:space:]].*)?$/\1/p'
}
if [[ -n "$tag" ]]; then
    current="$(read_file android/app/build.gradle | version_code)"
    [[ "$current" =~ ^[0-9]+$ ]] || fail "expected one numeric Android versionCode"
    # The parent excludes this tag itself, including when a release is re-run.
    previous="$(git describe --tags --abbrev=0 --match 'v*' "refs/tags/$tag^" 2>/dev/null || true)"
    if [[ -n "$previous" ]]; then
        old="$(git show "$previous:android/app/build.gradle" | version_code)"
        [[ "$old" =~ ^[0-9]+$ ]] || fail "expected one Android versionCode in $previous"
        (( current > old )) || fail "Android versionCode $current must exceed $previous's $old"
        echo "Android versionCode: $old ($previous) -> $current" >&2
    else
        echo "No previous v* tag; skipping the Android versionCode comparison." >&2
    fi
fi
prerelease=false
[[ "$version" != *-* ]] || prerelease=true
printf 'version=%s\nprerelease=%s\n' "$version" "$prerelease"
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    printf 'version=%s\nprerelease=%s\n' "$version" "$prerelease" >> "$GITHUB_OUTPUT"
fi
