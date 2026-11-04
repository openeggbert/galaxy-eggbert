#!/usr/bin/env python3
"""Re-times a playthrough's synced chains one by one, after a gameplay timing change.

A chain is a block headed by a comment line `# Chain ...` whose first `wait D` follows a sync (in
081/080, `until temp = 19` / `until temp = 0`: the vanishing saucers' cycle), ending at its first
`expect y >= ...`. The script is played; if it fails inside chain k, D is tried around its value on
a copy of the script cut after chain k (so a run ends there), the middle of the first passing run is
written back, and the loop goes on until the whole script passes (deterministic: tick for tick).

    python3 tools/autoplay_chains.py <world-number> [--jobs N] [--build DIR] [--span LO HI] [--step S]
    e.g. python3 tools/autoplay_chains.py 81 --jobs 3 --build build-probe/pass-bin

--span tries D-LO .. D+HI around the current value (default 30 18), never before the cycle the value
is in (D - D % 240). A chain with no passing value stops it: widen the span (`--span 240 0` tries the
whole cycle before: leaving in the previous cycle, the first landing just after the saucers come
back, gave 081's longer chains wide windows on 2026-11-04).
"""
import argparse
import os
import queue
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CYCLE = 240


def chains(lines):
    """[(header_index, wait_index, end_index)]."""
    heads = [i for i, l in enumerate(lines) if l.startswith('# Chain ')]
    out = []
    for n, h in enumerate(heads):
        nxt = heads[n + 1] if n + 1 < len(heads) else len(lines)
        w = next((i for i in range(h, nxt) if lines[i].startswith('wait ')), None)
        e = next((i for i in range(w or h, nxt) if lines[i].startswith('expect y >=')), nxt - 1)
        out.append((h, w, e))
    return out


def play(text, run_dir, build, mission):
    run_dir.mkdir(parents=True, exist_ok=True)
    script = run_dir / 'chain-script.txt'
    script.write_text(text)
    r = subprocess.run([str(REPO / 'tools/autoplay.sh'), str(build), str(script), str(mission)],
                       capture_output=True, text=True, env=dict(os.environ, AUTOPLAY_RUN=str(run_dir)), cwd=REPO)
    trace_path = run_dir / 'autoplay_trace.txt'
    trace = trace_path.read_text(errors='replace') if trace_path.exists() else ''
    last = 0
    for m in re.finditer(r'chain-script\.txt:(\d+):', trace):
        last = int(m.group(1))
    fail = next((l for l in trace.splitlines() if ' FAIL ' in l), '')
    return 'autoplay passed' in r.stdout, last, fail


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('world', type=int)
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--build', default='build-cna-opengl3')
    ap.add_argument('--span', type=int, nargs=2, default=(30, 18))
    ap.add_argument('--step', type=int, default=3)
    args = ap.parse_args()
    script_path = REPO / f'tools/autoplay/world{args.world:03d}.txt'
    build = (REPO / args.build).resolve()
    run_base = REPO / f'build-probe/p{args.world:03d}-chain'
    for _ in range(30):
        lines = script_path.read_text().splitlines()
        ok, last, fail = play('\n'.join(lines) + '\n', Path(f'{run_base}-full'), build, args.world)
        if ok:
            print('the whole script passes', flush=True)
            return 0
        cs = chains(lines)
        k = next((n for n, (h, w, e) in enumerate(cs) if h <= last - 1 <= e), None)
        print(f'fails at line {last}: {fail[:110]} -> chain index {k}', flush=True)
        if k is None or cs[k][1] is None:
            print('not inside a timed chain; stopping', flush=True)
            return 1
        h, w, e = cs[k]
        d0 = int(lines[w].split()[1])
        lo = max(d0 - d0 % CYCLE, d0 - args.span[0], 1)
        values = list(range(lo, d0 + args.span[1] + 1, args.step))
        cut = lines[:e + 1]
        free = queue.Queue()
        for i in range(args.jobs):
            free.put(i + 1)

        def trial(d):
            slot = free.get()
            try:
                t = list(cut)
                t[w] = f'wait {d}'
                return d, play('\n'.join(t) + '\n', Path(f'{run_base}-{slot}'), build, args.world)[0]
            finally:
                free.put(slot)

        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            passing = [d for d, good in sorted(pool.map(trial, values)) if good]
        print(f'chain index {k}, wait {d0}: tried {values[0]}..{values[-1]}, passing {passing}', flush=True)
        if not passing:
            print('no passing value; widen --span', flush=True)
            return 1
        runs, cur = [], [passing[0]]
        for d in passing[1:]:
            if d - cur[-1] == args.step:
                cur.append(d)
            else:
                runs.append(cur)
                cur = [d]
        runs.append(cur)
        best = runs[0][len(runs[0]) // 2]
        lines[w] = f'wait {best}'
        script_path.write_text('\n'.join(lines) + '\n')
        print(f'chain index {k}: wait {d0} -> {best} (passing {runs[0][0]}..{runs[0][-1]})', flush=True)
    return 1


if __name__ == '__main__':
    sys.exit(main())
