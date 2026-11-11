#!/usr/bin/env bash
# Two to four machines' network game on this one: one instance of the built game per script, each
# under its own Xvfb, find each other on the local network (CNA SystemLink), join, get ready and
# start (the GALAXY_NET_TEST hooks), then play their --autoplay scripts in real time. The first
# script is the host's (Ann, yellow), the others join in order (Ben orange, Cid blue, Dan green: the
# lobby's places). Passes when every script passes, each saw every other, and each line of the
# NET_EXPECT file is found.
#
# Usage: tools/net_players.sh <build-dir> <host-script> <join-script>...
#   e.g. NET_LIVES=0 tools/net_players.sh build-cna-opengl3 tools/autoplay/net/four_ann.txt \
#            tools/autoplay/net/four_ben.txt tools/autoplay/net/four_cid.txt tools/autoplay/net/four_dan.txt
# NET_OUT=<dir> the logs and traces (build-probe/net-players by default).
# NET_EXPECT=<file> lines "Name: extended regex" each to be found in that player's log (# comments).
# NET_LIVES=<n> starts each player with n reserve lives (GALAXY_NET_LIVES; the original's 10 by default).
# NET_WORLD=<201-212> is the host's mission (GALAXY_NET_WORLD; the first installed by default).
# NET_LOSS=<0-1> and NET_LATENCY=<ms> make the network a bad one (GALAXY_NET_LOSS, GALAXY_NET_LATENCY:
# CNA's simulated loss of the unreliable packets and latency of all, on what each machine sends).
# NET_TIMEOUT=<s> each instance's limit (240 by default).
# When the scripts write world hashes (autoplay's `hash`), every player's last one must be the same.
set -u
if [[ $# -lt 3 || $# -gt 5 ]]; then
    sed -n '2,21p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$(cd "$1" && pwd)"
shift
names=(Ann Ben Cid Dan)
scripts=()
for s in "$@"; do scripts+=("$(cd "$(dirname "$s")" && pwd)/$(basename "$s")"); done
count=${#scripts[@]}
out="${NET_OUT:-$repo/build-probe/net-players}"
mkdir -p "$out"
out="$(cd "$out" && pwd)" # the instances run in their own directories
rm -f "$out"/*.rc "$out"/*.log "$out"/*.trace

run() { # <name> <mode> <script>
    local dir="$repo/build-probe/net-run-$1"
    mkdir -p "$dir"
    for d in Content avatars3d textures3d worlds3d; do ln -sfn "$repo/$d" "$dir/$d"; done
    rm -f "$dir/GalaxyEggbert"
    ln "$build/GalaxyEggbert" "$dir/GalaxyEggbert"
    (
        cd "$dir"
        env GALAXY_USER_DIR="$dir" GALAXY_NET_TEST="$2" GALAXY_NET_NAME="$1" GALAXY_NET_PLAYERS="$count" \
            ${NET_LIVES:+GALAXY_NET_LIVES=$NET_LIVES} ${NET_WORLD:+GALAXY_NET_WORLD=$NET_WORLD} \
            ${NET_LOSS:+GALAXY_NET_LOSS=$NET_LOSS} ${NET_LATENCY:+GALAXY_NET_LATENCY=$NET_LATENCY} \
            AUTOPLAY_REALTIME=1 SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 \
            timeout "${NET_TIMEOUT:-240}" xvfb-run -a -s "-screen 0 800x600x24" ./GalaxyEggbert --autoplay "$3" \
            > "$out/$1.log" 2>&1
        echo $? > "$out/$1.rc"
        cp -f autoplay_trace.txt "$out/$1.trace" 2>/dev/null
    )
}

for ((i = 0; i < count; ++i)); do
    run "${names[$i]}" "$([[ $i == 0 ]] && echo host || echo join)" "${scripts[$i]}" &
    sleep 2 # the lobby's places in this order
done
wait

status=0
for ((i = 0; i < count; ++i)); do
    who=${names[$i]}
    rc=$(cat "$out/$who.rc" 2>/dev/null || echo 1)
    if [[ "$rc" != 0 ]]; then echo "FAIL: $who's script (exit $rc), $out/$who.log"; status=1; fi
    for ((j = 0; j < count; ++j)); do
        other=${names[$j]}
        [[ $j == "$i" ]] && continue
        if ! grep -aq "multiplayer: $other seen at" "$out/$who.log"; then echo "FAIL: $who never saw $other"; status=1; fi
    done
    grep -a "multiplayer: " "$out/$who.log" | sed "s/^GalaxyEggbertCNA: multiplayer: /$who: /"
done
# The world hashes (autoplay's `hash`): every player who wrote one wrote the same last one.
hashes=()
for ((i = 0; i < count; ++i)); do
    h=$(grep -a "multiplayer: world hash" "$out/${names[$i]}.log" | tail -1 | sed 's/.*world hash \([0-9a-f]*\).*/\1/')
    [[ -n "$h" ]] && hashes+=("${names[$i]}=$h")
done
if [[ ${#hashes[@]} -gt 0 ]]; then
    if [[ $(printf '%s\n' "${hashes[@]}" | sed 's/.*=//' | sort -u | wc -l) != 1 ]]; then
        echo "FAIL: the worlds differ: ${hashes[*]}"; status=1
    elif [[ ${#hashes[@]} -lt 2 ]]; then
        echo "FAIL: one world hash only (${hashes[*]}): nothing to compare"; status=1
    else
        echo "the worlds agree: ${hashes[*]}"
    fi
fi
if [[ -n "${NET_EXPECT:-}" ]]; then
    while IFS= read -r line; do
        [[ -z "$line" || "$line" == \#* ]] && continue
        who="${line%%: *}"
        pattern="${line#*: }"
        if ! grep -aEq "$pattern" "$out/$who.log" 2>/dev/null; then echo "FAIL: not in $who's log: $pattern"; status=1; fi
    done < "$NET_EXPECT"
fi
[[ $status == 0 ]] && echo "PASS: the network game of $count"
exit $status
