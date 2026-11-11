#!/usr/bin/env python3
"""World-authoring kit: build Galaxy Eggbert `.vwr` worlds from Python, deterministically.

Writes the native format exactly as `src/GalaxyEggbert/Worlds/World.cpp` and `Chunk.cpp` do
(`World Format.md`: header v2, chunk payload `VCH1` v2, palette + bit-packed indices, sparse
`BMD1` records for MoveObjects (type 1), plate rotations (type 2), BigDecor (type 3) and hint zones (type 4)), so the game, the editor and
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
PLATE_ROTATION_TYPE = 2
BIG_DECOR_TYPE = 3
HINT_ZONE_TYPE = 4
PLAYER_START_TYPE = 7          # PlayerStart.hpp: the orange/blue/green starts' cell (a colour mask)
PLAYER_START_HEADING_TYPE = 8  # each colour's two-bit heading at its start's cell


# The original's creatures (../mobile-eggbert/worlds, all 1,000-odd records; BD-0016, the owner's
# OR-0005): every patrol of a type takes the same 20 Hz ticks per cell along its longer axis, there
# and back, and rests the same ticks at each end -- its editor's per-type values. Galaxy's
# builders had set them freely (moving mines up to 14 cells/s against the original's 2). type: (ticks a
# cell, rest ticks). Type 54's file rest (200) is overridden to 152 at load (Decor::Read, BD-0013).
# `tools/audit/patrol_envelope.py` checks the shipped worlds against the original.
CREATURE_TIMING = {
    4: (30, 20),    # bulldozer: 0.67 cells/s
    16: (10, 0),    # moving mine: 2 cells/s, no rest
    17: (15, 47),   # fish: 1.33 cells/s
    20: (8, 8),     # bird: 2.5 cells/s
    32: (8, 25),    # robot (blupih, drops shots): 2.5 cells/s
    33: (10, 23),   # creature (blupit, fires sideways): 2 cells/s
    44: (6, 4),     # wasp: 3.33 cells/s
    54: (20, 200),  # the big creature: 1 cell/s
}


# Each mission's background music in the original (../mobile-eggbert/worlds/worldNNN.txt's
# header `music=`, Speedy Eggbert's level data: 0 none, N the track SOUND/MUSIC<N-1>.BLP;
# OR-0014). A world built for a mission takes its original's unless the builder says otherwise.
ORIGINAL_MUSIC = {
    1: 0, 10: 10, 11: 10, 12: 1, 13: 1, 14: 1, 20: 0, 21: 1, 22: 1, 23: 2, 24: 6, 25: 8,
    30: 0, 31: 1, 32: 10, 33: 4, 34: 2, 40: 0, 41: 3, 42: 1, 43: 2, 44: 8, 45: 6, 46: 9,
    50: 0, 51: 7, 52: 2, 53: 3, 54: 4, 55: 6, 56: 5, 57: 1, 58: 9, 60: 0, 61: 1, 62: 2, 63: 3,
    64: 4, 65: 10, 66: 9, 70: 0, 71: 1, 72: 5, 73: 2, 74: 3, 75: 4, 80: 0, 81: 1, 82: 2, 83: 3,
    84: 4, 90: 0, 91: 6, 92: 7, 93: 8, 94: 9, 95: 10, 100: 0, 101: 1, 102: 10, 103: 2, 104: 3,
    105: 4, 106: 9, 107: 8, 110: 0, 111: 5, 112: 6, 113: 7, 114: 8, 115: 5, 120: 0, 121: 10,
    122: 10, 123: 8, 124: 8, 125: 7, 199: 1,
}


class ObjectType:
    """`include/GalaxyEggbert/Def/ObjectType.hpp` ids used when authoring worlds."""
    LIFT = 1
    MINE = 2
    HANGING_MINE = 3      # hangs from above; a crouching Blupi passes under it (Decor.cpp MoveObjectDetect)
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
    OVERCRAFT = 46        # the hover vehicle (03-objects.md's DOC-300 correction)
    BALLOON = 46          # the old, wrong name of OVERCRAFT, kept for existing scripts
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


# Lift tile palettes from ../mobile-eggbert/worlds/worldNNN.txt, MoveObject type=1 icon.
# Small fidelity data table (standing owner approval, 2026-10-25). Only looks are reused;
# paths, counts and geometry remain the hand-authored 3D worlds. Cycle each mission's palette
# in authoring order; if its 2D mission had no lift, use its world's confirmed palette.
ORIGINAL_LIFT_ICONS = {
    11: (29,),
    21: (387,),
    22: (261, 387),
    23: (387, 29),
    24: (387,),
    25: (387,),
    31: (190,),
    34: (216, 10, 29, 262),
    42: (340,),
    44: (340,),
    45: (340,),
    46: (340, 29, 17),
    50: (84,),
    51: (13,),
    52: (80,),
    53: (78, 81, 79),
    54: (84,),
    55: (89,),
    56: (80, 89, 88),
    57: (78, 87, 89, 88, 90),
    58: (89,),
    61: (361, 29),
    62: (361,),
    63: (78, 361, 79, 80, 83, 84),
    64: (361, 29),
    65: (29, 361),
    66: (361,),
    71: (215, 230, 218),
    73: (218, 229, 215),
    74: (215, 227),
    75: (218, 217),
    82: (29,),
    83: (29, 17, 24),
    84: (29, 257),
    91: (16,),
    93: (16,),
    95: (338,),
    102: (153, 29),
    103: (29,),
    104: (153, 46),
    105: (153, 75, 74),
    106: (29, 153, 75, 74),
    107: (29, 75, 153, 74),
    111: (29, 10),
    112: (23, 15, 18),
    113: (29, 23, 11),
    114: (1, 10, 3, 29),
    122: (284, 261, 338),
    123: (261,),
    124: (261,),
    125: (261,),
    199: (361, 231, 397, 262, 157, 90, 87),
}


class World:
    def __init__(self, mission: int = 0, sky: int = 0, music: int | None = None,
                 legacy_saw_floors: bool = False):
        self.mission = mission
        self.legacy_saw_floors = legacy_saw_floors
        self._lift_style_index = 0
        self.sky = sky
        # The background music (OR-0014): 0 none, 1-10 the original tracks music000-009;
        # stored in the sky-region word's second byte (World Format.md). By default the
        # original mission's (ORIGINAL_MUSIC).
        self.music = ORIGINAL_MUSIC.get(mission, 0) if music is None else music
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

    # ---- player starts (PlayerStart.hpp) -----------------------------------------------------
    def start(self, color: int, x: int, y: int, z: int, turns: int = 0):
        """A player's start: colour 0 yellow (the header's spawn, the one a game alone uses),
        1 orange, 2 blue, 3 green (a network game's teams); `turns` quarter turns (0 faces -Z)."""
        if not 0 <= color <= 3 or not 0 <= turns <= 3:
            raise ValueError("start colour 0-3, turns 0-3")
        self._check(x, y, z)
        if color == 0:
            self.spawn = (x, y, z)
        else:
            self._or_byte(x, y, z, PLAYER_START_TYPE, 1 << color)
        if turns:
            self._or_byte(x, y, z, PLAYER_START_HEADING_TYPE, turns << (2 * color))

    def _or_byte(self, x, y, z, mtype: int, bits: int):
        cell = self.records.setdefault((x, y, z), [])
        for i, (t, payload) in enumerate(cell):
            if t == mtype:
                cell[i] = (t, bytes([payload[0] | bits]))
                return
        cell.append((mtype, bytes([bits])))

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
        timing fields are 20 Hz ticks like the reference's. Anchored at floor(start).

        A patrolling creature (CREATURE_TIMING) always gets its type's own timing, whatever is
        passed: the original's creatures of a type all move at one speed and rest alike."""
        ex, ey, ez = end if end is not None else (x, y, z)
        timing = CREATURE_TIMING.get(otype)
        if timing is not None and end is not None:
            ticks_per_cell, rest = timing
            cells = max(abs(ex - x), abs(ey - y), abs(ez - z))
            advance = recede = cells * ticks_per_cell
            stop_start = stop_end = rest
        if otype == ObjectType.LIFT and icon == 0:
            palette = ORIGINAL_LIFT_ICONS.get(self.mission)
            if not palette and 10 <= self.mission < 130:
                palette = tuple(dict.fromkeys(icon for n, icons in ORIGINAL_LIFT_ICONS.items()
                                               if n // 10 == self.mission // 10 for icon in icons))
            if palette:
                icon = palette[self._lift_style_index % len(palette)]
                self._lift_style_index += 1
        payload = struct.pack("<B11fH", otype, x, y, z, ex, ey, ez, speed, advance, recede, stop_start,
                              stop_end, icon)
        self._record(int(x // 1), int(y // 1), int(z // 1), MOVE_OBJECT_TYPE, payload)

    def rotate_plate(self, x: int, y: int, z: int):
        """Turn the flat plate at (x, y, z) -- a hub sign, a saw -- 90 degrees, to face along X
        instead of Z (PlateRotationMetadata.hpp, metadata type 2)."""
        self._record(x, y, z, PLATE_ROTATION_TYPE, bytes([1]))

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
        header = b"VWR1" + struct.pack("<BBBBIIIIIIII", 2, CHUNK, PER_AXIS,
                                       0 if self.legacy_saw_floors else 1, count, HEADER_SIZE,
                                       data_offset, (self.sky & 0xFF) | ((self.music & 0xFF) << 8),
                                       self.mission, sx, sy, sz)
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
    "D": ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."],
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
