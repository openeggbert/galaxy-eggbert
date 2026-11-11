#!/usr/bin/env bash
# Galaxy Eggbert's internet games: libcna's cna-gamer-services-server on a Linux host of the owner's
# (owner_questions.md Q-022). Every packet of an internet game goes through it (its relay), so no
# player has to set up a router. Guide: docs/handbook/10-server.md.
#
#   sudo tools/server/galaxy-server.sh install <domain> <e-mail>   # Debian/Ubuntu, as root, once
#   sudo tools/server/galaxy-server.sh add-player <name> <gamertag> # asks for the password
#   sudo tools/server/galaxy-server.sh status
#   sudo tools/server/galaxy-server.sh backup
#
# install: the build's packages, the server built from its source (SERVER_SOURCE=<dir> uses a checkout
# already there; else SERVER_REPO is cloned) and installed in /usr/local/bin; a system user; a
# Let's Encrypt certificate for <domain> (certbot, TCP 80 must reach the host while it asks; renewals
# copy it again and restart the service); the server's own systemd unit; the title galaxy-eggbert;
# TCP 47831 opened in ufw when ufw is there. The players type <domain> as the game's Server.
#
# For a try on a machine without root: SERVER_BIN=<dir with the binaries> SERVER_DB=<database> and
# NO_SUDO=1 make add-player and status work on that database.
set -euo pipefail

SERVER_REPO="${SERVER_REPO:-https://github.com/libcna/cna-gamer-services-server.git}"
SERVER_BIN="${SERVER_BIN:-/usr/local/bin}"
SERVER_DB="${SERVER_DB:-/var/lib/cna-gamer-services/service.sqlite3}"
SERVICE_USER=cna-gamer-services
TITLE=galaxy-eggbert
PORT=47831

die() { echo "galaxy-server: $*" >&2; exit 1; }
as_service() { if [[ "${NO_SUDO:-0}" == 1 ]]; then "$@"; else runuser -u "$SERVICE_USER" -- "$@"; fi; }
admin() { as_service "$SERVER_BIN/cna-gamer-services-admin" "$SERVER_DB" "$@"; }

install_server() {
    local domain="$1" email="$2"
    [[ $EUID == 0 ]] || die "install needs root (sudo)"
    [[ "$domain" == *.* ]] || die "'$domain' is not a domain name (the players type it as the game's Server)"
    echo "== packages"
    apt-get update
    apt-get install -y build-essential cmake ninja-build ccache git certbot \
        libboost-dev libssl-dev libsqlite3-dev nlohmann-json3-dev python3
    echo "== the server, built from its source"
    local source="${SERVER_SOURCE:-/usr/local/src/cna-gamer-services-server}"
    if [[ ! -d "$source/.git" && ! -f "$source/CMakeLists.txt" ]]; then
        git clone --depth 1 "$SERVER_REPO" "$source"
    fi
    cmake -S "$source" -B "$source/build-release" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    cmake --build "$source/build-release" --target cna-gamer-services-server cna-gamer-services-admin --parallel "$(nproc)"
    install -m 0755 "$source/build-release/cna-gamer-services-server" "$source/build-release/cna-gamer-services-admin" "$SERVER_BIN/"
    echo "== its user and folders"
    getent group "$SERVICE_USER" > /dev/null || groupadd --system "$SERVICE_USER"
    getent passwd "$SERVICE_USER" > /dev/null ||
        useradd --system --gid "$SERVICE_USER" --home-dir /var/lib/cna-gamer-services --shell /usr/sbin/nologin "$SERVICE_USER"
    install -d -m 0700 -o "$SERVICE_USER" -g "$SERVICE_USER" /var/lib/cna-gamer-services
    install -d -m 0700 /etc/cna-gamer-services
    echo "== the certificate for $domain (Let's Encrypt)"
    certbot certonly --standalone --non-interactive --agree-tos -m "$email" -d "$domain"
    cat > /etc/letsencrypt/renewal-hooks/deploy/cna-gamer-services.sh <<HOOK
#!/bin/sh
# A renewed certificate for the gamer services server (tools/server/galaxy-server.sh).
install -m 0600 /etc/letsencrypt/live/$domain/fullchain.pem /etc/cna-gamer-services/certificate.pem
install -m 0600 /etc/letsencrypt/live/$domain/privkey.pem /etc/cna-gamer-services/private-key.pem
systemctl try-restart cna-gamer-services.service
HOOK
    chmod 0755 /etc/letsencrypt/renewal-hooks/deploy/cna-gamer-services.sh
    install -m 0600 "/etc/letsencrypt/live/$domain/fullchain.pem" /etc/cna-gamer-services/certificate.pem
    install -m 0600 "/etc/letsencrypt/live/$domain/privkey.pem" /etc/cna-gamer-services/private-key.pem
    echo "== the title $TITLE"
    admin title "$TITLE" "Galaxy Eggbert" || true # already there on a second install
    echo "== the service"
    install -m 0644 "$source/tools/cna-gamer-services-deploy/systemd/cna-gamer-services.service" /etc/systemd/system/
    systemctl daemon-reload
    systemctl enable --now cna-gamer-services.service
    if command -v ufw > /dev/null; then ufw allow "$PORT/tcp"; fi
    echo "== done: the players type '$domain' as the game's Server (TCP $PORT must be reachable)."
    echo "   Make each player's account: sudo $0 add-player <name> <gamertag>"
}

add_player() {
    local name="$1" gamertag="$2"
    [[ "$name" =~ ^[a-z0-9][a-z0-9._-]{0,31}$ ]] || die "a name of lowercase letters, digits, . _ - (what the player signs in with)"
    [[ ${#gamertag} -ge 1 && ${#gamertag} -le 15 ]] || die "a gamertag of 1-15 characters (the player's name in the games)"
    local password again
    read -r -s -p "Password for $name: " password; echo
    read -r -s -p "Again: " again; echo
    [[ "$password" == "$again" ]] || die "the two passwords differ"
    [[ ${#password} -ge 8 ]] || die "a password of 8 characters at least"
    printf '%s\n' "$password" | admin user "$name" "$gamertag" > /dev/null
    echo "The account $name ($gamertag) is made: the player signs in with it in the game's Guide."
}

case "${1:-}" in
    install) [[ $# == 3 ]] || die "install <domain> <e-mail>"; install_server "$2" "$3" ;;
    add-player) [[ $# == 3 ]] || die "add-player <name> <gamertag>"; add_player "$2" "$3" ;;
    status)
        [[ "${NO_SUDO:-0}" == 1 ]] || systemctl --no-pager status cna-gamer-services.service || true
        admin inspect
        admin inspect-online "$TITLE" ;;
    backup)
        source="${SERVER_SOURCE:-/usr/local/src/cna-gamer-services-server}"
        as_service python3 "$source/tools/cna-gamer-services-deploy/backup.py" --database "$SERVER_DB" \
            --output-dir "$(dirname "$SERVER_DB")/backups" ;;
    *) sed -n '2,20p' "$0" >&2; exit 2 ;;
esac
