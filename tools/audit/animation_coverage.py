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


def set_sites():
    """BlupiAction name -> Decor.cpp lines that assign it (m_blupiAction / m_blupiActionOuf / BlupiDead)."""
    src = (REF / "src/WindowsPhoneSpeedyBlupi/Decor.cpp").read_text()
    out = {}
    for m in re.finditer(r"(m_blupiActionOuf|m_blupiAction|BlupiDead)\s*(=|\()\s*BlupiAction::(\w+)", src):
        out.setdefault(m.group(3), []).append(src.count("\n", 0, m.start()) + 1)
    return out


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


def parse_tables():
    """Every `Tables::table_x[N] = {...};` of the reference with a flat list of ints."""
    src = (REF / "src/WindowsPhoneSpeedyBlupi/Tables.cpp").read_text()
    out = {}
    for m in re.finditer(r"Tables::(table_\w+)\[[^\]]*\]\s*=\s*\{(.*?)\}\s*;", src, re.S):
        body = re.sub(r"/\*.*?\*/|//[^\n]*", "", m.group(2), flags=re.S)
        out[m.group(1)] = [int(x) for x in re.findall(r"-?\d+", body)]
    return out


def model_icons():
    """(sheet, icon) -> (model directory, pose seconds) from avatars3d/objects/*/icons.txt."""
    out = {}
    for d in sorted((ROOT / "avatars3d/objects").iterdir()):
        f = d / "icons.txt"
        if not f.is_file():
            continue
        for line in f.read_text().splitlines():
            p = line.split()
            if p and p[0] == "icon":
                out[(p[1], int(p[2]))] = (d.name, float(p[3]))
    return out


# type -> (sheet, tables whose icons the original draws or a literal list); Decor.cpp
# MoveObjectStepIcon (8192-9040) and MoveObjectStart's fixed icons (9009-9039).
OBJECT_SEQUENCES = {
    2: ("element", list(range(12, 21))), 3: ("element", list(range(48, 57))),
    4: ("element", ["table_bulldozer_left", "table_bulldozer_right", "table_bulldozer_turn2l", "table_bulldozer_turn2r"]),
    5: ("element", list(range(0, 12))), 6: ("element", list(range(21, 29))), 7: ("element", list(range(29, 37))),
    13: ("element", [68]), 16: ("element", list(range(69, 78))),
    17: ("element", ["table_poisson_left", "table_poisson_right", "table_poisson_turn2l", "table_poisson_turn2r"]),
    19: ("element", [89]),
    20: ("element", ["table_oiseau_left", "table_oiseau_right", "table_oiseau_turn2l", "table_oiseau_turn2r"]),
    21: ("element", ["table_cle"]), 24: ("element", ["table_skate"]), 25: ("element", ["table_shield"]),
    26: ("element", ["table_power"]), 28: ("element", [167]), 29: ("element", [177]), 30: ("element", [178]),
    31: ("object-m", ["table_charge"]), 34: ("element", ["table_glu"]), 36: ("element", ["table_pollution"]),
    37: ("element", ["table_clear"]), 39: ("element", ["table_tresortrack"]), 40: ("element", ["table_invert"]),
    41: ("element", ["table_invertstart"]), 42: ("element", ["table_invertstop"]),
    44: ("element", ["table_guepe_left", "table_guepe_right", "table_guepe_turn2l", "table_guepe_turn2r"]),
    46: ("element", [208]), 47: ("object-m", ["table_chenille"]), 48: ("object-m", ["table_chenillei"]),
    49: ("element", ["table_cle1"]), 50: ("element", ["table_cle2"]), 51: ("element", ["table_cle3"]),
    52: ("object-m", ["table_bridge"]), 53: ("explo", ["table_tentacule"]),
    54: ("element", ["table_creature_left", "table_creature_right", "table_creature_turn2"]),
    55: ("element", [252]), 56: ("element", ["table_dynamitef"]), 57: ("element", ["table_shieldtrack"]),
    96: ("element", ["table_follow1"]), 97: ("element", ["table_follow2"]),
    200: ("blupi", list(range(257, 263))), 201: ("blupi1", list(range(257, 263))),
    32: ("blupi1", ["table_blupih_left", "table_blupih_right", "table_blupih_turn2l", "table_blupih_turn2r"]),
    33: ("blupi1", ["table_blupit_left", "table_blupit_right", "table_blupit_turn2l", "table_blupit_turn2r"]),
}


def objects():
    tabs, mi = parse_tables(), model_icons()
    print("type sheet distinct_icons covered_by_a_model distinct_poses")
    for t, (sheet, seq) in sorted(OBJECT_SEQUENCES.items()):
        icons = set()
        for x in seq:
            icons |= set(tabs[x]) if isinstance(x, str) else {x}
        icons.discard(-1)
        hit = {i for i in icons if (sheet, i) in mi}
        poses = {mi[(sheet, i)] for i in hit}
        print(f"{t:3d} {sheet:8s} {len(icons):3d} {len(hit):3d} {len(poses):3d}  missing={sorted(icons - hit)}")


def main():
    if "--objects" in sys.argv:
        return objects()
    tb, en, cl = parse_table_blupi(), parse_enum(), parse_clips()
    sites = set_sites()
    print("id name orig_frames hold distinct_icons | galaxy_clip frames(@20fps) | Decor.cpp set sites")
    for act in sorted(set(tb) | set(en)):
        name = en.get(act, "?")
        n, hold, icons = tb.get(act, (0, 0, []))
        print(f"{act:3d} {name:14s} {n:4d} {hold:4d} {len(set(icons)):3d} | {cl.get(ALIAS.get(name, name), '-'):>4} | {','.join(map(str, sites.get(name, [])[:4])) or '-'}")
    used = {ALIAS.get(n, n) for n in en.values()}
    print("clips with no BlupiAction:", sorted(set(cl) - used))


if __name__ == "__main__":
    main()
