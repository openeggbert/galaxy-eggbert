#!/usr/bin/env python3
"""Tries a playthrough's timing over a range and prints which values pass.

A script template holds one ``{D}`` (typically ``wait {D}`` after a sync such as ``until temp = 16``);
each value is substituted, the run played (tools/autoplay.sh, deterministic tick for tick) and
reported. Pick the middle of the passing run so the script keeps a margin both ways.

    python3 tools/autoplay_search.py <template> "<mission> [--spawn x,y,z,yaw]" <from> <to> <step> [run-dir]
    e.g. python3 tools/autoplay_search.py build-probe/p083-stair.txt "83 --spawn 88.5,36,27.5,0" 0 26 2
"""
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]


def main() -> int:
    if len(sys.argv) < 6:
        print(__doc__, file=sys.stderr)
        return 2
    template, mission = Path(sys.argv[1]).read_text(), sys.argv[2]
    lo, hi, step = (int(v) for v in sys.argv[3:6])
    run_dir = Path(sys.argv[6]) if len(sys.argv) > 6 else REPO / "build-probe/autoplay-run-h"
    run_dir.mkdir(parents=True, exist_ok=True)
    script = run_dir / "search-script.txt"
    passing = []
    for d in range(lo, hi + 1, step):
        script.write_text(template.replace("{D}", str(d)))
        r = subprocess.run([str(REPO / "tools/autoplay.sh"), str(REPO / "build-cna-opengl3"), str(script)] + mission.split(),
                           capture_output=True, text=True, env=dict(os.environ, AUTOPLAY_RUN=str(run_dir)), cwd=REPO)
        ok = "autoplay passed" in r.stdout
        fail = next((line for line in r.stdout.splitlines() if " FAIL " in line), "")
        print(f"D={d}: {'ok' if ok else 'fail ' + fail}", flush=True)
        if ok:
            passing.append(d)
    print("passing", passing)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
