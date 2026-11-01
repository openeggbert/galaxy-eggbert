#!/usr/bin/env python3
"""Source-derived gameplay use-case ledger (plans/audit/gameplay_use_cases.csv).

Enumerates, from ../mobile-eggbert (read only), every BlupiAction, ObjectType and SoundChannel
value and every Decor:: function, with:
  - where it is defined, how often Decor.cpp refers to it and its first references;
  - for actions: how many times the code assigns it (m_blupiAction = ...; 0 = never entered);
  - for object types: how many MoveObjects of that type the 78 campaign worlds hold;
  - which Galaxy files cite it by name (src/, include/, tools/), a pointer to the implementation.
Hand-written columns (status, galaxy_impl, verification, tests, next_action) come from
plans/audit/gameplay_use_cases.notes.csv (keyed by id) and override the defaults.

IDs (GP-NNNN) are stable across runs (kept by key; new keys get the next number).

Usage: python3 tools/audit/gameplay_use_cases.py [--ref ../mobile-eggbert] [--check]
"""
import argparse
import collections
import csv
import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(REPO, "plans", "audit", "gameplay_use_cases.csv")
NOTES = os.path.join(REPO, "plans", "audit", "gameplay_use_cases.notes.csv")
FIELDS = ["id", "key", "kind", "name", "value", "source", "decor_refs", "assigned", "campaign_count",
          "first_refs", "galaxy_refs", "status", "galaxy_impl", "verification", "tests", "next_action"]
ENTRY = re.compile(r"^\s+(\w+)\s*=\s*(\d+)\s*,?\s*(?:///<\s*(?:@brief\s*)?(.*))?$")


def read(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def enum_entries(path):
    out = []
    for no, line in enumerate(read(path).splitlines(), 1):
        m = ENTRY.match(line)
        if m:
            out.append((m.group(1), int(m.group(2)), no))
    return out


def galaxy_corpus():
    corpus = {}
    for top in ("src", "include", "tools"):
        for dirpath, _, names in os.walk(os.path.join(REPO, top)):
            for name in names:
                if name.endswith((".cpp", ".hpp", ".py")):
                    path = os.path.join(dirpath, name)
                    corpus[os.path.relpath(path, REPO)] = read(path)
    return corpus


def cite(corpus, pattern, limit=4):
    rx = re.compile(pattern)
    files = sorted(path for path, text in corpus.items() if rx.search(text))
    return " ".join(files[:limit]) + (" +%d" % (len(files) - limit) if len(files) > limit else "")


def first_lines(lines, rx, limit=3):
    hits = [str(i) for i, line in enumerate(lines, 1) if rx.search(line)]
    return len(hits), " ".join(hits[:limit])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=os.path.join(REPO, "..", "mobile-eggbert"))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    ref = os.path.realpath(args.ref)
    inc = os.path.join(ref, "include", "WindowsPhoneSpeedyBlupi")
    decor_path = os.path.join(ref, "src", "WindowsPhoneSpeedyBlupi", "Decor.cpp")
    if not os.path.exists(decor_path):
        print("BLOCKED: no Decor.cpp under " + ref, file=sys.stderr)
        return 2
    decor = read(decor_path)
    decor_lines = decor.splitlines()
    all_src = "\n".join(read(os.path.join(ref, "src", "WindowsPhoneSpeedyBlupi", n))
                        for n in os.listdir(os.path.join(ref, "src", "WindowsPhoneSpeedyBlupi")) if n.endswith(".cpp"))
    campaign = collections.Counter()
    for name in os.listdir(os.path.join(ref, "worlds")):
        for m in re.finditer(r"\btype=(\d+)", read(os.path.join(ref, "worlds", name))):
            campaign[int(m.group(1))] += 1
    corpus = galaxy_corpus()

    rows = []
    for name, value, line in enum_entries(os.path.join(inc, "def", "BlupiAction.hpp")):
        n, first = first_lines(decor_lines, re.compile(r"BlupiAction::%s\b" % name))
        # Uses as a value (assigned, passed, returned), not compared against.
        assigned = len(re.findall(r"(?<![=!]=\s)(?<![=!]=)\bBlupiAction::%s\b" % name, all_src)) - \
            len(re.findall(r"case\s+BlupiAction::%s\b" % name, all_src))
        rows.append(dict(key="action:" + name, kind="action", name=name, value=value,
                         source="def/BlupiAction.hpp:%d" % line, decor_refs=n, assigned=assigned, campaign_count="",
                         first_refs=first, galaxy_refs=cite(corpus, r"\b%s\b" % name)))
    for name, value, line in enum_entries(os.path.join(inc, "decor", "ObjectType.hpp")):
        n, first = first_lines(decor_lines, re.compile(r"ObjectType::%s\b" % name))
        rows.append(dict(key="object:%d" % value, kind="object", name=name, value=value,
                         source="decor/ObjectType.hpp:%d" % line, decor_refs=n, assigned="",
                         campaign_count=campaign.get(value, 0), first_refs=first,
                         galaxy_refs=cite(corpus, r"ObjectType%d\b" % value)))
    for name, value, line in enum_entries(os.path.join(inc, "def", "SoundChannel.hpp")):
        n, first = first_lines(decor_lines, re.compile(r"SoundChannel::%s\b" % name))
        rows.append(dict(key="sound:%d" % value, kind="sound", name=name, value=value,
                         source="def/SoundChannel.hpp:%d" % line, decor_refs=n, assigned="", campaign_count="",
                         first_refs=first, galaxy_refs=cite(corpus, r"SoundChannel%d\b" % value)))
    seen = set()
    for no, line in enumerate(decor_lines, 1):
        m = re.match(r"^    [A-Za-z].*?\bDecor::(\w+)\(", line)
        if m and m.group(1) not in seen and not line.rstrip().endswith(";"):
            fn = m.group(1)
            seen.add(fn)
            n = len(re.findall(r"\b%s\(" % fn, decor)) - 1
            rows.append(dict(key="function:" + fn, kind="function", name="Decor::" + fn, value="",
                             source="Decor.cpp:%d" % no, decor_refs=n, assigned="", campaign_count="", first_refs="",
                             galaxy_refs=cite(corpus, r"\b%s\b" % fn)))

    old = {}
    if os.path.exists(OUT):
        with open(OUT, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                old[row["key"]] = row["id"]
    notes = {}
    if os.path.exists(NOTES):
        with open(NOTES, newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                notes[row["id"]] = row
    next_id = 1 + max([int(i[3:]) for i in old.values()] or [0])
    for row in rows:
        if row["key"] in old:
            row["id"] = old[row["key"]]
        else:
            row["id"] = "GP-%04d" % next_id
            next_id += 1
        row.update(status="UNREVIEWED", galaxy_impl="", verification="", tests="", next_action="")
        if row["kind"] == "action" and row["assigned"] <= 0 and row["value"] != 0:
            # Some actions are set from numbers (ToBlupiAction(MockeryDetect(...))): a lead only.
            row["next_action"] = "no use as a value found: rule out ToBlupiAction(int) before calling it dead"
        if row["kind"] == "function" and row["decor_refs"] <= 0:
            row["next_action"] = "check callers outside Decor.cpp"
        hand = notes.get(row["id"])
        if hand:
            for field in ("status", "galaxy_impl", "verification", "tests", "next_action"):
                if hand.get(field):
                    row[field] = hand[field]
    rows.sort(key=lambda r: r["id"])
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    text = buf.getvalue()
    if args.check:
        current = read(OUT) if os.path.exists(OUT) else ""
        if current != text:
            print("gameplay_use_cases.csv is stale: run tools/audit/gameplay_use_cases.py", file=sys.stderr)
            return 1
        return 0
    with open(OUT, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    count = collections.Counter((r["kind"], r["status"]) for r in rows)
    for kind in ("action", "object", "sound", "function"):
        print(kind, {s: c for (k, s), c in sorted(count.items()) if k == kind})
    return 0


if __name__ == "__main__":
    sys.exit(main())
