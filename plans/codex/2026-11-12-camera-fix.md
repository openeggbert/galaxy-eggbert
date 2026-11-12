You work in the git worktree /rv/data/development/github.com/openeggbert/galaxy-eggbert-localization (branch `localization`, Galaxy Eggbert, a C++ 3D remake on CNA). Read AGENTS.md / CLAUDE.md here and in the parent directory first and follow them strictly.

CONTEXT: plans/audit/camera_quality_2026-11-12.md (your own earlier measurement, tools/camera_probe.py) found the chase camera's failures and their causes in src/GalaxyEggbertCNA/GalaxyEggbertGameUpdate.cpp (UpdateTickCamera, ~2940-3160) and src/GalaxyEggbertCNA/GameHelpers.hpp (RaymarchWallDistance ~617-629):
1. the final wall correction replaces the smoothed eye directly (snaps up to 4 blocks in one tick);
2. the final safety ray protects the lagging smoothed target, not Blupi's body;
3. the smoothed target can sit inside a wall, making the final check ineffective (eye and target in the same block);
4. shake is added after all collision checks (it can push the eye into a wall);
5. the ray march samples every 0.1 block and can miss thin/grazing intersections.

GOAL: fix these in the camera code so that, on the same routes, the camera (a) never stays inside solid terrain, (b) hides Blupi behind static terrain far less often and for shorter runs, (c) has no one-tick snaps of several blocks except where an obstruction makes an instant pull-in necessary - and then prefer anticipating it (e.g. checking the ray to the body every tick and pulling in quickly but over a few ticks, or a predictive check) without ever showing the inside of a wall. Keep the camera's character otherwise: its distance, height, yaw behaviour (ChaseCameraYaw), the rise over obstacles, the auto-zoom option and the shake's look. No new gameplay mechanics. The camera is Galaxy's own (the original is 2D), so improving it is allowed.

VERIFY (all required, report the numbers):
- Build: export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --parallel 6 (only this existing tree; never create build dirs; never /tmp).
- ctest --test-dir build-cna-opengl3 -j4 --output-on-failure: every test must pass. If golden frames or the golden trace (tools/verify_golden_frames.sh, verify_golden_trace.sh) change because the camera changed, do NOT re-baseline: report which frames changed and by how much, and save before/after images under build-probe/camera-fix/ for review.
- Re-run tools/camera_probe.py on the same missions (011 025 045 065 082 094 105 113 and the hub route) into build-probe/camera-fix/ (add an output-dir option to the tool if needed, keeping build-probe/camera as is for comparison) and write build-probe/camera-fix/report.md with a before/after table per mission (jumps, occluded, longest occlusion, hidden, eye inside) and exact-frame screenshots of the former worst cases after the fix.
- Every autoplay script of those missions must still pass (tools/autoplay.sh build-cna-opengl3 tools/autoplay/worldNNN.txt NNN; the camera must not affect the simulation, and the trace of Blupi's positions must be identical before and after: compare autoplay_trace.txt).
- At most two game instances at once; runs silent (SDL_AUDIODRIVER=dummy).

Rules: do not commit (the reviewer commits). Do not modify ../cna, ../cna-engine, ../easy-gl, ../sharp-runtime, ../mobile-eggbert or anything outside this worktree. Do not touch ../speedyblupi-data. No AI attribution anywhere. Keep the code's comment style (plain explanations, the owner's dates where relevant). When done, print a short summary with the before/after numbers and the list of changed files.
