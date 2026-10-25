#!/usr/bin/env python3
"""World-authoring kit: build Galaxy Eggbert `.vwr` worlds from Python, deterministically.

Writes the native format exactly as `src/GalaxyEggbert/Worlds/World.cpp` and `Chunk.cpp` do
(`World Format.md`: header v2, chunk payload `VCH1` v2, palette + bit-packed indices, sparse
`BMD1` records for MoveObjects (type 1), BigDecor (type 3) and hint zones (type 4)), so the game, the editor and
`tools/vwr_inspect.py` read the result like any other world. Coordinates are the raw grid's
(0-99 on every axis, Y up), the ones `.vwr` files, the editor and `vwr_inspect.py` use.

    from worldkit import World, ObjectType as O
    w = World(mission=11, sky=0)
    w.fill(40, 0, 40, 60, 0, 60, 10)          # a grass floor
    w.box(48, 1, 48, 52, 3, 52, 261, hollow=True)
    w.place(O.TREASURE, 50, 1, 55)
    w.spawn = (45, 1, 45)
    w.save("worlds3d/world011.vwr")

Block ids are object-m.png icons (include/GalaxyEggbert/BlockTypes.hpp); object types are
`Def::ObjectType` ids. Nothing here invents gameplay: it only places what the game already has.
"""

from __future__ import annotations

import struct
from pathlib import Path

SIZE = 100
CHUNK = 10
PER_AXIS = SIZE // CHUNK
HEADER_SIZE = 40
TABLE_ENTRY_SIZE = 20
CHUNK_EMPTY = 0x0001
MOVE_OBJECT_TYPE = 1
BIG_DECOR_TYPE = 3
HINT_ZONE_TYPE = 4


class ObjectType:
    """`include/GalaxyEggbert/Def/ObjectType.hpp` ids used when authoring worlds."""
    LIFT = 1
    MINE = 2
    MINE_PATROL = 3
    BULLDOZER = 4
    TREASURE = 5
    EGG = 6
    EXIT = 7
    CRATE = 12
    HELICOPTER = 13
    MOVING_MINE = 16
    FISH = 17
    JEEP = 19
    BIRD = 20
    SECRET_EXIT = 21
    SKATEBOARD = 24
    SHIELD = 25
    LOLLIPOP = 26
    TANK = 28
    AMMO = 29
    DRINK = 30
    CHARGE = 31
    ROBOT = 32
    CREATURE = 33
    INVERT = 40
    WASP = 44
    BALLOON = 46
    LIFT_RIGHT = 47
    LIFT_LEFT = 48
    KEY1 = 49
    KEY2 = 50
    KEY3 = 51
    BIG_CREATURE = 54
    DYNAMITE = 55
    FOLLOW_STAR = 96
    FOLLOW_STAR_EXACT = 97


def _bits_for(count: int) -> int:
    bits, cap = 1, 2
    while cap < count:
        bits += 1
        cap <<= 1
    return bits


class World:
    def __init__(self, mission: int = 0, sky: int = 0):
        self.mission = mission
        self.sky = sky
        self.spawn: tuple[int, int, int] | None = None
        self.blocks: dict[tuple[int, int, int], int] = {}   # cell -> raw block (type << 4 | meta)
        self.records: dict[tuple[int, int, int], list[tuple[int, bytes]]] = {}

    # ---- blocks ---------------------------------------------------------------------------
    @staticmethod
    def _check(x, y, z):
        if not (0 <= x < SIZE and 0 <= y < SIZE and 0 <= z < SIZE):
            raise ValueError(f"cell ({x}, {y}, {z}) outside the world")

    def set(self, x: int, y: int, z: int, block: int, meta: int = 0):
        self._check(x, y, z)
        if block == 0:
            self.blocks.pop((x, y, z), None)
        else:
            self.blocks[(x, y, z)] = (block << 4) | (meta & 0xF)

    def get(self, x: int, y: int, z: int) -> int:
        return self.blocks.get((x, y, z), 0) >> 4

    def clear(self, x: int, y: int, z: int):
        self.set(x, y, z, 0)

    def fill(self, x0, y0, z0, x1, y1, z1, block: int, meta: int = 0):
        """Every cell of the inclusive box; block 0 carves air."""
        for x in range(min(x0, x1), max(x0, x1) + 1):
            for y in range(min(y0, y1), max(y0, y1) + 1):
                for z in range(min(z0, z1), max(z0, z1) + 1):
                    self.set(x, y, z, block, meta)

    def box(self, x0, y0, z0, x1, y1, z1, block: int, hollow: bool = False, floor: bool = True,
            roof: bool = True):
        """A solid box, or with `hollow` its walls only (and floor/roof if asked)."""
        xa, xb = sorted((x0, x1))
        ya, yb = sorted((y0, y1))
        za, zb = sorted((z0, z1))
        for x in range(xa, xb + 1):
            for y in range(ya, yb + 1):
                for z in range(za, zb + 1):
                    edge = x in (xa, xb) or z in (za, zb)
                    if not hollow or edge or (floor and y == ya) or (roof and y == yb):
                        self.set(x, y, z, block)

    def stairs(self, x, y, z, dx, dz, steps: int, block: int, width: int = 1):
        """`steps` one-block steps rising one cell each, heading (dx, dz), `width` wide."""
        px, pz = -dz, dx  # sideways
        for i in range(steps):
            for w in range(width):
                cx, cz = x + dx * i + px * w, z + dz * i + pz * w
                self.fill(cx, y, cz, cx, y + i, cz, block)

    # ---- objects --------------------------------------------------------------------------
    def _record(self, x, y, z, mtype: int, payload: bytes):
        self._check(x, y, z)
        cell = self.records.setdefault((x, y, z), [])
        if any(t == mtype for t, _ in cell):
            raise ValueError(f"cell ({x}, {y}, {z}) already holds a type-{mtype} record")
        cell.append((mtype, payload))

    def place(self, otype: int, x: float, y: float, z: float, end: tuple[float, float, float] | None = None,
              speed: float = 1.5, advance: float = 60.0, recede: float = 60.0, stop_start: float = 40.0,
              stop_end: float = 40.0, icon: int = 0):
        """A MoveObject (MoveObjectRecord.cpp's 47-byte payload). `end` makes it patrol; the
        timing fields are 20 Hz ticks like the reference's. Anchored at floor(start)."""
        ex, ey, ez = end if end is not None else (x, y, z)
        payload = struct.pack("<B11fH", otype, x, y, z, ex, ey, ez, speed, advance, recede, stop_start,
                              stop_end, icon)
        self._record(int(x // 1), int(y // 1), int(z // 1), MOVE_OBJECT_TYPE, payload)

    def bigdecor(self, icon: int, x: int, y: int, z: int):
        """A BigDecor cell (explo.png icon, BigDecorRecord.hpp)."""
        if icon == 0:
            raise ValueError("BigDecor icon 0 means none")
        self._record(x, y, z, BIG_DECOR_TYPE, struct.pack("<H", icon))

    def hint_zone(self, record: int, x0: int, y0: int, z0: int, x1: int, y1: int, z1: int):
        """Where training hint `record` of the mission's table (TrainingHints.cpp) shows: the
        inclusive box, raw cells (HintZoneRecord.hpp). The text and gate stay the original's."""
        xa, xb = sorted((x0, x1))
        ya, yb = sorted((y0, y1))
        za, zb = sorted((z0, z1))
        size = (xb - xa + 1, yb - ya + 1, zb - za + 1)
        if not 0 <= record < 256 or any(not 0 < v < 256 for v in size):
            raise ValueError("hint zone record or size out of range")
        self._check(xb, yb, zb)
        self._record(xa, ya, za, HINT_ZONE_TYPE, bytes([record, *size]))

    # ---- serialization (World.cpp / Chunk.cpp) --------------------------------------------
    def _chunk_payload(self, cx: int, cy: int, cz: int) -> bytes | None:
        blocks = [0] * (CHUNK ** 3)
        any_block = False
        for lz in range(CHUNK):
            for ly in range(CHUNK):
                for lx in range(CHUNK):
                    raw = self.blocks.get((cx * CHUNK + lx, cy * CHUNK + ly, cz * CHUNK + lz), 0)
                    if raw:
                        blocks[lx + ly * CHUNK + lz * CHUNK * CHUNK] = raw
                        any_block = True
        meta = []
        for (x, y, z), recs in sorted(self.records.items(), key=lambda kv: (kv[0][2], kv[0][1], kv[0][0])):
            if (x // CHUNK, y // CHUNK, z // CHUNK) != (cx, cy, cz):
                continue
            local = (x % CHUNK) + (y % CHUNK) * CHUNK + (z % CHUNK) * CHUNK * CHUNK
            for mtype, payload in recs:
                meta.append((local, mtype, payload))
        if not any_block and not meta:
            return None
        palette = [0] if not any_block or 0 in blocks else []
        for raw in blocks:
            if raw not in palette:
                palette.append(raw)
        if len(palette) > 256:
            raise ValueError(f"chunk ({cx}, {cy}, {cz}) has {len(palette)} block kinds (max 256)")
        bits = _bits_for(len(palette))
        index = {raw: i for i, raw in enumerate(palette)}
        nwords = (len(blocks) * bits + 63) // 64
        words = [0] * nwords
        for i, raw in enumerate(blocks):
            v = index[raw]
            off = i * bits
            w, b = divmod(off, 64)
            words[w] |= (v << b) & 0xFFFFFFFFFFFFFFFF
            if b + bits > 64:
                words[w + 1] |= v >> (64 - b)
        meta_bytes = b""
        if meta:
            meta_bytes = b"BMD1" + struct.pack("<HHI", 1, 0, len(meta))
            for local, mtype, payload in meta:
                meta_bytes += struct.pack("<IHH", local, mtype, len(payload)) + payload
        out = b"VCH1" + struct.pack("<BBBBHHIII", 2, CHUNK, bits, 1 if meta else 0, len(palette), 0,
                                    CHUNK ** 3, nwords * 8, len(meta_bytes))
        out += struct.pack(f"<{len(palette)}H", *palette)
        out += struct.pack(f"<{nwords}Q", *words)
        return out + meta_bytes

    def to_bytes(self) -> bytes:
        count = PER_AXIS ** 3
        data_offset = HEADER_SIZE + count * TABLE_ENTRY_SIZE
        table, blobs, offset = [], [], data_offset
        for cz in range(PER_AXIS):
            for cy in range(PER_AXIS):
                for cx in range(PER_AXIS):
                    blob = self._chunk_payload(cx, cy, cz)
                    if blob is None:
                        table.append(struct.pack("<QIHHHH", 0, 0, cx, cy, cz, CHUNK_EMPTY))
                    else:
                        table.append(struct.pack("<QIHHHH", offset, len(blob), cx, cy, cz, 0))
                        blobs.append(blob)
                        offset += len(blob)
        sx, sy, sz = (c + 1 for c in self.spawn) if self.spawn else (0, 0, 0)
        header = b"VWR1" + struct.pack("<BBBBIIIIIIII", 2, CHUNK, PER_AXIS, 0, count, HEADER_SIZE,
                                       data_offset, self.sky, self.mission, sx, sy, sz)
        return header + b"".join(table) + b"".join(blobs)

    def save(self, path: str | Path):
        if self.spawn is not None:
            self._check(*self.spawn)
        Path(path).write_bytes(self.to_bytes())


# ---- voxel lettering ---------------------------------------------------------------------
# A 5x7 block alphabet (rows top to bottom, "#" = block), for titles built from tiles.
LETTERS = {
    "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "B": ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    "E": ["#####", "#....", "#....", "####.", "#....", "#....", "#####"],
    "G": [".####", "#....", "#....", "#.###", "#...#", "#...#", ".####"],
    "L": ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    "R": ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"],
    "T": ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."],
    "X": ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"],
    "Y": ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
    " ": [".....", ".....", ".....", ".....", ".....", ".....", "....."],
}


def letter_cells(text: str, spacing: int = 1):
    """(column, row-from-top) cells of `text` in the 5x7 alphabet; width in columns."""
    cells, col = [], 0
    for ch in text.upper():
        glyph = LETTERS[ch]
        for r, line in enumerate(glyph):
            for c, mark in enumerate(line):
                if mark == "#":
                    cells.append((col + c, r))
        col += len(glyph[0]) + spacing
    return cells, col - spacing
