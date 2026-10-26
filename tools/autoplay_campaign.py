#!/usr/bin/env python3
"""Writes tools/autoplay/campaign.txt: a new game played through from the main hub, world by world
(each world's hub chain), then the gold bars and world 199.

    python3 tools/autoplay_campaign.py [last-world]   # default 12; fewer for a partial run
    tools/autoplay.sh build-cna-opengl3 tools/autoplay/campaign.txt 1
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
# The world signs' niches in the main hub (worlds3d/world001.vwr): (x, walk, z).
SIGNS = {1: (55, 1, 73), 2: (45, 7, 64), 3: (55, 3, 70), 4: (55, 5, 67), 5: (45, 9, 61), 6: (45, 11, 58),
         7: (55, 7, 64), 8: (55, 11, 58), 9: (45, 5, 67), 10: (55, 9, 61), 11: (45, 3, 70), 12: (45, 1, 73)}
# Each tier's south row (where Blupi arrives beside a sign of that walk): walk 1 -> z 74, 3 -> 71...
def front(walk):
    return 74 - 3 * (walk - 1) // 2

last = int(sys.argv[1]) if len(sys.argv) > 1 else 12
out = ["# A new game played through from the main hub (plans/briefs/world001.md): world by world,",
       "# each world's sign, its hub's chain (worldN0.txt) to its golden key, back in the main hub",
       "# beside that world's sign; then the summit's tunnel past the twelve gold bars (each opened",
       "# by a world won) to world 199 and the win screen. The tiers are climbed and left at x 50,",
       "# between the niches: a jump of two up onto the tier above, a step off its edge down.",
       "wait 30", "expect mission = 1"]

def move(frm, to_walk):
    """From standing on tier `frm` (at x 50, its south row) to tier `to_walk`'s south row."""
    lines, w = [], frm
    while w < to_walk:
        z = front(w)
        lines += [f"go 50 {z + 0.1:.1f} tol 0.15", "face 0", f"go 50 {z - 1.6:.1f} jump 1.6 tol 0.3",
                  "until ground = 1", f"expect y >= {w + 2 - 0.05:.2f}"]
        w += 2
    while w > to_walk:
        z = front(w)
        lines += [f"go 50 {z + 3.0:.1f}", "until ground = 1", f"expect y <= {w - 2 + 0.05:.2f}"]
        w -= 2
    return lines

walk = 1
for n in range(1, last + 1):
    sx, sw, sz = SIGNS[n]
    out.append(f"# World {n}: its sign ({sx}, {sw}, {sz}).")
    out.append(f"go 50 {front(walk)}")
    out += move(walk, sw)
    out += [f"go {sx} {front(sw)} tol 0.15", f"go {sx} {sz} until mission = {n * 10} max 900",
            f"until mission = {n * 10} max 600", f"include world{n * 10:03d}.txt", "expect mission = 1",
            "expect phase = Play", "wait 30"]
    walk = sw
if last == 12:
    out += ["# The summit (walk 13) and its tunnel (z 44) west past the twelve gold bars to world 199.",
            f"go 50 {front(walk)}"] + move(walk, 11) + [
            "go 50 59.1 tol 0.15", "face 0", "go 50 57.4 jump 1.6 tol 0.3", "until ground = 1", "expect y >= 12.95",
            "go 58.3 46", "go 58.3 44 tol 0.15", "go 43 44 until mission = 199 max 900",
            "until mission = 199 max 600", "include world199.txt"]
(REPO / "tools/autoplay/campaign.txt").write_text("\n".join(out) + "\n")
print(len(out), "lines")
