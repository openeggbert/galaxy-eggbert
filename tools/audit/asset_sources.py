#!/usr/bin/env python3
"""Inventory of every original 3D source in speedyblupi-data (plans/audit/asset_sources.csv).

One row per file with a 3D-ish extension (.rd4 .rds .rd3 .max .a3d .3ds .dxf .wrl .glb .obj),
case-insensitive, anywhere under the data tree. For each: its format as sniffed from its bytes,
size, SHA-1, the first file with identical bytes (duplicates), what Galaxy has done with it, and
the audit's per-stage state. Stages are filled only from evidence this script can see:

  opened     the Ray Dream survey read it (build-probe/rd4-survey/scenes.csv, tools/rd4_to_gltf/
             survey.py), tools/a3d/a3d.py read it (.a3d), or another parser exists for its format
  runtime    an object model (tools/rd4_to_gltf/objects.txt) or Blupi's clip library
             (build-probe/rd4-blupi/match/icons.csv, when present) is built from it
  verified   left to the hand-written notes (plans/audit/asset_sources.notes.csv)

IDs (AS-NNNN) are stable: rows already in the CSV keep theirs, new files get the next number.
Hand-written columns come from plans/audit/asset_sources.notes.csv (id,status,verified,notes,
next_action) and override the generated defaults.

Usage: python3 tools/audit/asset_sources.py [--data ../speedyblupi-data] [--check]
"""
import argparse
import csv
import hashlib
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "a3d"))
import a3d  # noqa: E402

EXTS = {".rd4", ".rds", ".rd3", ".max", ".a3d", ".3ds", ".dxf", ".wrl", ".glb", ".obj"}
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(REPO, "plans", "audit", "asset_sources.csv")
NOTES = os.path.join(REPO, "plans", "audit", "asset_sources.notes.csv")
FIELDS = ["id", "path", "ext", "format", "bytes", "sha1", "duplicate_of", "folder", "survey",
          "runtime_use", "opened", "runtime", "verified", "status", "notes", "next_action"]


def sniff(path, ext):
    with open(path, "rb") as f:
        head = f.read(16)
    if head.startswith(b"\xd0\xcf\x11\xe0"):
        return "OLE2 compound file (3ds Max scene)"
    if head.startswith(b"AMAP"):
        return "Amapi 3D (AMAP/BLOKSHAP chunks)"
    if head.startswith(b"glTF"):
        return "glTF binary"
    if head.startswith(b"#VRML"):
        return "VRML"
    if head[:2] == b"MM":
        return "3D Studio (.3ds chunks)"
    if ext in (".rd4", ".rds", ".rd3"):
        return "Ray Dream Studio scene" if ext != ".rd3" else "Ray Dream Designer 3 scene"
    if ext == ".dxf":
        return "AutoCAD DXF"
    return "unknown (" + head[:8].hex() + ")"


def load_survey():
    path = os.path.join(REPO, "build-probe", "rd4-survey", "scenes.csv")
    survey = {}
    if os.path.exists(path):
        with open(path, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                # The data's root is folder "." in the survey, nothing in an asset's path.
                folder = "" if row["folder"] in ("", ".") else row["folder"] + "/"
                survey[(folder + row["file"]).lower()] = row
    return survey


def load_objects():
    uses = {}
    with open(os.path.join(REPO, "tools", "rd4_to_gltf", "objects.txt"), encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].split()
            if len(line) < 4:
                continue
            for scene in line[3].split(","):
                uses.setdefault(scene.lower(), []).append("object:" + line[0])
    return uses


def load_blupi_scenes():
    path = os.path.join(REPO, "build-probe", "rd4-blupi", "match", "icons.csv")
    scenes = {"blupi/marche-profil.rd4": "blupi:skeleton+geometry"}
    if os.path.exists(path):
        with open(path, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                for key in ("scene", "plain_scene", "view_scene"):
                    if row.get(key):
                        scenes.setdefault("blupi/" + row[key].lower(), "blupi:clip-frames")
    return scenes


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--data", default=os.path.join(REPO, "..", "speedyblupi-data"))
    ap.add_argument("--check", action="store_true", help="exit 1 if the CSV is stale")
    args = ap.parse_args()
    root = os.path.realpath(os.path.join(args.data, "originals"))
    if not os.path.isdir(root):
        print("BLOCKED: no originals directory at " + root, file=sys.stderr)
        return 2

    old_ids = {}
    if os.path.exists(OUT):
        with open(OUT, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                old_ids[row["path"]] = row["id"]
    notes = {}
    if os.path.exists(NOTES):
        with open(NOTES, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                notes[row["id"]] = row
    survey = load_survey()
    objects = load_objects()
    blupi = load_blupi_scenes()

    files = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            ext = os.path.splitext(name)[1].lower()
            if ext in EXTS:
                files.append(os.path.join(dirpath, name))
    rows = []
    first_by_hash = {}
    next_id = 1 + max([int(i[3:]) for i in old_ids.values()] or [0])
    for full in files:
        rel = os.path.relpath(full, root)
        ext = os.path.splitext(rel)[1].lower()
        with open(full, "rb") as f:
            digest = hashlib.sha1(f.read()).hexdigest()
        dup = first_by_hash.setdefault(digest, rel)
        if rel in old_ids:
            ident = old_ids[rel]
        else:
            ident = "AS-%04d" % next_id
            next_id += 1
        key = rel.lower()
        surveyed = survey.get(key)
        uses = objects.get(key.split("/", 1)[1] if key.startswith("blupi/") else key, []) + objects.get(key, [])
        if key in blupi:
            uses.append(blupi[key])
        uses = sorted(set(uses))
        fmt = sniff(full, ext)
        if surveyed:
            survey_text = "%s frames=%s animated=%s%s" % (surveyed["status"], surveyed["frames"], surveyed["animated"],
                                                          (" unsupported=" + surveyed["unsupported"]) if surveyed["unsupported"] else "")
            opened = "yes" if surveyed["status"] == "ok" else "partial"
        elif ext == ".dxf":
            survey_text, opened = "", "parser:tools/rd4_to_gltf/dxf.py"
        elif ext == ".a3d":
            try:
                shapes = a3d.read(full)
                faced = [s for s in shapes if s.faces]
                survey_text = "a3d: %d shapes (%d with faces, %d faces)" % (
                    len(shapes), len(faced), sum(len(s.faces) for s in faced))
                opened = "yes"
            except Exception as exc:
                survey_text, opened = "a3d: " + str(exc), "partial"
        else:
            survey_text, opened = "", "no"
        if dup != rel:
            status = "NOT_APPLICABLE"
            next_action = "duplicate of " + dup
        elif uses:
            status = "UNREVIEWED"
            next_action = "fidelity review of the runtime model(s)"
        elif ext == ".max":
            # The owner has no 3ds Max licence and leaves the .max files unused (MR-0001, 2026-11-04);
            # their Ray Dream twins are what the game converts.
            status = "NOT_APPLICABLE"
            next_action = ""
        elif ext == ".a3d":
            status = "UNREVIEWED"
            next_action = "classify; a .rds of the same object is the richer source when one exists (materials, cameras)"
        else:
            status = "UNREVIEWED"
            next_action = "classify: game object, tile source, backdrop, demo or revision"
        row = {
            "id": ident, "path": rel, "ext": ext, "format": fmt, "bytes": os.path.getsize(full), "sha1": digest[:12],
            "duplicate_of": "" if dup == rel else dup, "folder": os.path.dirname(rel), "survey": survey_text,
            "runtime_use": " ".join(uses), "opened": opened, "runtime": "yes" if uses else "no", "verified": "",
            "status": status, "notes": "", "next_action": next_action,
        }
        hand = notes.get(ident)
        if hand:
            for field in ("status", "verified", "notes", "next_action"):
                if hand.get(field):
                    row[field] = hand[field]
        rows.append(row)

    rows.sort(key=lambda r: r["id"])
    import io
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    text = buf.getvalue()
    if args.check:
        current = open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
        if current != text:
            print("asset_sources.csv is stale: run tools/audit/asset_sources.py", file=sys.stderr)
            return 1
        return 0
    with open(OUT, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    # Summary by extension and stage (no double counting: duplicates are their own column).
    from collections import Counter
    by = Counter()
    for r in rows:
        unique = not r["duplicate_of"]
        by[(r["ext"], "files")] += 1
        by[(r["ext"], "unique")] += unique
        by[(r["ext"], "opened")] += unique and r["opened"] not in ("no", "")
        by[(r["ext"], "runtime")] += unique and r["runtime"] == "yes"
        by[(r["ext"], "blocked")] += unique and r["status"] == "BLOCKED"
    print("ext    files unique opened runtime blocked")
    for ext in sorted({k[0] for k in by}):
        print("%-6s %5d %6d %6d %7d %7d" % (ext, by[(ext, "files")], by[(ext, "unique")], by[(ext, "opened")],
                                           by[(ext, "runtime")], by[(ext, "blocked")]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
