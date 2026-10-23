"""Decoder for the tiled bitmaps embedded in Ray Dream scenes (texture maps, shading-domain masks).

A bitmap chunk carries ``w``, ``h``, ``dpth`` (1 or 8 bits), an optional ``CTAB`` palette continued
by ``MORE`` lines (256 x ``00RRGGBB``), ``tils W,H`` and a ``TDTA`` chunk with one ``TILE $...$``
line per tile in row-major order. Each tile is ``FFFFFFFF`` followed by one PackBits stream that
runs across row boundaries; rows are ``ceil(W * dpth / 8)`` bytes.
"""

from __future__ import annotations

import numpy as np

from rd4 import Chunk


def unpackbits_stream(data: bytes, size: int) -> bytes:
    out = bytearray()
    i = 0
    while i < len(data) and len(out) < size:
        n = data[i]
        i += 1
        if n < 128:
            out += data[i:i + n + 1]
            i += n + 1
        elif n > 128:
            out += bytes([data[i]]) * (257 - n)
            i += 1
    return bytes(out[:size]).ljust(size, b"\0")


def palette(chunk: Chunk) -> np.ndarray | None:
    text = chunk.leaf("CTAB")
    if text is None:
        return None
    text += "".join(chunk.leaves("MORE"))
    raw = bytes.fromhex(text)
    entries = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 4)
    return entries[:, 1:4].copy()


def decode(chunk: Chunk) -> np.ndarray:
    """Return an (h, w) array of palette indices (8-bit) or 0/1 values (1-bit)."""
    w, h = int(chunk.leaf("w")), int(chunk.leaf("h"))
    depth = int(chunk.leaf("dpth"))
    tw, th = (int(x) for x in chunk.leaf("tils").split(","))
    tiles = chunk.chunk("TDTA").leaves("TILE")
    cols = -(-w // tw)
    rows = -(-h // th)
    if len(tiles) != cols * rows:
        raise ValueError(f"expected {cols * rows} tiles, found {len(tiles)}")
    row_bytes = -(-tw * depth // 8)
    img = np.zeros((rows * th, cols * tw), dtype=np.uint8)
    for k, tile in enumerate(tiles):
        raw = bytes.fromhex(tile.strip("$"))
        if raw[:4] != b"\xff\xff\xff\xff":
            raise ValueError(f"unexpected tile header {raw[:4].hex()}")
        px = np.frombuffer(unpackbits_stream(raw[4:], row_bytes * th), dtype=np.uint8).reshape(th, row_bytes)
        if depth == 1:
            px = np.unpackbits(px, axis=1)[:, :tw]
        elif depth != 8:
            raise NotImplementedError(f"{depth}-bit bitmaps")
        r, c = divmod(k, cols)
        img[r * th:(r + 1) * th, c * tw:(c + 1) * tw] = px
    return img[:h, :w]
