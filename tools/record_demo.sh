#!/usr/bin/env bash
# Records a main-menu demo (src/GalaxyEggbert/Game/DemoReplay.hpp) from a mission's start.
#
# Usage: tools/record_demo.sh <build-dir> <out.gdemo> <mission> [game arguments...]
#   scripted, from a QA autoplay script, cut after 45 s (2700 ticks), under its own Xvfb display:
#     tools/record_demo.sh build-cna-opengl3 Content/demos/demo001.gdemo 11 \
#         --autoplay tools/autoplay/world011.txt --demo-ticks 2700 --demo-title "World 1: lifts and mines"
#   by hand, on this display: play, then press Escape (or close the window) to end the recording:
#     tools/record_demo.sh build-cna-opengl3 Content/demos/demo002.gdemo 74 --demo-title "Skateboard"
#
# The recording ends at Escape, at the mission's end (exit, death that leaves Play), at the
# autoplay's end, after --demo-ticks, or when the window closes; F-keys and Tab are left out.
# Like tools/autoplay.sh the game runs from build-probe/demo-record/ (DEMO_RUN=<dir> for another),
# with a fresh savedata.txt each time, so the recorded hub doors and lives are the new game's.
# A demo plays only with the simulation revision and the world it was recorded with: after
# recording, check it with tools/verify_demos.sh; after a simulation change, bump
# kDemoSimulationRevision and record every demo again (handoff.md, "Main-menu demos").
set -u

if [[ $# -lt 3 ]]; then
    sed -n '2,20p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
mission="$3"
shift 3

args=()
scripted=0
while [[ $# -gt 0 ]]; do
    if [[ "$1" == --autoplay && $# -ge 2 ]]; then
        args+=(--autoplay "$(cd "$(dirname "$2")" && pwd)/$(basename "$2")")
        scripted=1
        shift 2
    else
        args+=("$1")
        shift
    fi
done

run="${DEMO_RUN:-$repo/build-probe/demo-record}"
mkdir -p "$run"
for d in Content avatars3d textures3d worlds3d; do
    ln -sfn "$repo/$d" "$run/$d"
done
rm -f "$run/savedata.txt" "$run/resume.txt" "$run/autoplay_trace.txt"
# A hard link, not a symlink: CNA resolves its content roots against the executable's real
# directory (see qa_live.sh).
rm -f "$run/GalaxyEggbert"
ln "$build/GalaxyEggbert" "$run/GalaxyEggbert"

# The player's files in the run folder, not the system's per-user one (UserFolder.hpp).
export GALAXY_USER_DIR="$run"
cd "$run"
if [[ $scripted == 1 ]]; then
    # Silent unless QA_SOUND=1 (memory: scripted runs must not play through the speakers).
    audio_driver=dummy
    [[ "${QA_SOUND:-}" == 1 ]] && audio_driver="${SDL_AUDIODRIVER:-}"
    SDL_AUDIODRIVER="$audio_driver" SDL_VIDEODRIVER=x11 xvfb-run -a -s "-screen 0 1280x720x24" \
        ./GalaxyEggbert --mission "$mission" --record-demo "$out" "${args[@]}" > game.log 2>&1
else
    ./GalaxyEggbert --mission "$mission" --record-demo "$out" "${args[@]}" > game.log 2>&1
fi
code=$?
grep -E "demo|autoplay|ERROR|exception|Abort" game.log
[[ -f "$out" ]] || { echo "record_demo.sh: no demo was written" >&2; exit 1; }
exit $code
