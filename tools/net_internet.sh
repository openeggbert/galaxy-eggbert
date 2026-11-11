#!/usr/bin/env bash
# Internet games through CNA's gamer services server (Q-022), on this machine: a disposable server
# (cna-gamer-services-server, built out of its repository: tools/net_internet.sh's caller names the
# tree) on 127.0.0.1 in plain HTTP (--insecure-loopback), the title galaxy-eggbert and the accounts
# ann, ben, cid, dan (gamertags Ann ..., passwords <name>-password), then tools/net_players.sh with
# NET_SERVER: each signs in through CNA's Guide and the games are PlayerMatch, through the relay.
#
# Usage: tools/net_internet.sh <build-dir> <server-build-dir> <host-script> <join-script>...
#   e.g. tools/net_internet.sh build-cna-opengl3 build-probe/gss tools/autoplay/net/bomb_host.txt \
#            tools/autoplay/net/bomb_join.txt
# NET_SERVER_PORT=<port> (47931 by default); the other NET_* as tools/net_players.sh.
set -u
if [[ $# -lt 4 ]]; then
    sed -n '2,12p' "$0" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$1"
server="$(cd "$2" && pwd)"
shift 2
out="${NET_OUT:-$repo/build-probe/net-internet}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"
port="${NET_SERVER_PORT:-47931}"
db="$out/service.sqlite3"
rm -f "$db" "$db-wal" "$db-shm"
admin="$server/cna-gamer-services-admin"
"$admin" "$db" title galaxy-eggbert "Galaxy Eggbert" > /dev/null || exit 1
for user in ann ben cid dan; do
    echo "$user-password" | "$admin" "$db" user "$user" "${user^}" > /dev/null || exit 1
done
"$server/cna-gamer-services-server" --database "$db" --insecure-loopback --port "$port" > "$out/server.txt" 2>&1 &
pid=$!
trap 'kill $pid 2>/dev/null; wait $pid 2>/dev/null' EXIT
for ((t = 0; t < 100; ++t)); do
    (exec 3<>"/dev/tcp/127.0.0.1/$port") 2>/dev/null && break
    sleep 0.1
done
NET_SERVER="127.0.0.1:$port" NET_OUT="$out" "$repo/tools/net_players.sh" "$build" "$@"
