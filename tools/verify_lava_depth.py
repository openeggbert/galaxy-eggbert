#!/usr/bin/env python3
"""Render the ghost/lava depth regression (BD-0114) with the real game under Xvfb.

Usage: python3 tools/verify_lava_depth.py build-cna-opengl3 [output-dir]
Needs Pillow and tools/autoplay.sh's Xvfb dependencies. Uses its own worlds and saves;
screenshots/logs remain in output-dir (default build-probe/lava-depth).
"""

import argparse
import os
from pathlib import Path
import subprocess

from PIL import Image

from worldkit import World


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("output", type=Path, nargs="?", default=Path("build-probe/lava-depth"))
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    output = args.output.resolve()
    worlds = output / "worlds"
    worlds.mkdir(parents=True, exist_ok=True)
    # LoadContent needs the ordinary initial world even with --mission.
    for name in ("world001.vwr", "title.vwr"):
        link = worlds / name
        if not link.exists():
            link.symlink_to(repo / "worlds3d" / name)
    script = output / "capture.txt"
    script.write_text("wait 30\ntype ghost\nexpect ghost = 1\nwait 60\nshots 1\nwait 1\nend\n")

    passed = True
    for mission, name, foreground in ((786, "behind", False), (787, "in-front", True)):
        world = World(mission=mission, sky=0)
        world.fill(40, 0, 40, 60, 0, 60, 1)
        world.fill(46, 1, 43, 54, 1, 51, 68)
        if foreground:
            world.fill(48, 2, 53, 52, 2, 53, 68)
        world.spawn = (50, 1, 52)
        world.save(worlds / f"world{mission}.vwr")
        run = output / name
        env = dict(os.environ, AUTOPLAY_RUN=str(run), AUTOPLAY_WORLDS=str(worlds))
        # Remove any earlier capture so a missing new frame cannot pass.
        shot = run / "autoplay_shots" / "0000101.png"
        shot.unlink(missing_ok=True)
        result = subprocess.run(
            [str(repo / "tools/autoplay.sh"), str(args.build.resolve()), str(script), str(mission)],
            env=env, capture_output=True, text=True, timeout=60,
        )
        (output / f"{name}.log").write_text(result.stdout + result.stderr)
        if result.returncode or not shot.exists():
            print(f"FAIL: {name} capture; see {output / (name + '.log')}")
            passed = False
            continue
        with Image.open(shot) as image:
            # Fresh saves use the fixed 800x480 camera. This rectangle is Blupi's
            # upper body, overlapping the pool behind him; foreground lava covers it.
            # Count yellow body pixels, allowing shader rounding instead of golden bytes.
            if image.size != (800, 480):
                raise RuntimeError(f"Unexpected capture size: {image.size}")
            body = image.convert("RGB").crop((375, 340, 425, 390))
            yellow = sum(r > 110 and g > r * .75 and b < r * .45 for r, g, b in body.getdata())
        ok = yellow < 20 if foreground else yellow > 1500
        passed = passed and ok
        print(f"{'PASS' if ok else 'FAIL'}: lava {name}, {yellow} visible yellow body pixels")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
