#!/usr/bin/env bash
# Two machines' network game on this one: tools/net_players.sh with a host and one who joins (the
# scenarios of tools/autoplay/net/, the name kept for them and the handbook).
#
# Usage: tools/net_two_players.sh <build-dir> <host-script> <join-script> [out-dir]
#   e.g. tools/net_two_players.sh build-cna-opengl3 tools/autoplay/net/bomb_host.txt \
#            tools/autoplay/net/bomb_join.txt
# NET_LIVES, NET_WORLD, NET_LOSS, NET_LATENCY, NET_EXPECT, NET_TIMEOUT: as tools/net_players.sh.
set -u
if [[ $# -lt 3 || $# -gt 4 ]]; then
    sed -n '2,8p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NET_OUT="${4:-${NET_OUT:-$repo/build-probe/net-two-players}}" exec "$repo/tools/net_players.sh" "$1" "$2" "$3"
