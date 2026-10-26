#!/usr/bin/env bash
# Plays a mission by an autoplay script (src/GalaxyEggbert/Game/Autoplay.hpp) under its own Xvfb
# display and prints the trace; the exit code is the game's: 0 when the script passed.
#
# Usage: tools/autoplay.sh <build-dir> <script> <mission> [game arguments...]
#   e.g. tools/autoplay.sh build-cna-opengl3 tools/autoplay/world011.txt 11
#
# Like tools/qa_live.sh the game runs from build-probe/autoplay-run/ (AUTOPLAY_RUN=<dir> to use
# another), whose asset directories are symlinks to the repository's, so savedata.txt is written
# there and never into the build directory. AUTOPLAY_WORLDS=<dir> uses that directory as worlds3d/.
# The trace is left in the run directory as autoplay_trace.txt, the game's output as game.log.
set -u

if [[ $# -lt 3 ]]; then
    sed -n '2,12p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
script="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
mission="$3"
shift 3

run="${AUTOPLAY_RUN:-$repo/build-probe/autoplay-run}"
mkdir -p "$run"
for d in Content avatars3d textures3d worlds3d; do
    ln -sfn "$repo/$d" "$run/$d"
done
if [[ -n "${AUTOPLAY_WORLDS:-}" ]]; then
    ln -sfn "$(cd "$AUTOPLAY_WORLDS" && pwd)" "$run/worlds3d"
fi
# A fresh save each run, so the hubs' doors and lives start the same way every time.
rm -f "$run/savedata.txt" "$run/autoplay_trace.txt"
# A hard link, not a symlink: CNA resolves its content roots against the executable's real
# directory (see qa_live.sh).
rm -f "$run/GalaxyEggbertCNA"
ln "$build/GalaxyEggbertCNA" "$run/GalaxyEggbertCNA"

cd "$run"
SDL_VIDEODRIVER=x11 xvfb-run -a -s "-screen 0 1280x720x24" \
    ./GalaxyEggbertCNA --mission "$mission" --autoplay "$script" "$@" > game.log 2>&1
code=$?
cat autoplay_trace.txt 2>/dev/null
grep -E "autoplay|ERROR|exception|Abort" game.log
exit $code
