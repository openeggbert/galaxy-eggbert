# 9. The network game (multiplayer)

Galaxy Eggbert's multiplayer is Speedy Blupi 2's network game (the owner's choice of 2026-11-10,
`plans/audit/owner_questions.md` Q-019), as `../blupi_recon` reconstructs it, played in Galaxy's
3D worlds. Two to four players on one local network; Linux and Android. Not in the web build (a
browser cannot send the UDP broadcasts the game is found by).

The design and its sources: `plans/plan_gemultiplayer.md`. The rules' differences from the
original: `plans/audit/behavior_differences.md` BD-0120.

## Playing

1. Every player starts Galaxy Eggbert on a machine of the same local network (the same Wi-Fi or
   wired network), the same version (the game compares the multiplayer worlds and the Blupi model:
   a game with other content is not listed).
2. In the menu choose the player (A, B or C: on the network you are "Player A" and so on), then
   **Multiplayer** (in the left panel, right of Setup).
3. One player makes a **New game**: the lobby opens with the mission (#201-#204, the host changes
   it with `<` and `>`). The others see it under *Available games on the network* (the list is
   searched again every few seconds, or at **Update list**), choose it and **Join selected game**.
4. In the lobby each player may change **Colour** (yellow, orange, blue, green: the start and the
   bombs of that colour; colours need not differ) while not ready, and says **Ready ?**. **Send
   message** types a line for the chat zone (the keyboard on the screen, or the real one; Enter
   sends). The host's **Start** is open once every player present is ready, two at least.
5. The match: ten lives each, everyone at the start of his colour. Crates, goals and enemy Blupis
   are taken out of the world. Each team has its personal bombs: beside one of your colour the action
   key takes it back (five at most), and lays it again elsewhere; another colour's bomb kills at a
   touch (unless shielded). Tank and helicopter shots kill the other players. A death starts
   again where you last stood safely. With no life left you are knocked out; the last player still
   present wins ("Congratulations, you won, you're the only survivor!").
6. Pause (Escape) stops everyone's game until it goes on. Pause's **Menu** leaves the game (the
   others are told). Leaving the win or lost screen ends your network game. When the host leaves,
   the game ends for everyone.

## Playing over the internet: ZeroTier

The game finds its games by a broadcast on the local network, so over the internet the players
need a virtual network that carries broadcasts. ZeroTier does (a free account at
<https://my.zerotier.com> is enough for a few machines):

1. One player creates a network in ZeroTier Central and notes its 16-digit network ID. In the
   network's settings leave **Multicast Recipient Limit** at its default (32) or higher, and
   **Enable Broadcast (ff:ff:ff:ff:ff:ff)** on (the default).
2. Every player installs ZeroTier One (Linux: `curl -s https://install.zerotier.com | sudo bash`;
   Android: the ZeroTier One app) and joins the network (`sudo zerotier-cli join <network ID>`, or
   the app's +).
3. The network's owner authorises each member in ZeroTier Central (the Auth checkbox).
4. Every player checks that the ZeroTier interface has an address (`zerotier-cli listnetworks`,
   or the app shows it) and that the machines answer each other (`ping` the others' ZeroTier
   addresses).
5. Then play as on a local network. Should a game not appear in the list on Linux, the broadcast
   is leaving by the other interface: route it through ZeroTier with
   `sudo ip route add 255.255.255.255/32 dev <zerotier interface>` (the interface's name begins
   with `zt`), or turn the machine's other network off while playing.

The game itself sends nothing to the internet; ZeroTier carries the same packets the local network
would (UDP: the search on port 61190, the game on a port the host's system picks).

## For developers

- Code: `src/GalaxyEggbert/Net/` (engine-independent: `NetProtocol` the messages and their checks,
  `NetLobby`, `NetRules` SB2's rules and texts, `NetReplication` what a step changed in the world),
  `src/GalaxyEggbertCNA/NetSession.*` (CNA's SystemLink session; a stand-in without CNA
  networking), `GalaxyEggbertGameNet.cpp` (the screens, the match), `NetGame.hpp` (its state).
- Worlds: `tools/worlds/world201.py`-`world204.py` with `tools/worlds/arena.py`; briefs in
  `plans/briefs/world201.md`-`world204.md`.
- Tests: `VerifyNetProtocol` (CTest: messages, damaged packets, the lobby, the rules, a
  two-machine replication round). `tools/net_two_players.sh <build> <host script> <join script>`
  runs two instances on one machine (each under its own Xvfb) through the screens
  (`GALAXY_NET_TEST=host|join`) and the match (`--autoplay`, real time); scenarios in
  `tools/autoplay/net/`: `bomb` (with `NET_LIVES=1`), `laid`, `shot`, `vehicle`.
- Logs: the game prints `multiplayer:` lines (who joined, the match, the others seen, every world
  change sent or received).
