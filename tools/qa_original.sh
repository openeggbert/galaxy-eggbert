#!/usr/bin/env bash
# Scripted run of the ORIGINAL game -- the Linux build of the mobile-eggbert C++ port -- under its
# own Xvfb display, silent, with tools/qa_live.sh's steps: to see what the original does where the
# source alone leaves it open (BD-0017's teleport arrival was settled this way).
#
# Usage: tools/qa_original.sh <port-dir> <out-dir> <steps-file>
#
# <port-dir> is a COPY of the build (the owner's is ~/Downloads/SpeedyBlupi2013Linux; never run or
# change that one, nor ../mobile-eggbert): the game writes its saves into the copy's
# .cna_isolated_storage. Its menu's Play always starts mission 1, so to watch mission N copy the
# copy's worlds/worldNNN.txt over its worlds/world001.txt:
#
#   cp -a ~/Downloads/SpeedyBlupi2013Linux/. build-probe/orig-port/
#   cp build-probe/orig-port/worlds/world034.txt build-probe/orig-port/worlds/world001.txt
#   tools/qa_original.sh build-probe/orig-port build-probe/qa-shots/orig steps.txt
#
# The window is 800x480; Play is at (710, 380) on the menu (about 7 s after start). Blupi walks
# with the arrow keys. Steps: sleep, shot, key, keydown, keyup, click, type (see tools/qa_live.sh).
set -u
if [[ $# -lt 3 ]]; then
    sed -n '2,20p' "$0" >&2
    exit 2
fi
port="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
steps="$(cd "$(dirname "$3")" && pwd)/$(basename "$3")"
if [[ -z "${QA_ORIGINAL_INNER:-}" ]]; then
    QA_ORIGINAL_INNER=1 exec xvfb-run -a -s "-screen 0 1280x720x24" "$0" "$port" "$out" "$steps"
fi
cd "$port"
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 ./WindowsPhoneSpeedyBlupi > "$out/game.log" 2>&1 &
pid=$!
wid=""
for _ in $(seq 80); do
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
        type) xdotool type --delay 120 "$a" ;;
        "#"*|"") ;;
        *) echo "qa_original.sh: unknown step '$cmd'" >&2 ;;
    esac
done < "$steps"

kill "$pid" 2>/dev/null
sleep 1
kill -9 "$pid" 2>/dev/null
exit 0
