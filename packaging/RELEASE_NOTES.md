# Galaxy Eggbert 0.1.1

A 3D remake of *Speedy Blupi* (Epsitec), built on CNA, for Windows, Linux, Android and the web browser.

## New in 0.1.1

- **Sounds fixed:** a death no longer plays its sound again and again while Blupi lies in the lava,
  the saw or the trap, and no longer spawns hundreds of effects that slowed the game; small
  explosions in quick succession overlap instead of cutting each other off.
- **Nicknames:** players A, B and C can have a nickname (Setup, *Nickname*). It replaces "Player A"
  in the menu and is your name in the network game.
- **3D editor:** a new world has the yellow Blupi again, at the edge of the board facing the chest
  and the exit, as in Speedy Blupi's editor.

## What you get

- The whole campaign in 3D: the main hub, twelve worlds with their missions, and the bonus
  world, played as in the original (the same objects, enemies, vehicles and rules).
- Three players, each with their own progress, and **each player's own galaxy**: a world system
  built in the 3D editor, played from the menu's Galaxy button. Galaxies can be named, exported,
  imported and deleted (with a backup).
- **The 3D world editor**: blocks, objects, paths, Blupi's starts, backgrounds and music; on the
  keyboard and mouse, a gamepad or a touch screen.
- **Demos**: ten in the main menu (they play after 40 s without input); the player records
  their own in a mission (Pause > Record), names, exports and imports them.
- **Multiplayer** after Speedy Blupi 2's network game: 2 to 4 players on a local network (or a
  virtual one such as ZeroTier), its rules and multiplayer worlds, a lobby with chat (Windows,
  Linux and Android; not in the web browser).
- Optional 3D effects (Setup: off, low, medium, high, Ultra), 2x textures, a frame rate up to
  240 fps, full screen, a gamepad, touch controls and the original cheats.
- The game is saved in your user folder (Windows: `%LOCALAPPDATA%\Galaxy Eggbert`, Linux:
  `~/.local/share/Galaxy Eggbert`; `--user DIR` chooses another; in the browser: the browser's own
  storage for the page). An earlier build's files in its own folder are copied there once.
- The main menu shows the version and the build's date and time, left of Play.

## Downloads

| File | For |
|---|---|
| `GalaxyEggbert-0.1.1-windows-x86_64.zip` | 64-bit Windows; extract and run `GalaxyEggbert.exe` |
| `GalaxyEggbert-0.1.1-linux-x86_64.tar.gz` | 64-bit Linux; extract and run `./run-galaxy-eggbert` |
| `GalaxyEggbert-0.1.1-android-arm64.apk` | Android 7.0 or newer, arm64 phones and tablets |
| `GalaxyEggbert-0.1.1-web.zip` | a web server: extract and serve the folder, open `GalaxyEggbert.html` |
| `SHA256SUMS.txt` | checksums |

## Known limitations

- **Linux:** this build needs glibc 2.38 or newer (Debian 13, Ubuntu 24.04 and later) and the
  distribution's FFmpeg; an archive for older systems (glibc 2.35) needs a build on such a system.
- **Windows:** a cross-build from Linux, played under wine but not yet on Windows itself; it needs
  OpenGL 3.3.
- **Android:** gamepads have had little testing on real devices.
- **Web:** it must be served over HTTP(S) (opening the file from disk does not work) to a browser
  with WebGL 2; the first start downloads about 100 MB. No multiplayer.
- **Multiplayer:** played between several instances on one machine and under wine, not yet between
  two real machines.
