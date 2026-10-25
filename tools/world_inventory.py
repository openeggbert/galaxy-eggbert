#!/usr/bin/env python3
"""Campaign world inventory: every Mobile Eggbert world next to its Galaxy Eggbert `.vwr`.

Reads the reference levels (`../mobile-eggbert/worlds/worldNNN.txt`, read only) for what each
world is about -- background region, music, how much of the 100x100 grid it uses, its objects and
its signature tiles (hazards, doors, teleports, ...) -- and `worlds3d/worldNNN.vwr` for how far
the 3D world is (tools/vwr_inspect.py's reader). Writes a Markdown table (default stdout):

    python3 tools/world_inventory.py > plans/inventory/worlds.md

The reference's 2D layout is a design reference only (theme, mechanics, difficulty); Galaxy
worlds are authored in 3D, never converted from it (CLAUDE.md, plans/GALAXY_EGGBERT_COMPLETION.md).
"""

from __future__ import annotations

import argparse
import collections
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vwr_inspect  # noqa: E402

# ObjectType ids by role (include/GalaxyEggbert/Def/ObjectType.hpp).
OBJECTS = {
    1: "lift", 47: "lift", 48: "lift", 2: "mine", 3: "mine", 4: "bulldozer", 16: "moving mine",
    17: "fish", 20: "bird", 32: "robot", 33: "creature", 44: "wasp", 54: "big creature",
    96: "follow star", 97: "follow star", 5: "treasure", 6: "egg", 7: "exit", 21: "secret exit",
    49: "key", 50: "key", 51: "key", 12: "crate", 13: "helicopter", 19: "jeep", 24: "skateboard",
    28: "tank", 46: "balloon", 25: "shield", 26: "lollipop", 29: "ammo", 30: "drink",
    31: "charge", 40: "invert", 55: "dynamite", 200: "Blupi", 201: "Blupi", 202: "Blupi",
    203: "Blupi", 53: "tentacle",
}
# Signature tiles (include/GalaxyEggbert/BlockTypes.hpp and the anim groups it names).
TILES = [
    ("lava", range(68, 73)), ("spikes", (347, 373, 374)), ("crusher", range(317, 324)),
    ("saw", range(378, 384)), ("drip", (404,)), ("water", tuple(range(92, 100))),
    ("spring", (211,)), ("fan", range(126, 138)), ("blitz", (304, 305)),
    ("teleport", range(330, 334)), ("switch", (384, 385)), ("bridge", (364,)),
    ("door", range(334, 337)), ("temp", (324,)),
]


def parse_reference(path: Path) -> dict:
    lines = path.read_text(encoding="latin-1").splitlines()
    header = dict(re.findall(r"(\w+)=([^ ]+)", lines[0]))
    grid: dict[tuple[int, int], int] = {}
    objects = []
    section, x = None, 0
    for line in lines[1:]:
        if line.startswith("Decor:"):
            section, x = "decor", 0
            continue
        if line.startswith("BigDecor:"):
            section = "big"
            continue
        if line.startswith("MoveObject:"):
            section = None
            objects.append(int(dict(re.findall(r"(\w+)=([^ ]+)", line)).get("type", -1)))
            continue
        if section == "decor":
            for y, v in enumerate(line.split(",")):
                if v.strip() not in ("", "-1"):
                    grid[(x, y)] = int(v)
            x += 1
    return {"header": header, "grid": grid, "objects": objects}


def category(mission: int) -> str:
    if mission == 1:
        return "main hub"
    if mission == 199:
        return "bonus"
    return "theme hub" if mission % 10 == 0 else "mission"


def vwr_status(path: Path) -> str:
    if not path.exists():
        return "missing"
    w = vwr_inspect.read_world(path)
    heights = len({p[1] for p in w["blocks"]})
    n = len(w["blocks"])
    # tools/GenerateSampleWorld3D.cpp's GenerateMinimalSublevel(): an 11x11 floor and its exit.
    if n <= 121 and len(w["objects"]) <= 1:
        return f"placeholder ({n} blocks)"
    return f"{n} blocks, {heights} heights, {len(w['objects'])} objects"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mobile", type=Path, default=Path("../mobile-eggbert/worlds"))
    ap.add_argument("--vwr", type=Path, default=Path("worlds3d"))
    args = ap.parse_args(argv)
    rows = []
    for path in sorted(args.mobile.glob("world*.txt")):
        mission = int(path.stem[5:])
        ref = parse_reference(path)
        xs = [p[0] for p in ref["grid"]] or [0]
        ys = [p[1] for p in ref["grid"]] or [0]
        objs = collections.Counter(OBJECTS.get(t, f"type {t}") for t in ref["objects"])
        tiles = sorted(name for name, icons in TILES if any(v in icons for v in ref["grid"].values()))
        rows.append((mission, category(mission), ref["header"].get("region", "?"),
                     ref["header"].get("music", "?"), f"{max(xs) - min(xs) + 1}x{max(ys) - min(ys) + 1}",
                     ", ".join(f"{k} {n}" for k, n in sorted(objs.items())), ", ".join(tiles),
                     vwr_status(args.vwr / f"world{mission:03d}.vwr")))
    print("| World | Kind | Region | Music | Grid used | Objects (reference) | Signature tiles (reference) | Galaxy .vwr |")
    print("|---|---|---|---|---|---|---|---|")
    for r in rows:
        print("| " + " | ".join(str(c) for c in r) + " |")
    return 0


if __name__ == "__main__":
    sys.exit(main())
