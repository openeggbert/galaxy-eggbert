#!/usr/bin/env bash
# Scripted live run of GalaxyEggbertCNA under its own Xvfb display, for visual checks.
#
# Usage: tools/qa_live.sh <build-dir> <out-dir> <steps-file> [game arguments...]
#   e.g. tools/qa_live.sh build-cna-opengl3 build-probe/qa-shots/pause steps.txt --mission 999
#
# The game runs from build-probe/qa-run/, whose asset directories are symlinks to the
# repository's Content/, avatars3d/, textures3d/ and worlds3d/: savedata.txt and customworlds/
# are written there, never into the build directory, where they are the player's own data.
#
# Steps, one per line ("#" comments allowed):
#   sleep <seconds>          wait
#   shot <name>              save <out-dir>/<name>.png, cropped to the game window
#   key <keysym...>          press and release (xdotool names: Escape, Up, F3, ...)
#   keydown <keysym> / keyup <keysym>
#   click <x> <y>            left click at window coordinates
#   move <x> <y>             move the mouse to window coordinates
#   focus                    give the game window the keyboard focus again
set -u

if [[ $# -lt 3 ]]; then
    sed -n '2,20p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
steps="$(cd "$(dirname "$3")" && pwd)/$(basename "$3")"
shift 3

if [[ -z "${QA_LIVE_INNER:-}" ]]; then
    QA_LIVE_INNER=1 exec xvfb-run -a -s "-screen 0 1280x720x24" "$0" "$build" "$out" "$steps" "$@"
fi

run="$repo/build-probe/qa-run"
mkdir -p "$run"
for d in Content avatars3d textures3d worlds3d; do
    ln -sfn "$repo/$d" "$run/$d"
done
ln -sfn "$build/GalaxyEggbertCNA" "$run/GalaxyEggbertCNA"

cd "$run"
SDL_VIDEODRIVER=x11 ./GalaxyEggbertCNA "$@" > "$out/game.log" 2>&1 &
pid=$!
wid=""
for _ in $(seq 50); do
    wid=$(xdotool search --pid "$pid" 2>/dev/null | tail -1)
    [[ -n "$wid" ]] && break
    sleep 0.1
done
[[ -n "$wid" ]] && xdotool windowfocus "$wid" 2>/dev/null
geometry() { xdotool getwindowgeometry --shell "$wid" 2>/dev/null; }

while read -r cmd a b; do
    case "$cmd" in
        sleep) sleep "$a" ;;
        shot)
            eval "$(geometry)"
            import -window root -crop "${WIDTH}x${HEIGHT}+${X}+${Y}" +repage "$out/$a.png" ;;
        key) xdotool key --delay 80 $a ;;
        keydown) xdotool keydown "$a" ;;
        keyup) xdotool keyup "$a" ;;
        click)
            eval "$(geometry)"
            xdotool mousemove $((X + a)) $((Y + b)); sleep 0.2
            xdotool mousedown 1; sleep 0.15; xdotool mouseup 1 ;;
        move)
            eval "$(geometry)"
            xdotool mousemove $((X + a)) $((Y + b)) ;;
        focus) xdotool windowfocus "$wid" 2>/dev/null ;;
        "#"*|"") ;;
        *) echo "qa_live.sh: unknown step '$cmd'" >&2 ;;
    esac
done < "$steps"

kill "$pid" 2>/dev/null
sleep 1
kill -9 "$pid" 2>/dev/null
grep -E "ERROR|exception|Abort" "$out/game.log" && exit 1
exit 0
