# Changelog

All notable changes to Galaxy Eggbert are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the major version is
0, a minor version may change saved games, world files or controls — see the pre-1.0 note in
[`docs/releasing.md`](docs/releasing.md).

## [Unreleased]

### Added

- Nicknames for players A, B and C: the main Setup's Nickname. A nickname replaces "Player A" in the
  menu and is the player's name in the network game (BD-0123).

### Fixed

- A death played its sound again and again while Blupi lay in the lava, the saw or the trap, and
  the saw and the trap spawned their effects every tick (hundreds per death), which also slowed the
  game; the large creature caught a dying Blupi again and again (BD-0121).
- A new world in the 3D editor has the yellow Blupi again, as in Speedy Blupi's editor: at the edge
  of the board, facing the chest and the exit (BD-0124).
- Small explosions in quick succession cut each other off and stuttered; they now overlap as in
  the original (BD-0122).

## [0.1.0] — 2026-11-11

The first release: packages for Windows (x86-64), Linux (x86-64), Android (arm64) and the web
browser on GitHub (tag `v0.1.0`). The packages built on 2026-11-10 as "1.0.0-rc1" were built before
the numbering was decided and were never published; 0.1.0 replaces that number.

### Added

- The whole campaign of *Speedy Blupi* in 3D: the main hub, twelve worlds with their missions and
  the bonus world, with the original's objects, enemies, vehicles and rules.
- Three players with their own progress, and each player's own galaxy: a world system built in the
  3D editor, played from the menu's Galaxy button; named, exported, imported and deleted.
- The 3D world editor: blocks, objects, paths, Blupi's starts, backgrounds and music, on the
  keyboard and mouse, a gamepad or a touch screen.
- Demos: ten in the main menu, the player's own recordings (named, exported, imported).
- Multiplayer after Speedy Blupi 2's network game: 2-4 players on a local network (or a virtual one),
  its rules and worlds 201-212 with Galaxy's own 213-217, a lobby with chat (Windows, Linux,
  Android; not the web).
- Optional 3D effects (off, low, medium, high, Ultra), 2x textures, frame rates up to 240 fps,
  full screen, gamepad and touch controls, the original cheats.
- Packages for Windows (x86-64), Linux (x86-64) and the web browser (`tools/release/`), and an
  Android (arm64) APK.
- Versioning: the version is kept in `VERSION.txt` and shown left of Play on the main menu, with
  the build's date and time under it ([`docs/releasing.md`](docs/releasing.md)).

### Fixed

- A `.vwr` world that is broken or hostile (a shared galaxy, a file put into the custom worlds) is
  refused before it can make the game ask for gigabytes of memory.
- Saving a world in the editor is atomic: a full disk or a crash during the save keeps the file as
  it was, and the editor says "NOT SAVED" instead of ending the game.
- The game no longer ends on its first frame when its folder cannot be written (installed for all
  users, `/opt`, Program Files): the two diagnostic screenshots it saves there are skipped.
- The web build has no Multiplayer entry and no network code (the page's program 35 MB -> 8 MB).

[Unreleased]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.1.0...develop
[0.1.0]: https://github.com/openeggbert/galaxy-eggbert/releases/tag/v0.1.0
