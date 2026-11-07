#!/usr/bin/env python3
"""Read-only helper for plans/audit/animation_coverage.md.

Parses Tables::table_blupi of ../mobile-eggbert (blocks [action, frames, hold, icons...]),
the BlupiAction enum and Galaxy's avatars3d/blupi/clips.txt, and prints for every action the
original's frame count and hold frame next to the Galaxy clip's frame count (20 fps).
Usage: python3 -I tools/audit/animation_coverage.py [--objects]
It only reads; nothing is written.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REF = ROOT.parent / "mobile-eggbert"


def parse_table_blupi():
    src = (REF / "src/WindowsPhoneSpeedyBlupi/Tables.cpp").read_text()
    m = re.search(r"table_blupi\[2911\]\s*=\s*\{(.*?)\};", src, re.S)
    nums = [int(x) for x in re.findall(r"-?\d+", m.group(1))]
    out, i = {}, 0
    while nums[i] != 0:
        if nums[i] < 0:
            # Clear3's header says 70 frames but 100 follow (30 spare -1s); the original's own
            # scan steps over them two at a time (-1 + 3) and lands on Clear4's header.
            i += 2
            continue
        act, n, hold = nums[i], nums[i + 1], nums[i + 2]
        out[act] = (n, hold, nums[i + 3:i + 3 + n])
        i += n + 3
    return out


def parse_enum():
    src = (ROOT / "include/GalaxyEggbert/Def/BlupiAction.hpp").read_text()
    return {int(v): k for k, v in re.findall(r"(\w+)\s*=\s*(\d+)", src)}


def parse_clips():
    clips = {}
    for line in (ROOT / "avatars3d/blupi/clips.txt").read_text().splitlines():
        p = line.split()
        if p and p[0] == "clip":
            clips[p[1]] = len(p) - 2
    return clips


# Galaxy clip name per BlupiAction whose clip is named differently.
ALIAS = {"Teleporte": "Teleporting", "Clear1": "DeathLocked.Clear1", "Clear3": "DeathLocked.Clear3",
         "Clear4": "DeathLocked.Clear4", "Glu": "DeathLocked.Glu", "Drown": "DeathLocked.Drown",
         "Sucette": "PickupBusy.Sucette", "Drink": "PickupBusy.Drink", "Charge": "PickupBusy.Charge"}


def main():
    tb, en, cl = parse_table_blupi(), parse_enum(), parse_clips()
    print("id name orig_frames hold distinct_icons | galaxy_clip frames(@20fps)")
    for act in sorted(set(tb) | set(en)):
        name = en.get(act, "?")
        n, hold, icons = tb.get(act, (0, 0, []))
        print(f"{act:3d} {name:14s} {n:4d} {hold:4d} {len(set(icons)):3d} | {cl.get(ALIAS.get(name, name), '-')}")
    used = {ALIAS.get(n, n) for n in en.values()}
    print("clips with no BlupiAction:", sorted(set(cl) - used))


if __name__ == "__main__":
    main()
