#!/usr/bin/env bash
# Two machines' network game on this one: two instances of the built game, each under its own
# Xvfb, find each other on the local network (CNA SystemLink), join, get ready and start (the
# GALAXY_NET_TEST hooks), then play their --autoplay scripts in real time. Passes when both scripts
# pass and each saw the other.
#
# Usage: tools/net_two_players.sh <build-dir> <host-script> <join-script> [out-dir]
#   e.g. tools/net_two_players.sh build-cna-opengl3 tools/autoplay/net/bomb_host.txt \
#            tools/autoplay/net/bomb_join.txt
# NET_LIVES=<n> starts each player with n lives (GALAXY_NET_LIVES; the original's 10 by default).
set -u
if [[ $# -lt 3 ]]; then
    sed -n '2,11p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
host_script="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
join_script="$(cd "$(dirname "$3")" && pwd)/$(basename "$3")"
out="${4:-$repo/build-probe/net-two-players}"
mkdir -p "$out"

run() { # <name> <mode> <script>
    local dir="$repo/build-probe/net-run-$1"
    mkdir -p "$dir"
    for d in Content avatars3d textures3d worlds3d; do ln -sfn "$repo/$d" "$dir/$d"; done
    rm -f "$dir/GalaxyEggbert"
    ln "$build/GalaxyEggbert" "$dir/GalaxyEggbert"
    (
        cd "$dir"
        env GALAXY_USER_DIR="$dir" GALAXY_NET_TEST="$2" GALAXY_NET_NAME="$1" ${NET_LIVES:+GALAXY_NET_LIVES=$NET_LIVES} \
            AUTOPLAY_REALTIME=1 SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 \
            timeout 240 xvfb-run -a -s "-screen 0 800x600x24" ./GalaxyEggbert --autoplay "$3" > "$out/$1.log" 2>&1
        echo $? > "$out/$1.rc"
        cp -f autoplay_trace.txt "$out/$1.trace" 2>/dev/null
    )
}

run Ann host "$host_script" &
sleep 2
run Ben join "$join_script" &
wait

status=0
for who in Ann Ben; do
    rc=$(cat "$out/$who.rc" 2>/dev/null || echo 1)
    other=$([[ $who == Ann ]] && echo Ben || echo Ann)
    if [[ "$rc" != 0 ]]; then echo "FAIL: $who's script (exit $rc), $out/$who.log"; status=1; fi
    if ! grep -aq "multiplayer: $other seen at" "$out/$who.log"; then echo "FAIL: $who never saw $other"; status=1; fi
    grep -a "multiplayer: " "$out/$who.log" | sed "s/^/$who: /"
done
[[ $status == 0 ]] && echo "PASS: the network game of two"
exit $status
