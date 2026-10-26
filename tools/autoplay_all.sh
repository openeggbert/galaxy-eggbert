#!/usr/bin/env bash
# Replays every playthrough in tools/autoplay/ (worldNNN.txt plays mission NNN) and prints one
# line per mission; the exit code is 1 if any failed. A script whose first line says BLOCKED is
# listed and skipped. Run after an engine or world change that could touch gameplay.
#
# Usage: tools/autoplay_all.sh <build-dir> [worldNNN ...]
#   e.g. tools/autoplay_all.sh build-cna-opengl3
#        tools/autoplay_all.sh build-cna-opengl3 world011 world121
#
# Each run goes through tools/autoplay.sh (its own Xvfb display, a fresh save in AUTOPLAY_RUN,
# default build-probe/autoplay-run/).
set -u

if [[ $# -lt 1 ]]; then
    sed -n '2,10p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$1"
shift
if [[ $# -gt 0 ]]; then
    scripts=()
    for name in "$@"; do scripts+=("$repo/tools/autoplay/$name.txt"); done
else
    scripts=("$repo"/tools/autoplay/world[0-9][0-9][0-9].txt)
fi

failed=0
for script in "${scripts[@]}"; do
    name="$(basename "$script" .txt)"
    mission=$((10#${name#world}))
    if head -1 "$script" | grep -q BLOCKED; then
        printf '%-10s blocked (see its header)\n' "$name"
        continue
    fi
    result="$("$repo/tools/autoplay.sh" "$build" "$script" "$mission" 2>&1 | grep -E 'autoplay (passed|FAILED)|FAIL ' | tail -2)"
    if echo "$result" | grep -q 'autoplay passed'; then
        printf '%-10s passed  %s\n' "$name" "$(echo "$result" | grep -o 'after [0-9]* ticks')"
    else
        printf '%-10s FAILED  %s\n' "$name" "$(echo "$result" | head -1)"
        failed=1
    fi
done
exit $failed
