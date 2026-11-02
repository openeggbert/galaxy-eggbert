#!/usr/bin/env python3
"""How Galaxy draws every block icon and object (plans/audit/render_representation.csv).

Blocks: every object-m.png icon 0-440 with its name and category (mobile-eggbert-reference/
02-tiles.md), Galaxy's render mode (tools/data/blocks.csv, from DumpBlockDefinitions), how many of
the 78 Mobile Eggbert worlds and of Galaxy's shipped worlds use it, and a representation class:

  cube           UniformCube / DirectionalCube: a textured box
  pillar-box     InnerPillarBox: the sprite on an inner box
  thin-bar / plate / ground-plate   ThinBar / InnerFlatPlate / GroundAnchoredPlate
  crossed-planes TripleCrossBillboard
  lift-parts     drawn by LiftVisual.hpp (icon 29)
  native-model   drawn as its original scene's model (BlockModels.hpp's lists and families, where
                 avatars3d/objects has the model)

and a review flag where the tile catalogue says the thing is not bulk material (its §10 notes:
Billboard / ThinMechanical / special-surface / architectural-kit) or its name says it has a
shape of its own (fan, pillar, column, lava, saw, ...) while Galaxy draws it as a cube or crossed
planes: the candidates for "texture pasted on a cube" (VIS rows of the fidelity audit).

Objects: every model directory in avatars3d/objects/ (a 3D model drawn instead of sprites) and the
sprite families that remain billboards by decision.

Hand-written columns come from plans/audit/render_representation.notes.csv (id, status, verdict,
notes, next_action). IDs (VIS-NNNN) are stable.

Usage: python3 tools/audit/render_representation.py [--check]
"""
import argparse
import collections
import csv
import io
import os
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import vwr_inspect  # noqa: E402

OUT = REPO / "plans" / "audit" / "render_representation.csv"
NOTES = REPO / "plans" / "audit" / "render_representation.notes.csv"
FIELDS = ["id", "key", "kind", "icon", "name", "category", "galaxy_render", "representation", "flag",
          "me_worlds", "galaxy_worlds", "status", "verdict", "notes", "next_action"]
MODES = {0: "air", 1: "cube", 2: "cube", 3: "pillar-box", 4: "thin-bar", 5: "plate", 6: "crossed-planes",
         7: "ground-plate"}
MODE_NAMES = {0: "Air", 1: "UniformCube", 2: "DirectionalCube", 3: "InnerPillarBox", 4: "ThinBar",
              5: "InnerFlatPlate", 6: "TripleCrossBillboard", 7: "GroundAnchoredPlate"}
SHAPED = re.compile(r"fan|ventil|pillar|column|colonne|lava|saw|scie|pipe|tuyau|rotor|propeller|tree|"
                    r"crusher|spike|lightning|flame|fire|ladder|rope|chain|grille|grate|girder|pole|"
                    r"post|statue|plant|cobweb|spider web|balustrade|bar\b|rod|spring|wheel|gear",
                    re.IGNORECASE)


def tile_catalogue():
    names = {}
    text = (REPO / "mobile-eggbert-reference" / "02-tiles.md").read_text(encoding="utf-8")
    for line in text.splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) >= 4 and cells[1].isdigit():
            icon = int(cells[1])
            name = cells[2].strip("`")
            category = cells[3]
            names.setdefault(icon, (name, category))
    return names


def reference_usage(ref):
    usage = collections.Counter()
    worlds = sorted((ref / "worlds").glob("world*.txt"))
    for path in worlds:
        text = path.read_text(encoding="utf-8", errors="replace")
        decor = text.split("Decor:", 1)[1] if "Decor:" in text else ""
        decor = decor.split("Objects:", 1)[0] if "Objects:" in decor else decor
        icons = {int(t) for t in re.findall(r"(?<![=\d])(\d+)(?![=\d])", decor.split("\n", 1)[-1])}
        usage.update(icons)
    return usage


def galaxy_usage():
    usage = collections.Counter()
    for path in sorted((REPO / "worlds3d").glob("*.vwr")):
        try:
            world = vwr_inspect.read_world(path)
        except Exception:
            continue
        usage.update({raw >> 4 for raw in world["blocks"].values()})
    return usage


BLOCK_MODELS = REPO / "src" / "GalaxyEggbert" / "Game" / "BlockModels.hpp"
# Animated terrain whose frames BlockModels.hpp draws as models by its base (kAnimatedModelBases,
# kModelOverFloorBases, kFanModelBases, SpringFrameIcon): the frames of each family.
MODEL_FAMILIES = [range(126, 138), range(209, 214), range(317, 324), range(324, 330), range(378, 384),
                  range(404, 409)]


def model_drawn_blocks():
    """Block icons the host draws as their original scene's model: those BlockModels.hpp lists as
    still or sign models (kStillModelIcons, kSignModelIcons) and the frames of its animated
    families, where avatars3d/objects has a model covering the object-m icon."""
    text = BLOCK_MODELS.read_text(encoding="utf-8")
    listed = set()
    for name in ("kStillModelIcons", "kSignModelIcons"):
        m = re.search(name + r"\{([^}]*)\}", text)
        if m:
            listed |= {int(v) for v in re.findall(r"\d+", m.group(1))}
    for family in MODEL_FAMILIES:
        listed |= set(family)
    covered = set()
    for icons_txt in (REPO / "avatars3d" / "objects").glob("*/icons.txt"):
        for line in icons_txt.read_text(encoding="utf-8").splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[0] == "icon" and parts[1] == "object-m":
                covered.add(int(parts[2]))
    return listed & covered


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=str(REPO.parent / "mobile-eggbert"))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    names = tile_catalogue()
    me = reference_usage(Path(args.ref))
    gal = galaxy_usage()
    rows = []
    modelled = model_drawn_blocks()
    with open(REPO / "tools" / "data" / "blocks.csv", newline="", encoding="utf-8") as f:
        for b in csv.DictReader(f):
            icon = int(b["icon"])
            mode = int(b["render"])
            name, category = names.get(icon, ("", ""))
            rep = "lift-parts" if icon == 29 else MODES.get(mode, "?")
            flag = ""
            note_kind = re.search(r"\*\*(Billboard|ThinMechanical|special-surface|architectural-kit|thin-bar)\*\*",
                                  category)
            if rep in ("cube", "crossed-planes") and icon != 0:
                if note_kind and note_kind.group(1) != "architectural-kit":
                    flag = "catalogue says " + note_kind.group(1)
                elif SHAPED.search(name + " " + category.split("—")[0]):
                    flag = "shaped thing by its name"
                elif rep == "crossed-planes":
                    flag = "crossed planes"
            if mode == 0 and icon != 0:
                rep = "not drawn"
            if icon in modelled:
                rep, flag = "native-model", ""
            rows.append(dict(key="block:%d" % icon, kind="block", icon=icon, name=name,
                             category=re.sub(r"\s+", " ", category)[:160],
                             galaxy_render="model (BlockModels.hpp)" if icon in modelled else MODE_NAMES.get(mode, str(mode)),
                             representation=rep, flag=flag, me_worlds=me.get(icon, 0), galaxy_worlds=gal.get(icon, 0)))
    for model in sorted((REPO / "avatars3d" / "objects").iterdir()):
        if model.is_dir():
            icons = []
            icons_txt = model / "icons.txt"
            if icons_txt.exists():
                for line in icons_txt.read_text(encoding="utf-8").splitlines():
                    parts = line.split()
                    if len(parts) >= 3 and parts[0] == "icon":
                        icons.append(parts[1] + ":" + parts[2])
            rows.append(dict(key="model:" + model.name, kind="object-model", icon=" ".join(icons[:6]) +
                             (" +%d" % (len(icons) - 6) if len(icons) > 6 else ""), name=model.name, category="",
                             galaxy_render="SkinnedModelEXT", representation="native-model", flag="",
                             me_worlds="", galaxy_worlds=""))
    for key, name in (("billboard:effects", "explosions, splashes, sparkles, goo (explo/element effect icons)"),
                      ("billboard:charge", "charge (ObjectType 31)"), ("billboard:shield", "shield sparks (25)")):
        rows.append(dict(key=key, kind="object-billboard", icon="", name=name, category="", galaxy_render="billboard",
                         representation="2d-effect", flag="kept by decision 2026-10-24", me_worlds="", galaxy_worlds=""))

    old = {}
    if OUT.exists():
        with open(OUT, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                old[row["key"]] = row["id"]
    notes = {}
    if NOTES.exists():
        with open(NOTES, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                notes[row["id"]] = row
    next_id = 1 + max([int(i[4:]) for i in old.values()] or [0])
    for row in rows:
        row["id"] = old.get(row["key"]) or "VIS-%04d" % next_id
        if row["key"] not in old:
            next_id += 1
        row.update(status="UNREVIEWED", verdict="", notes="", next_action="look at it live" if row["flag"] else "")
        hand = notes.get(row["id"])
        if hand:
            for field in ("status", "verdict", "notes", "next_action"):
                if hand.get(field):
                    row[field] = hand[field]
    rows.sort(key=lambda r: r["id"])
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    text = buf.getvalue()
    if args.check:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != text:
            print("render_representation.csv is stale: run tools/audit/render_representation.py", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8")
    flagged = [r for r in rows if r["flag"] and r["kind"] == "block" and (r["me_worlds"] or r["galaxy_worlds"])]
    print("rows %d; blocks flagged and used in a world: %d" % (len(rows), len(flagged)))
    print(collections.Counter(r["representation"] for r in rows))
    return 0


if __name__ == "__main__":
    sys.exit(main())
