# Galaxy Eggbert 0.5.0

A 3D remake of *Speedy Blupi* (Epsitec), built on CNA, for Windows, Linux, Android and the web browser.

## New in 0.5.0

- **Twenty-one new cheats** (type them during play; `help` lists them on two pages):
  - **Going back in time:** `back` (five seconds; again to go farther), `mark` / `recall`,
    `restart`, `practice` (a death rewinds instead of costing a life), `replay` (the last ten
    seconds again).
  - **Looking around:** `freecam` (fly the camera, the world held), `here` (land below it),
    `photo` (no overlays), `snap` (a PNG picture in your player folder), `step` (one tick a press
    of F2).
  - **Showing what is hidden:** `xray`, `hitbox`, `trail`, `ids`, `sounds`, `chunks`, `perf`,
    `cascades`; and `freeze` (objects stop, Blupi moves) and `warp` (go to any world).
- **A keyboard on the screen for the cheats:** hold the middle of the screen or the gamepad's LT for
  one second, so phones, tablets and gamepads can type cheats too.
- **My recordings:** your own demo recordings in one place -- play, rename, delete, export, or make
  one a menu demo; Pause > Record saves or discards a new one. A recording keeps its world.
- **The camera** turns up or down when Blupi keeps looking up or down.
- **Fixed:** no magnified loading screen flashes at the start; High's shadows touch the ground again.
- The cheat `opendoors` opens the hubs' doors without giving keys, and the keys cheat is
  `wellkeys`, as in the original Speedy Blupi games.
- 0.5.0 plays the network game with 0.4.1, 0.4.0 and 0.3.0.

## New in 0.4.1

- **The goodbye screen** gives the game's home as **galaxyeggbert.com**.
- **Setup:** the Accelerometer option and its sensitivity show only on a device that has an
  accelerometer (phones and tablets); computers without one no longer show it.
- 0.4.1 plays the network game with 0.4.0 and 0.3.0.

## New in 0.4.0

- **Twenty languages:** Finnish, Danish, Norwegian, Turkish and Greek join the fifteen of 0.3.0. The
  font has their letters (æ ø, ğ ı İ ş, the Greek alphabet); the flags' panel shows twenty flags.
- **Clean text in full screen:** the dots that showed under letters, icons and flags when the game was
  magnified are gone.
- 0.4.0 plays the network game with 0.3.0 (the same network protocol).

## New in 0.3.0

- **Fifteen languages:** besides English, French, German and Czech, the game now speaks Slovak,
  Spanish, Italian, Polish, Brazilian Portuguese, Dutch, Hungarian, Romanian, Swedish, Russian and
  Ukrainian: the menus, the Setup, the help, the training hints, the 3D editor and the network game.
  The game starts in your system's language. The flag in the main menu and the Setup's Language
  button open a panel of the fifteen flags; a tap or a click chooses one, and it is remembered.
- **The font** has the letters these languages need, the Cyrillic ones too.
- **3D editor:** an edit shows at once, also in the large worlds (before, every edit rebuilt the
  whole world's picture).
- **Lava, lightning and wind** no longer blink for a frame when dynamite changes the terrain near them.
- **Multiplayer:** names and chat count their length in letters ("Гравець A" is no longer cut), and
  long button labels are drawn smaller instead of being cut. A 0.3.0 game cannot play with a 0.2.0
  one: everybody needs 0.3.0.
- Czech: the network game's results have their space after the kill count again.

## New in 0.2.0

- **The game in French, German and Czech** besides English: the menus, the Setup, the help, the
  training hints, the 3D editor and the network game. The game starts in your system's language;
  the Setup's new button at the top right switches between the four (English, Français, Deutsch,
  Čeština) and is remembered. The French uses Mobile Eggbert's own texts where it has them. The
  main menu shows the language's flag right of Help; it takes you to that button.
- **The font** has the letters these languages need, and capitals keep their accents.
- **No more hitch when the terrain changes** (dynamite cutting a saw, a drip or a fan): only the
  part of the world around the change is built again.
- **Multiplayer:** names with accents ("Hráč A") reach the other players. A 0.2.0 game cannot play
  with a 0.1.x one: everybody needs 0.2.0.

## New in 0.1.3

- **The 1.5x speed is on F4** now. In 0.1.2 it was on F9, which also made the graphics restart: in
  the web version the game went black until F10 was pressed. Please don't press F9 or F10 in
  earlier versions.

## New in 0.1.2

- **A 1.5x game speed** (F4 since 0.1.3), between F5's normal speed and F6's double speed. The speed
  shows at the bottom left; F5 goes back to normal.

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
| `GalaxyEggbert-0.4.1-windows-x86_64.zip` | 64-bit Windows; extract and run `GalaxyEggbert.exe` |
| `GalaxyEggbert-0.4.1-linux-x86_64.tar.gz` | 64-bit Linux; extract and run `./run-galaxy-eggbert` |
| `GalaxyEggbert-0.4.1-android-arm64.apk` | Android 7.0 or newer, arm64 phones and tablets |
| `GalaxyEggbert-0.4.1-web.zip` | a web server: extract and serve the folder, open `GalaxyEggbert.html` |
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
