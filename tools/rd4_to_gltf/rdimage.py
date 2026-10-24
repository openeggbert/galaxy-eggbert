"""Decoder for the tiled bitmaps embedded in Ray Dream scenes (texture maps, shading-domain masks).

A bitmap chunk carries ``w``, ``h``, ``dpth`` (1, 8 or 32 bits), an optional ``CTAB`` palette
continued by ``MORE`` lines (256 x ``00RRGGBB``), ``tils W,H`` and a ``TDTA`` chunk with one
``TILE $...$`` line per tile in row-major order. Each tile is ``FFFFFFFF`` followed by one PackBits
stream that runs across row boundaries. Older scenes (t.rd4) write the same data in lowercase
``ctab``/``tdta``/``tile`` leaves as raw bytes after a ``$`` and 8 hex digits of length, with no
``FFFFFFFF`` tile header. Tiles on the right and bottom edges are cropped to the
image. Rows are ``ceil(width * dpth / 8)`` bytes, except 32-bit tiles: three planes for the whole
tile, every red byte, then green, then blue (no alpha); only that reading gives the lollipop's
stripe textures (blupi-sucette.rd4) clean, seamless stripes.
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


def _binary(value: str) -> bytes:
    """Raw bytes of an older-format binary leaf ("$" + 8 hex digits of length + the bytes)."""
    return value[9:].encode("latin-1")


def palette(chunk: Chunk) -> np.ndarray | None:
    text = chunk.leaf("CTAB")
    if text is None:
        old = chunk.leaf("ctab")
        if old is None:
            return None
        raw = _binary(old)
        return np.frombuffer(raw, dtype=np.uint8).reshape(-1, 4)[:, 1:4].copy()
    text += "".join(chunk.leaves("MORE"))
    raw = bytes.fromhex(text)
    entries = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 4)
    return entries[:, 1:4].copy()


def decode(chunk: Chunk) -> np.ndarray:
    """Return (h, w) palette indices (8-bit), 0/1 values (1-bit) or (h, w, 3) RGB (32-bit)."""
    w, h = int(chunk.leaf("w")), int(chunk.leaf("h"))
    depth = int(chunk.leaf("dpth"))
    tw, th = (int(x) for x in chunk.leaf("tils").split(","))
    # Current format: TDTA/TILE hex text with an FFFFFFFF header; older (t.rd4): tdta/tile raw
    # length-prefixed bytes without it.
    old_format = chunk.chunk("TDTA") is None
    tiles = chunk.chunk("tdta").leaves("tile") if old_format else chunk.chunk("TDTA").leaves("TILE")
    cols = -(-w // tw)
    rows = -(-h // th)
    if len(tiles) != cols * rows:
        raise ValueError(f"expected {cols * rows} tiles, found {len(tiles)}")
    img = np.zeros((h, w, 3) if depth == 32 else (h, w), dtype=np.uint8)
    for k, tile in enumerate(tiles):
        r, c = divmod(k, cols)
        x0, y0 = c * tw, r * th
        cw, ch = min(tw, w - x0), min(th, h - y0)
        if old_format:
            raw = b"\xff\xff\xff\xff" + _binary(tile)
        else:
            raw = bytes.fromhex(tile.strip("$"))
        if raw[:4] != b"\xff\xff\xff\xff":
            raise ValueError(f"unexpected tile header {raw[:4].hex()}")
        if depth == 32:
            # Three planes per tile: every red byte of the tile, then green, then blue.
            px = np.frombuffer(unpackbits_stream(raw[4:], cw * ch * 3), dtype=np.uint8)
            img[y0:y0 + ch, x0:x0 + cw] = px.reshape(3, ch, cw).transpose(1, 2, 0)
            continue
        row_bytes = -(-cw * depth // 8)
        px = np.frombuffer(unpackbits_stream(raw[4:], row_bytes * ch), dtype=np.uint8).reshape(ch, row_bytes)
        if depth == 1:
            px = np.unpackbits(px, axis=1)[:, :cw]
        elif depth != 8:
            raise NotImplementedError(f"{depth}-bit bitmaps")
        img[y0:y0 + ch, x0:x0 + cw] = px
    return img
