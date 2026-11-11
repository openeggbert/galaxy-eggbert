# 10. The server for internet games

Galaxy Eggbert's games over the internet (the owner's choice, `plans/audit/owner_questions.md` Q-022)
go through **cna-gamer-services-server**, libcna's service for CNA games. Every packet goes through
its relay, so no player has to forward ports on a router. The players sign in with accounts the
administrator makes: the server has no self-registration. One server serves everyone. On a local
network or with ZeroTier no server is needed (`09-multiplayer.md`).

## What it needs

- **A Linux machine reachable from the internet:** a small VPS, or a home machine whose router
  forwards one TCP port. Debian 12/13 or Ubuntu 22.04+ for the script below.
- **A domain name pointing at it** (an A or AAAA record). The certificate is Let's Encrypt's, so
  TCP 80 must reach the machine while the certificate is asked for and renewed.
- **TCP 47831** open to the internet: the game, the relay and the events all go over this one port.
  The diagnostics port (47832) stays on the machine.

## Installing

From a checkout of Galaxy Eggbert on that machine, as root:

```sh
sudo tools/server/galaxy-server.sh install games.example.org admin@example.org
```

The script:

1. Installs the packages: compiler, CMake, Boost, OpenSSL, SQLite, nlohmann/json, certbot.
2. Clones the server's source (`SERVER_SOURCE=<dir>` uses a checkout already there) and builds
   it. The binaries go to `/usr/local/bin`.
3. Creates the system user `cna-gamer-services`, the database folder
   `/var/lib/cna-gamer-services` and `/etc/cna-gamer-services`.
4. Asks Let's Encrypt for the certificate. A renewal hook copies every renewal there and restarts
   the service.
5. Registers the title `galaxy-eggbert`.
6. Installs and starts the server's own hardened systemd unit (`cna-gamer-services.service`).
7. Opens TCP 47831 in ufw when ufw is there. Another firewall, and a home router's forwarding of
   TCP 47831, are yours to set.

## The players' accounts

```sh
sudo tools/server/galaxy-server.sh add-player eva Eva
```

- The first word is what the player signs in with: lowercase letters, digits, `.`, `_` and `-`.
- The second is the gamertag, the name the others see in the games (1-15 characters).
- The script asks for the password twice (8 characters at least) and never shows it. Give it to
  the player in person or by a private message.
- The server's own `cna-gamer-services-admin` does more: revoke an account (`revoke-user`), see
  the counts (`inspect`). It is described in the server's README.

## In the game

- Each player types the domain (`games.example.org`; `:port` only when it is not 47831) as
  **Server** in the multiplayer list.
- At the game's next start, CNA's Guide asks for the account's name and password.
- **Internet games** then lists the games on the server. Making, joining and playing are as on a
  local network. A game made there is seen only by the players signed in at that server.

## Keeping it

- **Status:** `sudo tools/server/galaxy-server.sh status` shows the service, the accounts and the
  games now online.
- **Backups:** `sudo tools/server/galaxy-server.sh backup` makes an online backup of the database
  (the server's `backup.py`: checked, `0600`) into `/var/lib/cna-gamer-services/backups`. Copy
  them off the machine.
- **Updates:** pull the server's source, then run `install` again. It keeps the database, the
  accounts and the title.

## What has been tried

On the developer's machine (2026-11-11):

- The server was built out of its repository and run on 127.0.0.1 in plain HTTP
  (`--insecure-loopback`).
- `add-player` made an account, and the game signed in with it.
- Two- and four-player games (two of them on the Windows build under wine) were played through
  the relay (`tools/net_internet.sh`).

Not yet tried:

- `install` on a real host, with TLS and Let's Encrypt.
- Players on different networks over the internet.
