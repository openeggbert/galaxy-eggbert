# Galaxy Eggbert 1.0.0-rc1

A 3D remake of *Speedy Blupi* (Epsitec), built on CNA, for Linux, Android and the web.

## What you get

- The whole campaign in 3D: the main hub, fourteen worlds with their missions, and the bonus
  world, played as in the original (the same objects, enemies, vehicles and rules).
- Three players, each with their own progress, and **each player's own galaxy**: a world system
  built in the 3D editor, played from the menu's Galaxy button. Galaxies can be named, exported,
  imported and deleted (with a backup).
- **The 3D world editor**: blocks, objects, paths, Blupi's starts, backgrounds and music; on the
  keyboard and mouse, a gamepad or a touch screen.
- **Demos**: ten in the main menu (they play after 40 s without input); the player records
  their own in a mission (Pause > Record), names, exports and imports them.
- Optional 3D effects (Setup: off, low, medium, high, Ultra), 2x textures, a frame rate up to
  240 fps, full screen, a gamepad, touch controls and the original cheats.
- The game is saved in your user folder (Linux: `~/.local/share/Galaxy Eggbert`; `--user DIR`
  chooses another). An earlier build's files in its own folder are copied there once.

## Downloads

| File | For |
|---|---|
| `GalaxyEggbert-1.0.0-rc1-linux-x86_64.tar.gz` | 64-bit Linux; extract and run `./run-galaxy-eggbert` |
| `GalaxyEggbert-1.0.0-rc1-android-arm64.apk` | Android 7.0 or newer, arm64 phones and tablets |
| `SHA256SUMS.txt` | checksums |

## Known limitations

- **Linux:** this build needs glibc 2.38 or newer (Debian 13, Ubuntu 24.04 and later) and the
  distribution's FFmpeg; an archive for older systems (glibc 2.35) needs a build on such a system.
- **Windows:** not part of this release candidate (its build has not been run and checked yet).
- **Android:** gamepads and the accelerometer have had little testing on real devices.
- Multiplayer is not available.
