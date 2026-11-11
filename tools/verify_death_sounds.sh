#!/usr/bin/env bash
# A death plays its sound once (a Discord player's report, 2026-11-11: "every sound is played several
# times"): the game itself, under Xvfb, puts Blupi over lava, a saw and a spike trap of world 999
# (--spawn) and lets him die there (tools/autoplay/probes/death_once.txt). With GALAXY_SOUND_LOG=1
# it writes each sound asked for; the death's channel must be asked for exactly once -- while the
# death lock held him in the hazard, it was asked for again every tick.
#
#   tools/verify_death_sounds.sh <build-dir>
#
# Runs from DEATH_RUN (default build-probe/death-sounds), the player's files there.
set -u
if [[ $# -lt 1 ]]; then
    sed -n '2,10p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
run="${DEATH_RUN:-$repo/build-probe/death-sounds}"
mkdir -p "$run"
for d in Content avatars3d textures3d worlds3d; do
    ln -sfn "$repo/$d" "$run/$d"
done
rm -f "$run/GalaxyEggbert"
ln "$build/GalaxyEggbert" "$run/GalaxyEggbert"
export GALAXY_USER_DIR="$run"
cd "$run"

failed=0
# name, the raw cell over the hazard, the death's channel
for case in "lava 41,3,5,0 8" "saw 4,2,19,0 75" "spike 75,2,17,0 51"; do
    set -- $case
    rm -f savedata.txt resume.txt autoplay_trace.txt
    GALAXY_SOUND_LOG=1 SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 xvfb-run -a -s "-screen 0 800x480x24" \
        ./GalaxyEggbert --mission 999 --spawn "$2" --autoplay "$repo/tools/autoplay/probes/death_once.txt" \
        > game.log 2>&1
    code=$?
    asked=$(grep -c "^GalaxyEggbertCNA: sound request $3\$" game.log)
    if [[ $code -eq 0 && $asked -eq 1 ]]; then
        echo "PASS: $1: the script passed, channel $3 asked for once"
    else
        echo "FAIL: $1: script exit $code, channel $3 asked for $asked times (once expected)"
        grep -E "FAIL|expect|ERROR|exception|Abort" autoplay_trace.txt game.log 2>/dev/null | head -5 >&2
        failed=1
    fi
done
exit $failed
