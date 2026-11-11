# Galaxy Eggbert — Windows (x86-64)

Start `GalaxyEggbert.exe` from this extracted folder. Keep it beside `Content\`, `worlds3d\`,
`textures3d\` and `avatars3d\`, and beside its DLLs (SDL3, SDL3_mixer, libcurl, libwinpthread).

The game draws with OpenGL 3.3: a graphics driver that supports it is needed (any GPU of the last ten
years with its vendor's driver). The network game uses UDP ports 61190 and 61191 on the local network.
The first time, Windows may ask whether to let the game through its firewall: allow it for
private networks to play with others.

`LICENSE` applies to Galaxy Eggbert; `THIRD_PARTY.txt` lists what it is built on and where its data
comes from, `RELEASE_NOTES.md` what this release holds. Your saved games are in
`%APPDATA%\Galaxy Eggbert` (`--user DIR` chooses another folder).

This build is a cross-build from Linux (MinGW-w64), tried under wine but not yet on Windows itself:
please report anything that does not work.
