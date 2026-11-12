You work in the git worktree /rv/data/development/github.com/openeggbert/galaxy-eggbert-localization (branch `localization` of Galaxy Eggbert, a C++ 3D remake of Mobile Eggbert on the CNA framework). Read AGENTS.md / CLAUDE.md in this directory and in the parent directory first and follow them strictly.

GOAL: measure the in-play CHASE CAMERA's quality objectively and write a report. Do NOT change game code in this task.

What to measure, over real playthroughs:
1. Camera jumps: per-frame displacement of the camera eye that is much larger than Blupi's own motion that frame (not explained by a teleport, a respawn after death, a mission start or a world change). Report the worst cases with world, tick, eye delta, Blupi delta.
2. Occlusion: frames where solid terrain lies between the camera eye and Blupi (he is hidden), and how long such runs last (consecutive frames). The chase camera already has a wall pull-in and rises over obstacles (src/GalaxyEggbert/Game/ChaseCameraYaw.*, src/GalaxyEggbertCNA/GalaxyEggbertGameUpdate.cpp around UpdateTickCamera); find where it fails.
3. Eye inside solid blocks.

Tools available:
- GALAXY_INTERP_TRACE=<file> makes the game write a line per drawn frame in play: "step N alpha A between B eye X Y Z shake ... lag ... moved ..." (src/GalaxyEggbertCNA/GalaxyEggbertGameDraw.cpp).
- tools/autoplay.sh <build-dir> <script> <mission> plays a mission by script (tools/autoplay/worldNNN.txt, mission = NNN); AUTOPLAY_REALTIME=1 keeps 60 Hz pacing and a draw each tick; AUTOPLAY_RUN=<dir> uses a separate run directory (use build-probe/camera/run-<n>); the trace autoplay_trace.txt there has Blupi's position per tick.
- Worlds: worlds3d/worldNNN.vwr; the engine-independent world reader is in include/GalaxyEggbert/ and src/GalaxyEggbert/Worlds/ (World, blocks, BlockDefinitionRegistry for which blocks are solid/occluding). A small analysis program may be written as a Python script under tools/ or a C++ tool target like the existing tools/Verify*.cpp ones (if C++, add it to CMakeLists.txt like those, but do not add it to CTest).
- Build: export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --target <target> --parallel 4. Use only the existing build-cna-opengl3 tree. Never create build directories, never build in /tmp.
- Runs must be silent: SDL_AUDIODRIVER=dummy (autoplay.sh already sets it).

Pick about 8 varied missions (e.g. 011, 025, 045, 065, 082, 094, 105, 113) plus a hub; prefer AUTOPLAY_REALTIME=1 runs if the trace needs every frame, and keep at most two game instances running at once (another long job, tools/play_pass.sh, is already running on this machine: do not touch build-probe/playpass-1112 or build-probe/play-pass-run-*).

Deliverables:
- build-probe/camera/report.md: the method, the numbers per mission (frames, jumps over a threshold you justify, occluded frames and longest occluded run, eye-in-solid frames), the 10 worst cases each with world/tick/positions and, where you can, a screenshot (AUTOPLAY_SHOTS=N saves frames: see tools/autoplay.sh) or a description of the geometry, and the likely cause in the camera code with file:line references.
- Any analysis script/tool you wrote, kept in tools/ with a header comment saying what it does.

Rules: do not commit (the reviewer commits). Do not modify ../cna, ../cna-engine, ../easy-gl, ../sharp-runtime, ../mobile-eggbert or anything outside this worktree. Do not change game behaviour. Do not add AI attribution anywhere. When done, print a short summary of the findings.
