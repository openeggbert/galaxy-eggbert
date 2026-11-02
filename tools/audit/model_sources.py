#!/usr/bin/env python3
"""Every original 3D source, and whether Galaxy draws it (owner 2026-11-02, OR-0050).

For each Ray Dream scene of speedyblupi-data/originals (the survey's scenes.csv): the sprite icons
it was rendered to (the survey's best matches on object-m.png, element.png and explo.png at IoU
>= 0.80, and the Blupi clip library's matches on blupi.png), whether objects.txt converts it, and
whether its model is in avatars3d (or, for Blupi's scenes, in his clip library). Lists, per
state, the scenes; the actionable list is "matched, not converted": a game sprite rendered from a
scene that Galaxy still draws flat. The .max and .a3d sources are counted at the end (MR-0001; the
.a3d lava and columns are converted separately).

Inputs: build-probe/rd4-survey/ (tools/rd4_to_gltf/survey.py) and build-probe/rd4-blupi/match/
(match_icons.py); writes plans/audit/model_sources.md.

    python3 tools/audit/model_sources.py
"""
from __future__ import annotations

import collections
import csv
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SURVEY = REPO / "build-probe" / "rd4-survey"
BLUPI_MATCH = REPO / "build-probe" / "rd4-blupi" / "match" / "icons.csv"
OBJECTS = REPO / "tools" / "rd4_to_gltf" / "objects.txt"
AVATARS = REPO / "avatars3d" / "objects"
ORIGINALS = REPO.parent / "speedyblupi-data" / "originals"
OUT = REPO / "plans" / "audit" / "model_sources.md"
CONFIDENT = 0.80
# Scenes whose sprites Galaxy draws in 3D by another way than objects.txt.
OTHERWISE = {
    "Blupi/aero-seul.rd4": "the parked overcraft: its model from Blupi's clip library (vehicles/over)",
    "Blupi/jeep-seul.rd4": "the parked jeep: its model from Blupi's clip library (vehicles/jeep)",
    "Blupi/skate.rd4": "the parked skateboard: its model from Blupi's clip library (vehicles/skate)",
    "Decor/lave/lave1.rds": "the lava pool from the .a3d wave (LavaSurface.hpp)",
    "Decor/lave/lave2.rds": "the lava pool from the .a3d wave (LavaSurface.hpp)",
    "Decor/lave/lave3.rds": "the lava pool from the .a3d wave (LavaSurface.hpp)",
    "Decor/lave/lave4.rds": "the lava pool from the .a3d wave (LavaSurface.hpp)",
    "Decor/lave/lave5.rds": "the lava pool from the .a3d wave (LavaSurface.hpp)",
}
# Terrain tiles: drawn as textured cubes so that neighbours join; their scenes are lumpy blocks
# (owner_questions.md Q-004 asks whether the terrain should become models).
TERRAIN_PREFIXES = ("Decor/bois", "Decor/grotte", "Decor/roc", "Decor/techno", "Decor/smooth/", "Decor/fromage",
                    "Decor/enfant", "Decor/maison", "Decor/meca", "Decor/palais/mur", "Decor/tuyau", "Decor/herbe")


def scene_key(folder: str, name: str) -> str:
    return name if folder in ("", ".") else f"{folder}/{name}"


def main() -> int:
    scenes = []
    with open(SURVEY / "scenes.csv", newline="") as f:
        for row in csv.DictReader(f):
            scenes.append(scene_key(row["folder"], row["file"]))
    matched: dict[str, list[str]] = collections.defaultdict(list)
    for sheet in ("object-m", "element", "explo"):
        path = SURVEY / f"{sheet}_icons.csv"
        if not path.exists():
            continue
        with open(path, newline="") as f:
            for row in csv.DictReader(f):
                if float(row["iou"]) >= CONFIDENT:
                    matched[row["scene"]].append(f"{sheet}:{row['icon']}")
    blupi_scenes = set()
    if BLUPI_MATCH.exists():
        with open(BLUPI_MATCH, newline="") as f:
            for row in csv.DictReader(f):
                if row.get("status") in ("confident", "probable"):
                    scene = "Blupi/" + row["scene"]
                    matched[scene].append(f"blupi:{row['icon']}")
                    blupi_scenes.add(scene)
    converted: dict[str, str] = {}
    for line in OBJECTS.read_text().splitlines():
        fields = line.split("#", 1)[0].split()
        if len(fields) >= 4:
            for scene in fields[3].split(","):
                converted[scene] = fields[0]
    exported = {d.name for d in AVATARS.iterdir() if d.is_dir()} if AVATARS.exists() else set()

    def in_game(scene: str) -> bool:
        name = converted.get(scene)
        stem = Path(scene).stem
        return (name is not None and f"{name}_{stem}" in exported) or scene in blupi_scenes

    groups = collections.OrderedDict((k, []) for k in (
        "in game", "in game otherwise", "converted, not exported", "terrain tile (Q-004)",
        "matched, not converted", "no game sprite"))
    for scene in sorted(set(scenes) | set(matched)):
        icons = matched.get(scene, [])
        if in_game(scene):
            groups["in game"].append((scene, icons))
        elif scene in OTHERWISE:
            groups["in game otherwise"].append((scene, icons + [OTHERWISE[scene]]))
        elif scene in converted:
            groups["converted, not exported"].append((scene, icons))
        elif icons and scene.startswith(TERRAIN_PREFIXES):
            groups["terrain tile (Q-004)"].append((scene, icons))
        elif icons:
            groups["matched, not converted"].append((scene, icons))
        else:
            groups["no game sprite"].append((scene, icons))

    lines = ["# Every original 3D source and whether Galaxy draws it (generated)", "",
             "`tools/audit/model_sources.py` (OR-0050). A scene is matched when the survey found one of "
             f"its frames rendered as a game sprite at IoU >= {CONFIDENT:.2f} (object-m, element, explo) "
             "or the Blupi clip library uses it. *Matched, not converted* is the work left: sprites "
             "still drawn flat that have a scene.", "",
             "| State | Scenes |", "|---|---|"]
    lines += [f"| {k} | {len(v)} |" for k, v in groups.items()]
    for state in ("matched, not converted", "terrain tile (Q-004)", "in game otherwise", "converted, not exported",
                  "in game"):
        lines += ["", f"## {state.capitalize()}", "", "| Scene | Sprites rendered from it |", "|---|---|"]
        for scene, icons in groups[state]:
            shown = " ".join(icons[:12]) + (f" (+{len(icons) - 12})" if len(icons) > 12 else "")
            lines.append(f"| `{scene}` | {shown or '-'} |")
    others = collections.Counter(p.suffix.lower() for p in ORIGINALS.rglob("*") if p.suffix.lower() in (".max", ".a3d"))
    lines += ["", "## Other formats", "",
              f"- `.max`: {others['.max']} files, BLOCKED until 3D Studio MAX with a genuine licence (MR-0001).",
              f"- `.a3d`: {others['.a3d']} files, decoded (`tools/a3d/`): the lava's wave and the columns are "
              "converted; the rest are frames of the same objects.",
              "", f"The {len(groups['no game sprite'])} scenes without a game sprite are backdrops, menu "
              "and editor art, revisions and tests: nothing in the game is drawn from them."]
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {OUT}: " + ", ".join(f"{k} {len(v)}" for k, v in groups.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
