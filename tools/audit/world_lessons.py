#!/usr/bin/env python3
"""One row per campaign world (plans/audit/world_lessons.csv): the Mobile Eggbert world it
re-imagines, its brief, its playthrough, where the gameplay video shows the original
(plans/evidence/video-timeline.csv), and the hand-written audit of its lesson
(plans/audit/world_lessons.notes.csv: id, lesson, reinterpretation_3d, bypasses, human_play,
status, next_action). IDs (LV-NNNN) are stable.

Usage: python3 tools/audit/world_lessons.py [--check]
"""
import argparse
import csv
import io
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import vwr_inspect  # noqa: E402

OUT = REPO / "plans" / "audit" / "world_lessons.csv"
NOTES = REPO / "plans" / "audit" / "world_lessons.notes.csv"
FIELDS = ["id", "world", "role", "me_world", "brief", "autoplay", "video", "blocks", "heights", "objects",
          "lesson", "reinterpretation_3d", "bypasses", "human_play", "status", "next_action"]


def role(n):
    if n == 1:
        return "main hub"
    if n == 199:
        return "bonus"
    if n == 999:
        return "showcase"
    if n % 10 == 0:
        return "theme hub"
    return "mission"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=str(REPO.parent / "mobile-eggbert"))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    video = {}
    with open(REPO / "plans" / "evidence" / "video-timeline.csv", newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            mission = row["mission"].strip()
            if mission.isdigit():
                start, end = float(row["start_s"]), float(row["end_s"])
                lo, hi = video.get(int(mission), (start, end))
                video[int(mission)] = (min(lo, start), max(hi, end))
    rows = []
    for path in sorted((REPO / "worlds3d").glob("world*.vwr")):
        n = int(path.stem[5:])
        world = vwr_inspect.read_world(path)
        heights = len({p[1] for p in world["blocks"]})
        me = Path(args.ref) / "worlds" / ("world%03d.txt" % n)
        brief = REPO / "plans" / "briefs" / ("world%03d.md" % n)
        script = REPO / "tools" / "autoplay" / ("world%03d.txt" % n)
        span = video.get(n)
        rows.append(dict(world="world%03d" % n, role=role(n), me_world=me.name if me.exists() else "",
                         brief=str(brief.relative_to(REPO)) if brief.exists() else "",
                         autoplay=str(script.relative_to(REPO)) if script.exists() else "",
                         video="%d-%ds" % span if span else "", blocks=len(world["blocks"]), heights=heights,
                         objects=len(world["objects"])))
    old = {}
    if OUT.exists():
        with open(OUT, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                old[row["world"]] = row["id"]
    notes = {}
    if NOTES.exists():
        with open(NOTES, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                notes[row["id"]] = row
    next_id = 1 + max([int(i[3:]) for i in old.values()] or [0])
    for row in rows:
        row["id"] = old.get(row["world"]) or "LV-%04d" % next_id
        if row["world"] not in old:
            next_id += 1
        row.update(lesson="", reinterpretation_3d="", bypasses="", human_play="", status="UNREVIEWED",
                   next_action="read the brief against the original (video, world file), live look")
        for field, value in (notes.get(row["id"]) or {}).items():
            if field in FIELDS and field != "id" and value:
                row[field] = value
    rows.sort(key=lambda r: r["id"])
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    text = buf.getvalue()
    if args.check:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != text:
            print("world_lessons.csv is stale: run tools/audit/world_lessons.py", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8")
    print("worlds %d (missions %d, with video span %d)" % (len(rows), sum(r["role"] == "mission" for r in rows),
                                                          sum(bool(r["video"]) for r in rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
