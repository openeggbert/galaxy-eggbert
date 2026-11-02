#!/usr/bin/env python3
"""Tries a playthrough's timing over a range and prints which values pass.

A script template holds one ``{D}`` (typically ``wait {D}`` after a sync such as ``until temp = 16``);
each value is substituted, the run played (tools/autoplay.sh, deterministic tick for tick) and
reported. Pick the middle of the passing run so the script keeps a margin both ways.

    python3 tools/autoplay_search.py [--jobs N] <template> "<mission> [--spawn x,y,z,yaw]" <from> <to> <step> [run-dir]
    e.g. python3 tools/autoplay_search.py build-probe/p083-stair.txt "83 --spawn 88.5,36,27.5,0" 0 26 2

--jobs N plays N values at once, each in its own run directory (<run-dir>-1 .. -N).
"""
import os
import queue
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]


def play(template: str, mission: str, d: int, run_dir: Path):
    run_dir.mkdir(parents=True, exist_ok=True)
    script = run_dir / "search-script.txt"
    script.write_text(template.replace("{D}", str(d)))
    r = subprocess.run([str(REPO / "tools/autoplay.sh"), str(REPO / "build-cna-opengl3"), str(script)] + mission.split(),
                       capture_output=True, text=True, env=dict(os.environ, AUTOPLAY_RUN=str(run_dir)), cwd=REPO)
    ok = "autoplay passed" in r.stdout
    fail = next((line for line in r.stdout.splitlines() if " FAIL " in line), "")
    return d, ok, fail


def main() -> int:
    args = sys.argv[1:]
    jobs = 1
    if len(args) >= 2 and args[0] == "--jobs":
        jobs = max(1, int(args[1]))
        args = args[2:]
    if len(args) < 5:
        print(__doc__, file=sys.stderr)
        return 2
    template, mission = Path(args[0]).read_text(), args[1]
    lo, hi, step = (int(v) for v in args[2:5])
    run_dir = Path(args[5]) if len(args) > 5 else REPO / "build-probe/autoplay-run-h"
    values = list(range(lo, hi + 1, step))
    passing = []
    if jobs == 1:
        results = (play(template, mission, d, run_dir) for d in values)
    else:
        free = queue.Queue()
        for i in range(jobs):
            free.put(Path(f"{run_dir}-{i + 1}"))

        def play_in_free_dir(d):
            directory = free.get()
            try:
                return play(template, mission, d, directory)
            finally:
                free.put(directory)

        pool = ThreadPoolExecutor(max_workers=jobs)
        futures = [pool.submit(play_in_free_dir, d) for d in values]
        results = (f.result() for f in futures)
    for d, ok, fail in results:
        print(f"D={d}: {'ok' if ok else 'fail ' + fail}", flush=True)
        if ok:
            passing.append(d)
    print("passing", passing)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
