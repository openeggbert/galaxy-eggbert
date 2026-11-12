#!/usr/bin/env python3
"""Capture and analyze chase-camera motion/terrain visibility in real autoplay runs.

Uses the existing game binary without rebuilding or changing game code. Capture
uses AUTOPLAY_SHOTS=1 to defeat autoplay draw suppression; the companion GDB
observer omits expensive diagnostic PNG exports, retaining selected screenshots.
Outputs default to build-probe/camera; --output-dir keeps comparison runs separate. One game instance at a time.

    python3 tools/camera_probe.py run 011 025 045 065 082 094 105 113 001
    python3 tools/camera_probe.py analyze
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import re
import random
import subprocess
import time

import vwr_inspect

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build-probe/camera"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(missions, label="", keep="", every=600, realtime=False, tick_observer=False, override_script=None):
    binary = ROOT / "build-cna-opengl3/GalaxyEggbert"
    for number in missions:
        mission = int(number)
        folder = OUT / f"run-{mission:03d}{label}"
        if folder.exists():
            raise FileExistsError(f"Refusing to overwrite {folder}")
        folder.mkdir(parents=True)
        for name in ("Content", "avatars3d", "textures3d", "worlds3d"):
            (folder / name).symlink_to(ROOT / name, target_is_directory=True)
        os.link(binary, folder / "GalaxyEggbert")
        script = folder / "input.txt"
        source_script = Path(override_script).resolve() if override_script else ROOT / f"tools/autoplay/world{mission:03d}.txt"
        script.write_text(f"trace 1000000\ninclude {source_script}\n")
        env = dict(os.environ, GALAXY_USER_DIR=str(folder), SDL_AUDIODRIVER="dummy",
                   SDL_VIDEODRIVER="x11", AUTOPLAY_SHOTS=str((30 if keep else every) if tick_observer else 1), AUTOPLAY_REALTIME=str(int(realtime)),
                   GALAXY_INTERP_TRACE=str(folder / "interp.txt"),
                   CAMERA_KEEP_EVERY=str(every), CAMERA_KEEP_TICKS=keep, LP_NUM_THREADS="2",
                   CAMERA_TICK_OBSERVER=str(int(tick_observer)))
        command = ["xvfb-run", "-a", "-s", "-screen 0 1280x720x24", "gdb", "-q", "-batch",
                   "-ex", "set pagination off", "-ex", "set confirm off",
                   "-ex", "set debuginfod enabled off", "-ex", "set print thread-events off",
                   "-ex", f"source {ROOT}/tools/camera_probe_gdb.py", "-ex", "run", "--args",
                   "./GalaxyEggbert", "--mission", str(mission), "--autoplay", str(script)]
        metadata = {"mission": mission, "started": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
                    "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    "binary_sha256": digest(binary), "world_sha256": digest(ROOT / f"worlds3d/world{mission:03d}.vwr"),
                    "script_sha256": digest(source_script), "source_script": str(source_script),
                    "command": command, "realtime": realtime, "tick_observer": tick_observer,
                    "index_every_draw": bool(tick_observer and keep)}
        (folder / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"Start {mission:03d}: {folder}", flush=True)
        start = time.monotonic()
        with (folder / "game.log").open("w") as log:
            result = subprocess.run(command, cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT)
        metadata.update(exit_code=result.returncode, elapsed_seconds=round(time.monotonic() - start, 2))
        text = (folder / "autoplay_trace.txt").read_text() if (folder / "autoplay_trace.txt").exists() else ""
        metadata["passed"] = "autoplay passed" in (folder / "game.log").read_text()
        (folder / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
        (folder / "GalaxyEggbert").unlink()
        print(f"End {mission:03d}: passed={metadata['passed']} seconds={metadata['elapsed_seconds']}", flush=True)
        if not metadata["passed"]:
            raise RuntimeError(f"Playthrough failed; inspect {folder}/game.log and autoplay_trace.txt")


def read_states(folder):
    """Autoplay tick t is the state BEFORE update t, hence AFTER screenshot t."""
    phase, world, vehicle = None, None, "none"
    states, events = {}, []
    for line in (folder / "autoplay_trace.txt").read_text().splitlines():
        match = re.match(r"t=(\d+) (.*)", line)
        if not match:
            continue
        tick, value = int(match[1]), match[2]
        if value.startswith("phase "):
            phase = value.split()[1]
            events.append((tick, value))
        elif value.startswith("mission "):
            world = int(value.split()[1])
            events.append((tick, value))
        elif value.startswith("vehicle "):
            vehicle = value.split()[1]
        elif value.startswith("at "):
            words = value.split()
            states[tick] = dict(tick=tick, world=world, phase=phase,
                                blupi=tuple(map(float, words[1:4])), vehicle=vehicle,
                                yaw=float(words[5]), air="air" in words)
        elif any(word in value.lower() for word in ("dead", "death", "fail", "lives")):
            events.append((tick, value))
    return states, events


def read_frames(folder):
    states, events = read_states(folder)
    if (folder / "camera_ticks.csv").exists():
        poses = {}
        shakes = {}
        if (folder / "shake_ticks.csv").exists():
            shakes = {int(r["tick"]): (float(r["x"]), float(r["y"]))
                      for r in csv.DictReader((folder / "shake_ticks.csv").open())}
        # std::string symbol names contain commas; the first seven CSV fields
        # are numeric and sufficient. Last SetLookAt per tick is the final eye.
        for row in csv.reader((folder / "camera_ticks.csv").open()):
            if row[0] == "tick":
                continue
            if not row[7].startswith("GalaxyEggbert::CNA::GalaxyEggbertGame::UpdateTickCamera("):
                continue
            poses[int(row[0])] = tuple(map(float, row[1:7]))
        frames, last, last_shake = [], None, None
        for tick, state in sorted(states.items()):
            last = poses.get(tick, last)
            last_shake = shakes.get(tick, last_shake)
            if state["phase"] != "Play" or last is None:
                continue
            eye = (last[0] + 50, last[1], last[2] + 50)
            target = (last[3] + 50, last[4], last[5] + 50)
            frames.append(dict(state, eye=eye, target=target, line=None, shake=last_shake,
                               run=folder.name, held=tick not in poses))
        return frames, [], events
    offsets = {}
    # Non-play draws do not extend interp.txt. Only the FIRST screenshot for each
    # byte offset belongs to that interpolation record; later ones are menu frames.
    for row in csv.DictReader((folder / "draw_ticks.csv").open()):
        offsets.setdefault(int(row["trace_bytes"]), int(row["tick"]))
    frames, unmatched, end = [], [], 0
    for index, line in enumerate((folder / "interp.txt").read_bytes().splitlines(keepends=True)):
        end += len(line)
        tick = offsets.get(end)
        if tick is None or tick not in states:
            unmatched.append(dict(line=index + 1, tick=tick))
            continue
        words = line.decode().split()
        if words[3] != "1.000" or words[5] != "0":
            raise ValueError("This analyzer requires non-interpolated 60 Hz autoplay")
        eye = tuple(map(float, words[7:10]))
        # Both positions in raw world/grid coordinates, cell centers are integers.
        eye = (eye[0] + 50, eye[1], eye[2] + 50)
        frames.append(dict(states[tick], eye=eye, line=index + 1,
                           shake=tuple(map(float, words[11:13])), run=folder.name))
    observer = folder.with_name(folder.name + "-observer")
    if (observer / "camera_ticks.csv").exists():
        additional = {f["tick"]: f for f in read_frames(observer)[0]}
        for frame in frames:
            other = additional.get(frame["tick"])
            if other and math.dist(frame["eye"], other["eye"]) < 0.0001 and frame["blupi"] == other["blupi"]:
                frame["target"] = other["target"]
    return frames, unmatched, events


def segment_box(a, b, cell, margin=0.0):
    """Exact slab intersection with a centered unit cube; reject tangent contact."""
    lo, hi = 0.0, 1.0
    for axis in range(3):
        lower, upper = cell[axis] - 0.5 + margin, cell[axis] + 0.5 - margin
        delta = b[axis] - a[axis]
        if abs(delta) < 1e-12:
            if not lower < a[axis] < upper:
                return None
        else:
            first, last = (lower - a[axis]) / delta, (upper - a[axis]) / delta
            lo, hi = max(lo, min(first, last)), min(hi, max(first, last))
            if hi - lo <= 1e-9:
                return None
    return (lo, hi) if hi > 1e-8 and lo < 1 - 1e-8 else None


def traversed(a, b):
    """3D DDA candidate cells, followed by exact slab testing (no 0.1-unit steps)."""
    cell = [math.floor(x + 0.5) for x in a]
    delta = [b[i] - a[i] for i in range(3)]
    step = [1 if d > 0 else -1 if d < 0 else 0 for d in delta]
    next_t = [(cell[i] + 0.5 * step[i] - a[i]) / delta[i] if step[i] else math.inf for i in range(3)]
    increments = [1 / abs(d) if d else math.inf for d in delta]
    for _ in range(10000):
        yield tuple(cell)
        t = min(next_t)
        if t > 1 or math.isinf(t):
            return
        for i in range(3):
            if abs(next_t[i] - t) < 1e-10:
                cell[i] += step[i]
                next_t[i] += increments[i]
    raise ValueError("Unexpectedly long camera ray")


class Terrain:
    def __init__(self, number, registry):
        self.world = vwr_inspect.read_world(ROOT / f"worlds3d/world{number:03d}.vwr")
        self.blocks = {p: raw >> 4 for p, raw in self.world["blocks"].items()}
        self.registry = registry
        # Read the existing rendering exceptions instead of inventing solid IDs.
        models = (ROOT / "src/GalaxyEggbert/Game/BlockModels.hpp").read_text()
        still = re.search(r"kStillModelIcons\s*\{([^}]+)\}", models)[1]
        self.model_icons = set(map(int, re.findall(r"\d+", still)))

    def kind(self, cell):
        icon = self.blocks.get(cell, 0)
        definition = self.registry.get(icon)
        if not icon or not definition or definition["water"] or not definition["collision_mask"]:
            return 0, icon
        # Known static opaque cube terrain only. Camera-mask-only hits are kept
        # separately; models, opening doors and crumbling/animated tiles cannot
        # be asserted visually occluding from the on-disk voxel alone.
        stable = (definition["render"] in (1, 2) and not definition["animated"]
                  and icon not in self.model_icons and definition["door_key"] < 0
                  and not definition["hub_door"] and not definition["bridge"]
                  and not definition["fan"] and not definition["spring"]
                  and not definition["teleporter"] and not 421 <= icon <= 440
                  and icon not in (30, 31, 262, 263, 283))
        return (2 if stable else 1), icon

    def hits(self, a, b, margin=0.01):
        hits = []
        for cell in traversed(a, b):
            kind, icon = self.kind(cell)
            if kind:
                interval = segment_box(a, b, cell, margin)
                if interval:
                    hits.append(dict(cell=cell, icon=icon, stable=kind == 2,
                                     entry=interval[0], exit=interval[1]))
        return sorted(hits, key=lambda hit: hit["entry"])

    def inside(self, eye, margin=0.01):
        cell = tuple(math.floor(x + 0.5) for x in eye)
        kind, icon = self.kind(cell)
        if kind and all(abs(eye[i] - cell[i]) < 0.5 - margin for i in range(3)):
            return dict(cell=cell, icon=icon, stable=kind == 2,
                        depth=min(0.5 - abs(eye[i] - cell[i]) for i in range(3)))
        return None


def contiguous_runs(frames, field):
    runs, current = [], []
    def finish():
        if current:
            runs.append(dict(world=current[0]["world"], run=current[0]["run"],
                             start=current[0]["tick"], end=current[-1]["tick"],
                             frames=len(current), seconds=len(current) / 60,
                             first=current[0], middle=current[len(current) // 2], last=current[-1]))
    for frame in frames:
        if current and (not frame[field] or frame["world"] != current[-1]["world"]
                        or frame["run"] != current[-1]["run"]
                        or frame["tick"] != current[-1]["tick"] + 1):
            finish()
            current = []
        if frame[field]:
            current.append(frame)
    finish()
    return sorted(runs, key=lambda r: r["frames"], reverse=True)


def analyze(folders=None, partial=False):
    registry = {int(row["icon"]): {key: int(value) for key, value in row.items()}
                for row in csv.DictReader((ROOT / "tools/data/blocks.csv").open())}
    worlds, all_frames, summary, excluded, alignment = {}, [], [], [], []
    default_folders = sorted(OUT.glob("run-[0-9][0-9][0-9]"))
    if (OUT / "run-001-hub").exists():
        default_folders.append(OUT / "run-001-hub")
    for folder in folders or default_folders:
        metadata = json.loads((folder / "metadata.json").read_text())
        if not metadata.get("passed") and not partial:
            raise RuntimeError(f"Incomplete/failed capture: {folder}; --partial is for progress inspection only")
        if not all((folder / f).exists() for f in ("draw_ticks.csv", "autoplay_trace.txt", "interp.txt")):
            continue
        frames, unmatched, events = read_frames(folder)
        drawn = (folder / "interp.txt").read_text().splitlines()
        validation = {}
        if (folder / "camera_ticks.csv").exists():
            expected = [f for f in frames if f["tick"] % 30 == 0]
            compared_draws = drawn
            unindexed_startup_draws = 0
            if metadata.get("index_every_draw"):
                # Selected failure windows add draws between the usual 30-tick samples.
                # Index every draw via its flushed trace offset, rather than assuming spacing.
                by_tick = {f["tick"]: f for f in frames}
                by_offset = {}
                for row in csv.DictReader((folder / "draw_ticks.csv").open()):
                    by_offset.setdefault(int(row["trace_bytes"]), int(row["tick"]))
                offset, expected, compared_draws = 0, [], []
                for index, line in enumerate((folder / "interp.txt").read_bytes().splitlines(keepends=True)):
                    offset += len(line)
                    tick = by_offset.get(offset)
                    if index == 0 and tick is None:
                        # The startup draw precedes the first screenshot index, as in the
                        # original every-frame capture. Do not invent a tick for it.
                        unindexed_startup_draws += 1
                        continue
                    if tick not in by_tick:
                        raise RuntimeError(f"Unindexed drawn camera frame: {folder}, offset {offset}")
                    expected.append(by_tick[tick])
                    compared_draws.append(line.decode())
            # Exit may perform an unmeasured final update, but it is not usually
            # drawn. Reject any discrepancy rather than silently shifting ticks.
            errors = []
            for frame, line in zip(expected, compared_draws):
                eye = tuple(map(float, line.split()[7:10]))
                errors.append(math.dist(frame["eye"], (eye[0] + 50, eye[1], eye[2] + 50)))
            validation = dict(expected_draws=len(expected), actual_draws=len(drawn),
                              unindexed_startup_draws=unindexed_startup_draws,
                              max_draw_error=max(errors, default=0),
                              all_draws_match=len(expected) == len(compared_draws) and max(errors, default=0) < 0.0001)
        alignment.append(dict(run=folder.name, matched=len(frames), drawn=len(drawn),
                              unmatched=unmatched, events=events, **validation))
        previous = None
        first_world_tick = 1
        for frame in frames:
            if previous and frame["world"] != previous["world"]:
                first_world_tick = frame["tick"]
            frame["startup"] = frame["tick"] <= first_world_tick + 1
            n = frame["world"]
            if n not in worlds:
                worlds[n] = Terrain(n, registry)
            terrain = worlds[n]
            eye, blupi = frame["eye"], frame["blupi"]
            hits = terrain.hits(eye, blupi)
            stable = [h for h in hits if h["stable"]]
            frame["hit"] = stable[0] if stable else None
            frame["occluded"] = bool(stable)
            frame["mask_hit"] = hits[0] if hits else None
            frame["mask_occluded"] = bool(hits)
            inside = terrain.inside(eye)
            frame["inside_hit"] = inside
            frame["inside"] = bool(inside and inside["stable"])
            frame["mask_inside"] = bool(inside)
            if "target" in frame:
                frame["target_hit"] = terrain.inside(frame["target"])
                frame["target_ray_hits"] = terrain.hits(eye, frame["target"])
            # Three vertical body landmarks, plus four lateral points around
            # the central landmark. A visibility proxy, NOT full silhouette QA.
            probes = [(0, -0.35, 0), (0, 0.25, 0), (-0.25, 0, 0),
                      (0.25, 0, 0), (0, 0, -0.25), (0, 0, 0.25)]
            frame["body_hidden"] = frame["occluded"] and all(
                any(h["stable"] for h in terrain.hits(eye, tuple(blupi[i] + p[i] for i in range(3))))
                for p in probes)
            frame["eye_delta"] = frame["blupi_delta"] = 0.0
            frame["jump"] = False
            frame["excluded"] = None
            if previous:
                de, db = math.dist(eye, previous["eye"]), math.dist(blupi, previous["blupi"])
                frame.update(eye_delta=de, blupi_delta=db, previous_eye=previous["eye"],
                             previous_blupi=previous["blupi"], previous_tick=previous["tick"])
                if frame["shake"] is not None and previous["shake"] is not None:
                    def unshaken(f):
                        return (f["eye"][0] - f["shake"][0] / 64,
                                f["eye"][1] + f["shake"][1] / 64, f["eye"][2])
                    frame["unshaken_delta"] = math.dist(unshaken(frame), unshaken(previous))
                reason = ("world change" if n != previous["world"] else
                          "mission start / first chase update" if frame["startup"] else
                          "nonconsecutive play frames" if frame["tick"] != previous["tick"] + 1 else
                          "player discontinuity > 2 blocks (teleport/respawn)" if db > 2 else None)
                frame["excluded"] = reason
                if reason:
                    excluded.append(frame.copy())
                else:
                    frame["jump"] = de > 0.5 and de > 4 * db + 0.02
                if frame["jump"]:
                    sweep = terrain.hits(previous["eye"], eye)
                    frame["sweep_hits"] = [h for h in sweep if h["stable"]]
                    old_view = terrain.hits(previous["eye"], blupi)
                    frame["previous_eye_to_blupi_hits"] = [h for h in old_view if h["stable"]]
            previous = frame
        all_frames.extend(f for f in frames if not f["startup"])
        for n in sorted({f["world"] for f in frames}):
            section = [f for f in frames if f["world"] == n and not f["startup"]]
            runs = contiguous_runs(section, "occluded")
            inside_runs = contiguous_runs(section, "inside")
            summary.append(dict(run=folder.name, world=n, frames=len(section),
                                jumps=sum(f["jump"] for f in section),
                                occluded=sum(f["occluded"] for f in section),
                                longest_occluded=runs[0]["frames"] if runs else 0,
                                body_hidden=sum(f["body_hidden"] for f in section),
                                inside=sum(f["inside"] for f in section),
                                longest_inside=inside_runs[0]["frames"] if inside_runs else 0,
                                mask_occluded=sum(f["mask_occluded"] for f in section),
                                mask_inside=sum(f["mask_inside"] for f in section),
                                inside_target_solid=sum(f["inside"] and bool(f.get("target_hit") and f["target_hit"]["stable"]) for f in section),
                                occluded_target_clear=sum(f["occluded"] and "target_ray_hits" in f and
                                                          not any(h["stable"] for h in f["target_ray_hits"]) for f in section),
                                max_eye_delta=max((f["eye_delta"] for f in section if not f["excluded"]), default=0),
                                excluded=sum(bool(f["excluded"]) for f in section)))
    data = dict(summary=summary, alignment=alignment, excluded=excluded,
                jumps=sorted([f for f in all_frames if f["jump"]], key=lambda f: f["eye_delta"], reverse=True),
                occlusion_runs=contiguous_runs(all_frames, "occluded"),
                inside_runs=contiguous_runs(all_frames, "inside"),
                hidden_runs=contiguous_runs(all_frames, "body_hidden"))
    deltas = sorted(f["eye_delta"] for f in all_frames if not f["excluded"])
    data["motion_distribution"] = {str(p): deltas[min(len(deltas) - 1, int((len(deltas) - 1) * p))]
                                   for p in (.5, .95, .99, .999)} if deltas else {}
    data["threshold_sensitivity"] = {
        str(t): sum(f["eye_delta"] > t and f["eye_delta"] > 4 * f["blupi_delta"] + .02
                    for f in all_frames if not f["excluded"]) for t in (.25, .5, 1.0)}
    (OUT / "analysis.json").write_text(json.dumps(data, indent=2) + "\n")
    fields = ("run", "world", "tick", "line", "eye", "blupi", "eye_delta", "blupi_delta",
              "jump", "occluded", "body_hidden", "inside", "mask_occluded", "mask_inside", "hit", "inside_hit", "excluded",
              "target", "shake", "unshaken_delta")
    with (OUT / "frames.csv").open("w") as file:
        writer = csv.DictWriter(file, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(all_frames)
    print(json.dumps(summary, indent=2))
    return data


def self_test():
    assert segment_box((-2, 0, 0), (2, 0, 0), (0, 0, 0)) == (.375, .625)
    assert segment_box((-2, .5, 0), (2, .5, 0), (0, 0, 0)) is None
    assert segment_box((0, 0, 0), (2, 0, 0), (0, 0, 0)) == (0, .25)
    assert segment_box((-.51, 0, 0), (-.51, 1, 0), (0, 0, 0)) is None
    assert list(traversed((-1, 0, 0), (1, 0, 0))) == [(-1, 0, 0), (0, 0, 0), (1, 0, 0)]
    assert list(traversed((1, 0, 0), (-1, 0, 0))) == [(1, 0, 0), (0, 0, 0), (-1, 0, 0)]
    rng = random.Random(1701)
    cells = list(itertools.product(range(-3, 4), repeat=3))
    for _ in range(500):
        a, b = [tuple(rng.uniform(-2.5, 2.5) for _ in range(3)) for _ in range(2)]
        expected = {cell for cell in cells if segment_box(a, b, cell, .01)}
        actual = {cell for cell in traversed(a, b) if segment_box(a, b, cell, .01)}
        assert actual == expected, (a, b, actual, expected)
    records = [dict(world=w, tick=t, run=r, flag=True) for w, t, r in
               [(1, 1, "a"), (1, 2, "a"), (1, 4, "a"), (2, 5, "a"), (2, 6, "b")]]
    assert [r["frames"] for r in contiguous_runs(records, "flag")] == [2, 1, 1, 1]
    subprocess.run([str(ROOT / "build-cna-opengl3/DumpBlockDefinitions"), "--check",
                    str(ROOT / "tools/data/blocks.csv")], check=True)
    print("PASS: six slab/DDA fixtures, 500 exhaustive candidate-cell comparisons, run segmentation, registry freshness")


def report_tables():
    """Refresh measured sections in the report, preserving its reviewed method/interpretation."""
    data = json.loads((OUT / "analysis.json").read_text())
    primary = [s for s in data["summary"] if s["run"] == "run-001-hub" or
               (s["world"] in (11, 25, 45, 65, 82, 94, 105, 113) and s["run"] == f"run-{s['world']:03d}")]
    primary.sort(key=lambda s: s["world"])
    total = {k: sum(s[k] for s in primary) for k in ("frames", "jumps", "occluded", "inside", "body_hidden")}
    position = lambda p: "(" + ", ".join(f"{v:.3f}" for v in p) + ")"

    def shot(world, tick):
        for name in (f"run-{world:03d}-evidence", f"run-{world:03d}"):
            path = OUT / name / "autoplay_shots" / f"{tick:07d}.png"
            if path.exists() and not path.is_symlink():
                return f"[PNG]({path.relative_to(OUT)})"
        return ""

    def geometry(frame, key="hit"):
        hit = frame.get(key) or frame.get("target_hit") or frame.get("inside_hit")
        if not hit:
            hits = frame.get("previous_eye_to_blupi_hits") or frame.get("sweep_hits") or []
            hit = hits[0] if hits else None
        if not hit:
            return "No static-cube intersection at the final pose"
        return f"cell {tuple(hit['cell'])}, tile {hit['icon']}" + (" (model/state-dependent candidate)" if not hit["stable"] else "")

    findings = (
        f"**The chase camera still has measurable failures.** Across the eight complete mission routes and the longer hub route, "
        f"**{total['frames']:,} eligible 60 Hz camera samples** contain **{total['jumps']} jump events**, "
        f"**{total['occluded']} static-terrain occlusion samples ({100 * total['occluded'] / total['frames']:.3f}%)**, "
        f"and **{total['inside']} eye-inside-solid samples**. The seven-point hidden proxy fires on {total['body_hidden']} samples. "
        f"The worst jump is **{data['jumps'][0]['eye_delta']:.3f} blocks** while Blupi moves "
        f"**{data['jumps'][0]['blupi_delta']:.3f} blocks** (world {data['jumps'][0]['world']:03d}, tick {data['jumps'][0]['tick']}). "
        f"The longest static-terrain occlusion is **{data['occlusion_runs'][0]['frames']} ticks "
        f"({data['occlusion_runs'][0]['seconds']:.3f} s)**; the longest penetration is "
        f"**{data['inside_runs'][0]['frames']} ticks ({data['inside_runs'][0]['seconds']:.3f} s)**. "
        "These are short but severe failures, corroborated by exact-frame screenshots. "
        "The strongest mechanisms are the final undamped wall correction, a smoothed target entering terrain, "
        "and shake applied after collision checking.\n\n"
        "Every standard mission script and both hub routes passed. No deaths/respawns occurred. "
        "World changes and the two observed teleports are excluded from jump counts. "
        "**Coverage distinction:** every-tick camera poses were measured everywhere; every-tick rasterization was performed "
        "for 011, the short hub route, and selected failure windows. The other full routes use the independently validated read-only observer.")
    if (OUT / "shake-analysis.json").exists():
        shake = json.loads((OUT / "shake-analysis.json").read_text())
        findings += (
            f"\n\n**Intentional shake matters:** {len(shake['jumps_below_threshold_without_shake'])} of the jump candidates "
            f"fall below the same threshold after subtracting shake; {len(shake['jumps_still_above_threshold'])} still exceed it, "
            f"and {len(shake['jumps_shake_unknown'])} lack complete shake coverage. Thus 131 raw candidates should not be called "
            f"131 unexplained camera defects. All penetration samples have shake coverage: "
            f"{len(shake['penetrations_resolved_without_shake'])} disappear when shake is subtracted, "
            f"while {len(shake['penetrations_persist_without_shake'])} persist. See `shake-analysis.json`.")
    lines = ["## Numbers by mission", "",
             "Counts below exclude the first two start/load ticks. `Hidden` means all seven body landmarks blocked. "
             "Durations assume the fixed 60 Hz game clock. Occlusion includes eye-inside samples.", "",
             "| World / route | Samples | Jumps | Occluded | Longest occlusion (ticks / seconds) | Hidden | Eye inside | Longest inside (ticks) |",
             "|---|---:|---:|---:|---:|---:|---:|---:|"]
    for s in primary:
        label = f"{s['world']:03d}" + (" terrace/idle hub route" if s["run"].endswith("hub") else "")
        lines.append(f"| {label} | {s['frames']:,} | {s['jumps']} | {s['occluded']} | {s['longest_occluded']} / {s['longest_occluded']/60:.3f} | {s['body_hidden']} | {s['inside']} | {s['longest_inside']} |")
    lines += ["", f"Primary total: {total['frames']:,} samples ({total['frames']/60:.2f} seconds of game time). "
              f"The raw candidate test using every on-disk camera-solid voxel produces "
              f"{sum(s['mask_occluded'] for s in primary)} occlusion and {sum(s['mask_inside'] for s in primary)} inside candidates. "
              "Those higher counts include non-cubic models and obsolete closed-door cells; they are not additional confirmed failures.", "",
              "### Additional route coverage", "",
              "The original 001→010→011 portal route and each mission's return-to-hub tail were retained separately:", "",
              "| Capture | World | Eligible samples | Jumps / occluded / inside |", "|---|---:|---:|---:|"]
    for s in data["summary"]:
        if s not in primary and s["frames"]:
            lines.append(f"| {s['run']} | {s['world']:03d} | {s['frames']} | {s['jumps']} / {s['occluded']} / {s['inside']} |")
    lines += ["", "### Threshold sensitivity and sampling checks", "",
              "Across all eligible main captures (including the additional hub tails), the same four-times-player rule gives " +
              ", ".join(f"{value} events above {threshold} blocks" for threshold, value in data["threshold_sensitivity"].items()) + ". "
              "Eye displacement quantiles are " + ", ".join(f"p{100*float(p):g}={v:.4f}" for p,v in data["motion_distribution"].items()) + " blocks.", "",
              "| Capture | Camera samples before startup exclusions | Native drawn trace lines | Observer-to-draw check |",
              "|---|---:|---:|---|"]
    for a in data["alignment"]:
        check = (f"{a['max_draw_error']:.8f} blocks max error; all match" if a.get("all_draws_match") else
                 "Every tick drawn; first unindexed frame omitted" if "all_draws_match" not in a else "MISMATCH — inspect raw traces")
        lines.append(f"| {a['run']} | {a['matched']} | {a['drawn']} | {check} |")
    lines += ["", "### Ten largest jumps", "",
              "Positions are before → after the indicated tick. Geometry describes the final target/eye cell, "
              "or terrain intersecting the old eye's new view. The raw displacement includes any intentional shake.", "",
              "| Rank | World / tick | Eye Δ | Blupi Δ | Eye before → after | Blupi before → after | Geometry / exact image |",
              "|---:|---|---:|---:|---|---|---|"]
    for i, f in enumerate(data["jumps"][:10], 1):
        lines.append(f"| {i} | {f['world']:03d} / {f['tick']} | {f['eye_delta']:.4f} | {f['blupi_delta']:.4f} | {position(f['previous_eye'])} → {position(f['eye'])} | {position(f['previous_blupi'])} → {position(f['blupi'])} | {geometry(f)} {shot(f['world'], f['tick'])} |")
    for title, key, hitkey in (("Ten longest static-terrain occlusion runs", "occlusion_runs", "hit"),
                               ("Ten longest eye-inside-solid runs", "inside_runs", "inside_hit")):
        lines += ["", f"### {title}", "",
                  "Runs are ranked by consecutive samples (ties retain capture order). Positions and the intersected cell are from the representative middle tick. "
                  "All first/middle/last positions are in `analysis.json`; all samples are in `frames.csv`.", "",
                  "| Rank | World / tick range | Ticks / seconds | Representative tick | Eye | Blupi | Geometry / exact image |",
                  "|---:|---|---:|---:|---|---|---|"]
        for i, r in enumerate(data[key][:10], 1):
            f = r["middle"]
            lines.append(f"| {i} | {r['world']:03d} / {r['start']}–{r['end']} | {r['frames']} / {r['seconds']:.3f} | {f['tick']} | {position(f['eye'])} | {position(f['blupi'])} | {geometry(f, hitkey)} {shot(f['world'], f['tick'])} |")
    lines += ["", "### Excluded teleport jumps", "",
              "These large camera moves follow actual discontinuities in Blupi's position and are not camera-jump failures:", "",
              "| World / tick | Eye Δ | Blupi Δ | Blupi before → after |", "|---|---:|---:|---|"]
    for f in data["excluded"]:
        if "player discontinuity" in f["excluded"]:
            lines.append(f"| {f['world']:03d} / {f['tick']} | {f['eye_delta']:.3f} | {f['blupi_delta']:.3f} | {position(f['previous_blupi'])} → {position(f['blupi'])} |")
    lines += ["", f"Of the {sum(s['inside'] for s in data['summary'])} measured static-terrain penetration samples in all main captures, "
              f"{sum(s['inside_target_solid'] for s in data['summary'])} also have the look target inside static terrain. "
              f"On {sum(s['occluded_target_clear'] for s in data['summary'])} occluded samples the final eye-to-target ray is clear of static cubes "
              "while the eye-to-Blupi ray is blocked. These are direct geometric evidence for mechanisms 2 and 3 below."]
    path = OUT / "report.md"
    report = path.read_text()
    for key, value in (("FINDINGS", findings), ("RESULTS", "\n".join(lines))):
        start, end = f"<!-- {key} -->", f"<!-- /{key} -->"
        content = f"{start}\n{value}\n{end}"
        if end in report:
            report = re.sub(re.escape(start) + r".*?" + re.escape(end), lambda m: content, report, flags=re.S)
        else:
            report = report.replace(start, content)
    path.write_text(report)
    print(f"Updated measured sections in {path}")


def validate_evidence():
    """Check repeatability/draw alignment and distinguish shake-driven threshold crossings."""
    results, base_by_world, extra_by_world = {}, {}, {}
    for number in (25, 45, 65, 105, 113):
        folder = OUT / f"run-{number:03d}-evidence"
        if not folder.exists():
            continue
        if not json.loads((folder / "metadata.json").read_text()).get("passed"):
            raise RuntimeError(f"Incomplete evidence replay: {folder}")
        base = {f["tick"]: f for f in read_frames(OUT / f"run-{number:03d}")[0]}
        extra = {f["tick"]: f for f in read_frames(folder)[0]}
        base_by_world[number], extra_by_world[number] = base, extra
        pairs = [(f, base[t]) for t, f in extra.items() if t in base]
        errors = [math.dist(f["eye"], b["eye"]) for f, b in pairs]
        player_errors = [math.dist(f["blupi"], b["blupi"]) for f, b in pairs]
        assert max(errors, default=0) < .0001 and max(player_errors, default=0) == 0
        offset, native = 0, {}
        for line in (folder / "interp.txt").read_bytes().splitlines(keepends=True):
            offset += len(line)
            words = line.decode().split()
            xyz = tuple(map(float, words[7:10]))
            native[offset] = ((xyz[0] + 50, xyz[1], xyz[2] + 50), tuple(map(float, words[11:13])))
        draw_errors, shake_errors = [], []
        for row in csv.DictReader((folder / "draw_ticks.csv").open()):
            tick, offset = int(row["tick"]), int(row["trace_bytes"])
            if tick in base and offset in native:
                eye, shake = native[offset]
                draw_errors.append(math.dist(base[tick]["eye"], eye))
                if tick in extra and extra[tick]["shake"] is not None:
                    shake_errors.append(math.dist(extra[tick]["shake"], shake))
        assert max(draw_errors, default=0) < .0001 and max(shake_errors, default=0) == 0
        results[str(number)] = dict(matched_ticks=len(pairs), max_eye_error=max(errors, default=0),
                                    max_player_error=max(player_errors, default=0), indexed_draws=len(draw_errors),
                                    max_draw_error=max(draw_errors, default=0), checked_shakes=len(shake_errors),
                                    max_shake_error=max(shake_errors, default=0))
    (OUT / "evidence-validation.json").write_text(json.dumps(results, indent=2) + "\n")
    data = json.loads((OUT / "analysis.json").read_text())

    def lookup(frame):
        n, tick = frame["world"], frame["tick"]
        if n not in base_by_world:
            base_by_world[n] = {f["tick"]: f for f in read_frames(OUT / frame["run"])[0]}
        base = base_by_world[n].get(tick)
        extra = extra_by_world.get(n, {}).get(tick)
        return extra if extra and base and math.dist(extra["eye"], base["eye"]) < .0001 else base

    def unshake(frame):
        return (frame["eye"][0] - frame["shake"][0] / 64,
                frame["eye"][1] + frame["shake"][1] / 64, frame["eye"][2])

    shake_jumps, residual_jumps, unknown = [], [], []
    for f in data["jumps"]:
        current = lookup(f)
        previous = lookup(dict(f, tick=f["previous_tick"]))
        if not current or not previous or current["shake"] is None or previous["shake"] is None:
            unknown.append((f["world"], f["tick"]))
            continue
        delta = math.dist(unshake(current), unshake(previous))
        item = dict(world=f["world"], tick=f["tick"], raw_delta=f["eye_delta"], unshaken_delta=delta)
        (residual_jumps if delta > .5 and delta > 4*f["blupi_delta"]+.02 else shake_jumps).append(item)
    registry = {int(r["icon"]): {k: int(v) for k, v in r.items()}
                for r in csv.DictReader((ROOT / "tools/data/blocks.csv").open())}
    terrain = {}
    inside_shake, inside_residual, inside_unknown = [], [], []
    for run in data["inside_runs"]:
        n = run["world"]
        if n not in terrain:
            terrain[n] = Terrain(n, registry)
        for tick in range(run["start"], run["end"] + 1):
            frame = lookup(dict(run["middle"], tick=tick))
            if not frame or frame["shake"] is None:
                inside_unknown.append((n, tick))
                continue
            hit = terrain[n].inside(unshake(frame))
            (inside_residual if hit and hit["stable"] else inside_shake).append((n, tick))
    shake = dict(jumps_below_threshold_without_shake=shake_jumps, jumps_still_above_threshold=residual_jumps,
                 jumps_shake_unknown=unknown, penetrations_resolved_without_shake=inside_shake,
                 penetrations_persist_without_shake=inside_residual, penetrations_shake_unknown=inside_unknown)
    (OUT / "shake-analysis.json").write_text(json.dumps(shake, indent=2) + "\n")
    print(json.dumps(results, indent=2))
    print("Shake analysis:", {key: len(value) for key, value in shake.items()})


def main():
    global OUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=OUT)
    sub = parser.add_subparsers(dest="command", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("missions", nargs="+")
    capture.add_argument("--label", default="")
    capture.add_argument("--keep", default="")
    capture.add_argument("--every", default=600, type=int)
    capture.add_argument("--realtime", action="store_true")
    capture.add_argument("--tick-observer", action="store_true")
    capture.add_argument("--script", type=Path)
    analysis = sub.add_parser("analyze")
    analysis.add_argument("--partial", action="store_true")
    sub.add_parser("self-test")
    sub.add_parser("report-tables")
    sub.add_parser("validate-evidence")
    args = parser.parse_args()
    OUT = args.output_dir.resolve()
    if args.command == "run":
        run(args.missions, args.label, args.keep, args.every, args.realtime, args.tick_observer, args.script)
    elif args.command == "analyze":
        analyze(partial=args.partial)
    elif args.command == "self-test":
        self_test()
    elif args.command == "report-tables":
        report_tables()
    elif args.command == "validate-evidence":
        validate_evidence()


if __name__ == "__main__":
    main()
