#!/usr/bin/env python3
"""The campaign's structure, checked over the shipped worlds: the main hub holds the twelve world
signs and the gold-bar tunnel's exit (to 199); each theme hub holds one mission sign per mission
(the original's counts, 4, 5, 4, 6, 8, 6, 5, 4, 5, 7, 5, 5); each mission exists, has a spawn and
one way out: the red arrow (ObjectType7), or for each world's last mission the golden key
(ObjectType21) and no arrow; 199 ends at the red arrow.

    python3 tools/campaigncheck.py
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vwr_inspect  # noqa: E402

MISSIONS = (4, 5, 4, 6, 8, 6, 5, 4, 5, 7, 5, 5)
WORLD_SIGNS = {**{n: 157 + n for n in range(1, 9)}, 9: 309, 10: 411, 11: 412, 12: 413}
MISSION_SIGN1 = 174
EXIT, GOLDEN_KEY = 7, 21
ROOT = Path(__file__).resolve().parents[1] / "worlds3d"


def icons(world) -> set:
    return {raw >> 4 for raw in world["blocks"].values()}


def kinds(world) -> list:
    return [o.get("type") for o in world["objects"]]


def main() -> int:
    problems = []

    def need(condition, message):
        if not condition:
            problems.append(message)

    hub = vwr_inspect.read_world(ROOT / "world001.vwr")
    for n, icon in WORLD_SIGNS.items():
        need(icon in icons(hub), f"world001: no sign of world {n} ({icon})")
    need(kinds(hub).count(EXIT) == 1, "world001: not exactly one exit (the gold bars' tunnel to 199)")

    for n, count in enumerate(MISSIONS, start=1):
        hub_path = ROOT / f"world{n * 10:03d}.vwr"
        hub = vwr_inspect.read_world(hub_path)
        signs = icons(hub)
        for m in range(1, count + 1):
            need(MISSION_SIGN1 + m - 1 in signs, f"{hub_path.name}: no sign of mission {m}")
        need(count == 8 or MISSION_SIGN1 + count not in signs,       # 182 after 181 is the blue bar
             f"{hub_path.name}: a sign past its {count} missions")
        for m in range(1, count + 1):
            path = ROOT / f"world{n * 10 + m:03d}.vwr"
            if not path.exists():
                problems.append(f"{path.name}: missing")
                continue
            world = vwr_inspect.read_world(path)
            k = kinds(world)
            need(world["spawn"] is not None, f"{path.name}: no spawn")
            need(world["mission"] == n * 10 + m, f"{path.name}: its header says mission {world['mission']}")
            if m == count:
                need(k.count(GOLDEN_KEY) == 1 and EXIT not in k,
                     f"{path.name}: world {n}'s last mission must end at one golden key, no arrow")
            else:
                need(k.count(EXIT) == 1 and GOLDEN_KEY not in k, f"{path.name}: not exactly one red arrow")

    bonus = vwr_inspect.read_world(ROOT / "world199.vwr")
    need(kinds(bonus).count(EXIT) == 1, "world199: not exactly one red arrow")

    for p in problems:
        print(p)
    if not problems:
        print(f"campaign: the main hub, {len(MISSIONS)} hubs, {sum(MISSIONS)} missions and 199 are complete")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
