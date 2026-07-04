#!/usr/bin/env python3
"""Extracts one BlupiAction record from mobile-eggbert's Tables::table_blupi, live.

Reads ../../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp directly at run time --
does NOT copy the table into galaxy-eggbert (matches CLAUDE.md's mobile-eggbert reuse rules;
same "read live, don't transcribe" pattern make-gif.sh uses for sprite sheets).

Record format (confirmed from Decor::BlupiSearchIcon(), Decor.cpp ~line 2393):
  table_blupi is a flat, 0-terminated list of records:
    {actionId, frameCount, holdLimit, icon_0, icon_1, ..., icon_(frameCount-1)}
  next record starts at i + frameCount + 3.

Usage: extract-blupi-action.py <actionId>
Prints: frameCount, holdLimit, and the icon list (one per line, easy to consume from a shell loop).
"""
import re
import sys
from pathlib import Path

TABLES_CPP = Path(__file__).resolve().parents[2].parent / "mobile-eggbert" / "src" / \
    "WindowsPhoneSpeedyBlupi" / "Tables.cpp"


def parse_table_blupi(text):
    m = re.search(r"table_blupi\[2911\]\s*=\s*\{", text)
    if not m:
        raise RuntimeError("table_blupi array not found")
    start = m.end() - 1
    depth = 0
    end = start
    for idx in range(start, len(text)):
        if text[idx] == '{':
            depth += 1
        elif text[idx] == '}':
            depth -= 1
            if depth == 0:
                end = idx
                break
    body = text[start + 1:end]
    nums = [int(x) for x in re.findall(r"-?\d+", body)]

    records = []
    i = 0
    while i < len(nums) and nums[i] != 0:
        action_id = nums[i]
        frame_count = nums[i + 1]
        hold_limit = nums[i + 2]
        icons = nums[i + 3:i + 3 + frame_count]
        records.append((action_id, frame_count, hold_limit, icons))
        i += frame_count + 3
    return records


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <actionId>", file=sys.stderr)
        sys.exit(1)
    action_id = int(sys.argv[1])
    text = TABLES_CPP.read_text()
    records = parse_table_blupi(text)
    matches = [r for r in records if r[0] == action_id]
    if not matches:
        print(f"actionId {action_id} not found in table_blupi", file=sys.stderr)
        sys.exit(2)
    for action_id, frame_count, hold_limit, icons in matches:
        print(f"frameCount={frame_count}")
        print(f"holdLimit={hold_limit}")
        for icon in icons:
            print(icon)


if __name__ == "__main__":
    main()
