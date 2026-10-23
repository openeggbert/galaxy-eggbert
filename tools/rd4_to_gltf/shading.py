"""Flat base colours from Ray Dream shaders.

A shader (``sh``) lists shading domains in ``elms``; ``sprm`` is the primary domain covering the
whole surface. Its first channel is the colour: ``sclr`` (``colr r,g,b``) or ``smap`` (a texture
map). An instance's own ``sh`` overrides its master's. Domains other than the primary one, texture
maps and the remaining channels (highlight, shininess, bump, ...) are not converted yet; where the
primary colour is a texture the master's primary colour is used instead.
"""

from __future__ import annotations

import rd4
import scene as sc


def _color_channel(domain: rd4.Chunk) -> rd4.Chunk | None:
    shdr = domain.chunk("shdr")
    if shdr is None:
        return None
    node = shdr
    while True:
        kids = [k for k in node.chunks() if k.tag != "ACom"]  # ACom holds animation tracks
        if not kids:
            return None
        first = kids[0]
        if first.tag in ("scmp", "ssub"):
            node = first
            continue
        return first


def primary_color(sh: rd4.Chunk | None) -> tuple[float, float, float] | str | None:
    if sh is None or sh.chunk("elms") is None:
        return None
    domains = sh.chunk("elms").chunks()
    primary = next((d for d in domains if d.tag == "sprm"), None)
    if primary is None:
        return None
    ch = _color_channel(primary)
    if ch is None:
        return None
    if ch.tag == "sclr" and ch.leaf("colr"):
        r, g, b = ch.floats("colr")
        return (round(r, 3), round(g, 3), round(b, 3))
    if ch.tag == "smap":
        return "texture"
    return None


def material_key(scene: sc.Scene, obj: sc.Obj) -> tuple[float, float, float]:
    inst = primary_color(obj.chunk.chunk("sh") if obj.chunk is not None else None)
    if isinstance(inst, tuple):
        return inst
    master = scene.masters.get(obj.master)
    base = primary_color(master.chunk("sh") if master is not None else None)
    if isinstance(base, tuple):
        return base
    return (1.0, 1.0, 1.0)


def _srgb_to_linear(c: float) -> float:
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def material_json(key: tuple[float, float, float]) -> dict:
    r, g, b = key
    return {
        "name": f"rd_{int(r * 255):02x}{int(g * 255):02x}{int(b * 255):02x}",
        "pbrMetallicRoughness": {
            "baseColorFactor": [_srgb_to_linear(r), _srgb_to_linear(g), _srgb_to_linear(b), 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.5,
        },
    }
