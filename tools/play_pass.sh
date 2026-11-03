#!/usr/bin/env bash
# Plays every campaign world (or the ones named) by its autoplay script with AUTOPLAY_SHOTS, keeping
# a frame every N ticks of the run, and lays each world's frames out as contact sheets: a look at
# the whole campaign as the player sees it along the playthroughs' routes.
#
# Usage: tools/play_pass.sh <build-dir> <out-dir> [worldNNN ...]
#   PLAY_PASS_EVERY=N  ticks between frames (default 360: 6 s of play)
#   PLAY_PASS_JOBS=N   worlds played at once (default 3), each in build-probe/play-pass-run-<n>/
#   PLAY_PASS_KEEP=1   keep the full-size frames (default: only the sheets, to spare the disk)
# A script that includes others (a hub played as a chain of its missions) is played without
# frames: its missions have their own pass. Every script's result is kept, so a pass is also a
# full playthrough regression.
#
# Output: <out-dir>/<world>-pN.jpg (the sheets, 20 frames each, every frame labelled with its tick
# and Blupi's place from the trace), <out-dir>/<world>/ (the trace, and the frames with KEEP),
# <out-dir>/results.txt.
set -u
if [[ $# -lt 2 ]]; then
    sed -n '2,16p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
mkdir -p "$2"
out="$(cd "$2" && pwd)"
shift 2
cd "$repo"
if [[ $# -gt 0 ]]; then names=("$@"); else names=(); for s in tools/autoplay/world[0-9][0-9][0-9].txt; do names+=("$(basename "$s" .txt)"); done; fi
every=${PLAY_PASS_EVERY:-360}
jobs=${PLAY_PASS_JOBS:-3}
keep=${PLAY_PASS_KEEP:-0}
: > "$out/results.txt"

play_one() {
    local slot=$1 name=$2
    local run="$repo/build-probe/play-pass-run-$slot"
    local script="$repo/tools/autoplay/$name.txt"
    local mission=$((10#${name#world}))
    local shots=$every
    grep -q '^include ' "$script" && shots=0
    mkdir -p "$run"
    rm -rf "$run/autoplay_shots"
    local result
    result="$(AUTOPLAY_SHOTS=$shots AUTOPLAY_RUN=$run "$repo/tools/autoplay.sh" "$build" "$script" "$mission" 2>&1 |
              grep -E 'autoplay (passed|FAILED)|FAIL ' | tail -2 | tr '\n' ' ')"
    rm -rf "${out:?}/$name" "$out/$name"-p*.jpg
    mkdir -p "$out/$name"
    cp "$run/autoplay_trace.txt" "$out/$name/" 2>/dev/null
    if [[ $shots -gt 0 && -d "$run/autoplay_shots" ]]; then
        python3 "$repo/tools/play_pass_sheet.py" "$run/autoplay_shots" "$out/$name" "$run/autoplay_trace.txt"
        if [[ "$keep" == 1 ]]; then
            cp "$run"/autoplay_shots/*.png "$out/$name/" 2>/dev/null
        fi
    fi
    rm -rf "$run/autoplay_shots"
    printf '%-10s %s\n' "$name" "${result:-no result}" >> "$out/results.txt"
}
# Lanes, as build-probe/wave-e/lanes_log.sh: each plays its share in turn in its own run directory.
for ((l = 0; l < jobs; l++)); do
    (
        for ((k = l; k < ${#names[@]}; k += jobs)); do
            play_one "$l" "${names[$k]}"
        done
    ) &
done
wait
sort -o "$out/results.txt" "$out/results.txt"
cat "$out/results.txt"
