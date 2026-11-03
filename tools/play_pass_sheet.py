#!/usr/bin/env python3
"""Contact sheets of a play pass's frames (tools/play_pass.sh): the NNNNNNN.png frames of a run (the
number its tick), in tick order, 20 to a page (4 x 5), each labelled with its tick and Blupi's place
at that tick from the run's autoplay_trace.txt (raw grid x y z).

    python3 tools/play_pass_sheet.py <frames-dir> <out-prefix> [trace]

writes <out-prefix>-p1.jpg, -p2.jpg, ...
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw

COLUMNS, ROWS = 4, 5
W, H = 400, 240


def places(trace: Path) -> dict[int, str]:
    """Blupi's place per traced tick ("t=N at X Y Z ...")."""
    found: dict[int, str] = {}
    if not trace.exists():
        return found
    pattern = re.compile(r"^t=(\d+) at (\S+) (\S+) (\S+)")
    for line in trace.read_text(errors="replace").splitlines():
        m = pattern.match(line)
        if m:
            found[int(m.group(1))] = "%s %s %s" % (m.group(2), m.group(3), m.group(4))
    return found


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    frames_dir, prefix = Path(sys.argv[1]), sys.argv[2]
    trace = Path(sys.argv[3]) if len(sys.argv) > 3 else frames_dir / "autoplay_trace.txt"
    frames = sorted(p for p in frames_dir.glob("*.png") if p.stem.isdigit())
    known = places(trace)
    ticks = sorted(known)
    per_page = COLUMNS * ROWS
    for page in range((len(frames) + per_page - 1) // per_page):
        chunk = frames[page * per_page:(page + 1) * per_page]
        rows = (len(chunk) + COLUMNS - 1) // COLUMNS
        out = Image.new("RGB", (W * COLUMNS, H * rows), "black")
        draw = ImageDraw.Draw(out)
        for i, frame in enumerate(chunk):
            tick = int(frame.stem)
            # The trace's nearest tick at or before the frame's (it records every 60 ticks).
            before = [t for t in ticks if t <= tick]
            place = known[before[-1]] if before else ""
            x, y = (i % COLUMNS) * W, (i // COLUMNS) * H
            out.paste(Image.open(frame).convert("RGB").resize((W, H)), (x, y))
            label = "t%d  %s" % (tick, place)
            draw.rectangle([x, y, x + 6 * len(label) + 8, y + 13], fill="black")
            draw.text((x + 3, y + 1), label, fill="yellow")
        out.save("%s-p%d.jpg" % (prefix, page + 1), quality=85)
    return 0


if __name__ == "__main__":
    sys.exit(main())
