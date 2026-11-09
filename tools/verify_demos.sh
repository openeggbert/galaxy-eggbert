#!/usr/bin/env bash
# Replays every shipped main-menu demo (Content/demos/demoNNN.gdemo, DemoReplay.hpp) in the game,
# undrawn and silent under its own Xvfb display, and compares each one's end state with the one
# stored when it was recorded. Also checks that the replays leave the player's savedata.txt and
# resume.txt byte for byte as they were.
#
# Usage: tools/verify_demos.sh <build-dir> [demo files...]
#
# A demo reported "different" no longer replays as recorded: the simulation changed. Bump
# kDemoSimulationRevision (DemoReplay.hpp) and record the demos again with tools/record_demo.sh
# (the commands are in handoff.md, "Main-menu demos"). "cannot be played" means it was refused: damaged,
# another revision, or its world changed (the game's log says which; build-probe/demo-verify/).
# The game runs from build-probe/demo-verify/ (DEMO_RUN=<dir> for another).
set -u

if [[ $# -lt 1 ]]; then
    sed -n '2,14p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
shift

demos=()
if [[ $# -gt 0 ]]; then
    for f in "$@"; do demos+=("$(cd "$(dirname "$f")" && pwd)/$(basename "$f")"); done
else
    for n in $(seq -f "%03g" 1 99); do
        [[ -f "$repo/Content/demos/demo$n.gdemo" ]] || break
        demos+=("Content/demos/demo$n.gdemo")
    done
fi
if [[ ${#demos[@]} -eq 0 ]]; then
    echo "verify_demos.sh: no demos found" >&2
    exit 1
fi

run="${DEMO_RUN:-$repo/build-probe/demo-verify}"
mkdir -p "$run"
for d in Content avatars3d textures3d worlds3d; do
    ln -sfn "$repo/$d" "$run/$d"
done
rm -f "$run/GalaxyEggbert"
ln "$build/GalaxyEggbert" "$run/GalaxyEggbert"
cd "$run"
# A player's own files, which no demo may change or delete.
printf 'soundEnabled=1\nselectedGamer=2\ngamer2.lives=7\ngamer2.missionNumber=23\ngamer2.hasProgress=1\n' > savedata.txt
printf 'GALAXY_RESUME 7\n"a player build"\nsentinel\n' > resume.txt
before="$(sha256sum savedata.txt resume.txt)"

failed=0
for demo in "${demos[@]}"; do
    SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 xvfb-run -a -s "-screen 0 1280x720x24" \
        ./GalaxyEggbert --verify-demo "$demo" > game.log 2>&1
    code=$?
    line="$(grep -E "^DEMO VERIFY" game.log | tail -1)"
    echo "${line:-DEMO VERIFY $demo: no result (exit $code)}"
    if [[ $code -ne 0 || "$line" != *": equal"* ]]; then
        grep -E "demo|ERROR|exception|Abort" game.log | grep -v "^DEMO VERIFY" >&2
        failed=1
    fi
done

after="$(sha256sum savedata.txt resume.txt 2>/dev/null)"
if [[ "$before" != "$after" ]]; then
    echo "verify_demos.sh: a demo changed the player's savedata.txt or resume.txt" >&2
    failed=1
else
    echo "player files unchanged"
fi
exit $failed
