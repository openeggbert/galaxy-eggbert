#!/usr/bin/env python3
"""Read-only inspector for Galaxy Eggbert's native `.vwr` worlds (`World Format.md`).

Prints, per world: header fields (version, mission, sky region, spawn), the number of solid
blocks per block type and per height (Y), the bounding box of the solid blocks, and every
MoveObject and BigDecor record. With ``--summary`` one line per world, for inventories:

    python3 tools/vwr_inspect.py worlds3d/world999.vwr
    python3 tools/vwr_inspect.py --summary worlds3d/*.vwr
    python3 tools/vwr_inspect.py --objects 7,21 worlds3d/world999.vwr   # only exits

It never writes. The reader follows `src/GalaxyEggbert/Worlds/World.cpp`, `Chunk.cpp` and
`MoveObjectRecord.cpp`; when those change, change this too.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import struct
import sys
from pathlib import Path

HEADER_SIZE = 40
TABLE_ENTRY_SIZE = 20
CHUNK_EMPTY = 0x0001
MOVE_OBJECT_TYPE = 1      # MoveObjectRecord.hpp, kMoveObjectMetadataType
PLATE_ROTATION_TYPE = 2   # PlateRotationMetadata.hpp
BIG_DECOR_TYPE = 3        # BigDecorRecord.hpp, kBigDecorMetadataType
HINT_ZONE_TYPE = 4        # HintZoneRecord.hpp, kHintZoneMetadataType


def packed_index(words: list[int], i: int, bits: int) -> int:
    off = i * bits
    w, b = divmod(off, 64)
    value = words[w] >> b
    if 64 - b < bits:
        value |= words[w + 1] << (64 - b)
    return value & ((1 << bits) - 1)


def read_chunk(data: bytes):
    """(blocks {local index: raw block}, metadata [(local index, type, payload)])."""
    if data[:4] != b"VCH1":
        raise ValueError("bad chunk magic")
    version, size, bits, flags = data[4], data[5], data[6], data[7]
    palette_count, = struct.unpack_from("<H", data, 8)
    if version == 1:
        block_count, data_size = struct.unpack_from("<II", data, 12)
        meta_size, pos = 0, 20
    elif version == 2:
        block_count, data_size, meta_size = struct.unpack_from("<III", data, 12)
        pos = 24
    else:
        raise ValueError(f"unsupported chunk version {version}")
    palette = list(struct.unpack_from(f"<{palette_count}H", data, pos))
    pos += 2 * palette_count
    words = list(struct.unpack_from(f"<{data_size // 8}Q", data, pos))
    pos += data_size
    blocks = {}
    for i in range(block_count):
        raw = palette[packed_index(words, i, bits)] if palette_count > 1 else palette[0]
        if raw != 0:
            blocks[i] = raw
    meta = []
    if flags & 1 and meta_size:
        sec = data[pos:pos + meta_size]
        if sec[:4] != b"BMD1":
            raise ValueError("bad metadata magic")
        count, = struct.unpack_from("<I", sec, 8)
        p = 12
        for _ in range(count):
            local, mtype, psize = struct.unpack_from("<IHH", sec, p)
            p += 8
            meta.append((local, mtype, sec[p:p + psize]))
            p += psize
    return size, blocks, meta


def decode_move_object(payload: bytes) -> dict:
    if len(payload) not in (45, 47):
        return {"bad_size": len(payload)}
    f = struct.unpack_from("<11f", payload, 1)
    rec = {"type": payload[0], "start": f[0:3], "end": f[3:6], "speed": f[6],
           "advance": f[7], "recede": f[8], "stop_start": f[9], "stop_end": f[10]}
    rec["icon"] = struct.unpack_from("<H", payload, 45)[0] if len(payload) == 47 else 0
    return rec


def read_world(path: Path) -> dict:
    data = path.read_bytes()
    if data[:4] != b"VWR1":
        raise ValueError("bad world magic")
    version, chunk_size, per_axis, flags = data[4], data[5], data[6], data[7]
    chunk_count, table_offset, data_offset, sky, mission, sx, sy, sz = struct.unpack_from("<8I", data, 8)
    world = {"path": path, "version": version, "chunk_size": chunk_size, "per_axis": per_axis,
             "flags": flags, "sky": sky, "mission": mission,
             "spawn": (sx - 1, sy - 1, sz - 1) if sx and sy and sz else None,
             "blocks": {}, "objects": [], "bigdecor": [], "hints": [], "plates": 0, "other_meta": collections.Counter(),
             "sha1": hashlib.sha1(data).hexdigest()[:10], "bytes": len(data)}
    for k in range(chunk_count):
        off, size, cx, cy, cz, cflags = struct.unpack_from("<QIHHHH", data, table_offset + k * TABLE_ENTRY_SIZE)
        if cflags & CHUNK_EMPTY:
            continue
        csize, blocks, meta = read_chunk(data[off:off + size])
        base = (cx * csize, cy * csize, cz * csize)

        def pos(local):
            lx, rest = local % csize, local // csize
            ly, lz = rest % csize, rest // csize
            return (base[0] + lx, base[1] + ly, base[2] + lz)

        for local, raw in blocks.items():
            world["blocks"][pos(local)] = raw
        for local, mtype, payload in meta:
            if mtype == MOVE_OBJECT_TYPE:
                world["objects"].append(decode_move_object(payload))
            elif mtype == BIG_DECOR_TYPE:
                world["bigdecor"].append((pos(local), struct.unpack_from("<H", payload)[0] if len(payload) >= 2 else None))
            elif mtype == HINT_ZONE_TYPE and len(payload) == 4:
                world["hints"].append((payload[0], pos(local), tuple(payload[1:4])))
            elif mtype == PLATE_ROTATION_TYPE:
                world["plates"] += 1
            else:
                world["other_meta"][mtype] += 1
    return world


def summary_line(w: dict) -> str:
    blocks = w["blocks"]
    types = collections.Counter(raw >> 4 for raw in blocks.values())
    ys = collections.Counter(p[1] for p in blocks)
    obj_types = collections.Counter(o.get("type") for o in w["objects"])
    if blocks:
        xs = [p[0] for p in blocks]; zs = [p[2] for p in blocks]
        box = f"x{min(xs)}-{max(xs)} y{min(ys)}-{max(ys)} z{min(zs)}-{max(zs)}"
    else:
        box = "-"
    return (f"{w['path'].name}: mission {w['mission']} sky {w['sky']} spawn {w['spawn']} "
            f"blocks {len(blocks)} ({len(types)} types, {len(ys)} heights, {box}) "
            f"objects {len(w['objects'])} ({len(obj_types)} types) bigdecor {len(w['bigdecor'])} "
            f"sha1 {w['sha1']}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("worlds", nargs="+", type=Path)
    ap.add_argument("--summary", action="store_true", help="one line per world")
    ap.add_argument("--objects", type=lambda v: [int(x) for x in v.split(",")],
                    help="list only MoveObjects of these comma-separated types")
    args = ap.parse_args(argv)
    for path in args.worlds:
        try:
            w = read_world(path)
        except Exception as e:  # noqa: BLE001 - report and go on with the next world
            print(f"{path}: NOT READ ({e})")
            continue
        print(summary_line(w))
        if args.summary:
            continue
        types = collections.Counter(raw >> 4 for raw in w["blocks"].values())
        print("  block types (icon: count):", ", ".join(f"{t}:{n}" for t, n in sorted(types.items())))
        ys = collections.Counter(p[1] for p in w["blocks"])
        print("  blocks per Y:", ", ".join(f"{y}:{n}" for y, n in sorted(ys.items())))
        for o in w["objects"]:
            if args.objects and o.get("type") not in args.objects:
                continue
            if "bad_size" in o:
                print(f"  object with bad payload size {o['bad_size']}")
                continue
            s, e = o["start"], o["end"]
            moving = "" if s == e else f" -> ({e[0]:.2f}, {e[1]:.2f}, {e[2]:.2f}) speed {o['speed']:g}"
            icon = f" icon {o['icon']}" if o["icon"] else ""
            print(f"  object type {o['type']} at ({s[0]:.2f}, {s[1]:.2f}, {s[2]:.2f}){moving}{icon}")
        for p, icon in w["bigdecor"]:
            print(f"  bigdecor icon {icon} at {p}")
        for record, p, size in sorted(w["hints"]):
            print(f"  hint zone record {record} at {p} size {size}")
        if w["plates"]:
            print(f"  plate rotation records: {w['plates']}")
        for mtype, n in w["other_meta"].items():
            print(f"  unknown metadata type {mtype}: {n} record(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
