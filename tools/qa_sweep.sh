#!/usr/bin/env bash
# Loads every shipped world live (tools/qa_live.sh: wait, walk ahead two seconds, a screenshot)
# and writes one line per world to <out>/result.txt: "NNN ok shot", or "NNN FAIL ..." when the
# game's log holds an error. Run after changes that could break a world's loading or drawing.
#
# Usage: tools/qa_sweep.sh <build-dir> [out-dir]     (default out-dir build-probe/sweep)
set -u
if [[ $# -lt 1 ]]; then
    sed -n '2,7p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$1"
out="${2:-$repo/build-probe/sweep}"
mkdir -p "$out/run"
printf 'sleep 6\nfocus\nkeydown Up\nsleep 2\nkeyup Up\nsleep 1\nshot v\n' > "$out/steps.txt"
: > "$out/result.txt"
cd "$repo"
for f in worlds3d/world[0-9]*.vwr; do
    n=$(basename "$f" .vwr); n=${n#world}
    if QA_LIVE_RUN="$out/run" timeout 120 tools/qa_live.sh "$build" "$out/w$n" "$out/steps.txt" --mission $((10#$n)) \
        > "$out/w$n.out" 2>&1; then
        echo "$n ok $([[ -f "$out/w$n/v.png" ]] && echo shot || echo noshot)" >> "$out/result.txt"
    else
        echo "$n FAIL $(tail -2 "$out/w$n.out" | tr '\n' ' ')" >> "$out/result.txt"
    fi
done
grep -v ' ok shot$' "$out/result.txt" && exit 1
exit 0
