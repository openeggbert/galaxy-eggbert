# Changelog

All notable changes to Galaxy Eggbert are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the major version is
0, a minor version may change saved games, world files or controls — see the pre-1.0 note in
[`docs/releasing.md`](docs/releasing.md).

## [Unreleased]

## [0.3.0] — 2026-11-12

Fifteen languages and a picker of flags, a faster editor. Packages for Windows, Linux, Android and
the web on GitHub (tag `v0.3.0`); Android's versionCode is 6. Network protocol 5: 0.3.0 does not
play with 0.2.0.

### Added

- The game in Slovak, Spanish, Italian, Polish, Brazilian Portuguese, Dutch, Hungarian, Romanian,
  Swedish, Russian and Ukrainian: fifteen languages in all. The font has the letters they need, the
  Cyrillic ones too.
- A language picker: the main menu's flag and the Setup's Language button open the fifteen flags with
  their languages' names; a press chooses one.

### Changed

- The network game counts a name's and a chat line's length in letters, not bytes, so that
  Ukrainian's "Гравець A" is not cut: network protocol 5, 0.3.0 cannot play with 0.2.0.
- The network game's buttons draw a long label smaller before they cut it.
- The world editor shows an edit at once in a large world: the background picture is no longer loaded
  again and only the terrain's chunks the edit reaches are rebuilt (world065: 6 ms instead of 76).

### Fixed

- A change of the terrain near lava, lightning or wind (dynamite, the editor) no longer drops their
  animation for one frame.
- Czech: the network game's results put a space after the kill count again ("3 zásahů, 1 smrt").

## [0.2.0] — 2026-11-12

The game in French, German and Czech, and the terrain's hitch on a change gone. Packages for
Windows, Linux, Android and the web on GitHub (tag `v0.2.0`); Android's versionCode is 5.

### Added

- The game in French: the menus, the Setup, the help, the training hints (Mobile Eggbert's own
  French), the editor and the network game. The language follows the system's; the Setup's new
  Language button (top right) switches English and French and is saved (BD-0126).
- The game in German and Czech too; the Language button goes through the four. The font has the
  letters they need, and capitals keep their accents (French's too). The network game carries names
  with accents ("Hráč A"): it cannot be played with 0.1.x (network protocol 4).
- The flag of the game's language in the main menu, right of Help: it opens the Setup, its Language
  button pulsing.

### Fixed

- A hitch when the terrain changes (dynamite cutting a saw, a drip or a fan): the terrain is kept in
  chunks and only the few a change reaches are built again, about 5 ms instead of 60-70 (150 with
  the Ultra effects) in a large world on a desktop (OR-0074).

## [0.1.3] — 2026-11-11

The 1.5x speed moves to F4. Packages for Windows, Linux, Android and the web on GitHub (tag
`v0.1.3`); Android's versionCode is 4.

### Fixed

- The 1.5x speed is on F4 now. On F9 (0.1.2) it also set off CNA's debug simulation of a lost
  graphics context: on the web the game stayed black until F10 (UB-013).

## [0.1.2] — 2026-11-11

A 1.5x game speed. Packages for Windows, Linux, Android and the web on GitHub (tag `v0.1.2`);
Android's versionCode is 3.

### Added

- A 1.5x game speed on F9, between F5's normal and F6's 2x (BD-0125).

## [0.1.1] — 2026-11-11

Fixes for sounds that a player of 0.1.0 reported repeating and slowing the game, nicknames for the
players, and the yellow Blupi in a new editor world. Packages for Windows, Linux, Android and the
web on GitHub (tag `v0.1.1`); Android's versionCode is 2.

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

[Unreleased]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.3.0...develop
[0.3.0]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.1.3...v0.2.0
[0.1.3]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/openeggbert/galaxy-eggbert/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/openeggbert/galaxy-eggbert/releases/tag/v0.1.0
