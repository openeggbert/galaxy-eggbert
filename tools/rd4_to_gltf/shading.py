"""Flat base colours from Ray Dream shaders.

A shader (``sh``) lists shading domains in ``elms``; ``sprm`` is the primary domain covering the
whole surface. Its first channel is the colour: ``sclr`` (``colr r,g,b``) or ``smap`` (a texture
map). An instance's own ``sh`` overrides its master's. A primary texture map is baked into one
image together with the ``srct``/``sovl`` domains above it (``texture_map``); masked ``srgn``
domains and the remaining channels (highlight, shininess, bump, ...) are not converted yet.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

import rd4
import rdimage
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


# ---- texture maps --------------------------------------------------------------------

# The only texture-map placement verified against the sprites (Blupi's face in every scene); see
# freeform_mesh.texture_coordinates.
VERIFIED_PLACEMENT = {"rotc": "3", "flpv": "1", "flph": "0", "cnth": "1", "cntv": "1",
                      "tile": "0", "invC": "0"}


@dataclass
class TextureMap:
    image: np.ndarray        # (H, W, 3) uint8 with "white is transparent" already resolved
    domain: tuple            # primary domain bbox (u0, v0, u1, v1)
    quarter_turns: int       # rotc
    warnings: list


def _find_image(scene: sc.Scene, refn: str) -> rd4.Chunk | None:
    tabl = scene.root.chunk("sc").chunk("TABL")
    if tabl is None:
        return None
    for sub in tabl.chunks("SUBT"):
        imap = sub.chunk("IMAP")
        if imap is None:
            continue
        for robj in imap.chunks("ROBJ"):
            iden = robj.chunk("IDEN")
            if iden is not None and iden.leaf("REFN") == refn:
                return robj.chunk("DATA")
    return None


def _border_connected(mask: np.ndarray) -> np.ndarray:
    """The part of ``mask`` 4-connected to the image border."""
    grown = np.zeros_like(mask)
    grown[0], grown[-1], grown[:, 0], grown[:, -1] = mask[0], mask[-1], mask[:, 0], mask[:, -1]
    while True:
        nxt = grown.copy()
        nxt[1:] |= grown[:-1]
        nxt[:-1] |= grown[1:]
        nxt[:, 1:] |= grown[:, :-1]
        nxt[:, :-1] |= grown[:, 1:]
        nxt &= mask
        if (nxt == grown).all():
            return grown
        grown = nxt


def texture_map(scene: sc.Scene, obj: sc.Obj) -> TextureMap | None:
    """The primary domain's texture map of ``obj``'s own shader, baked over its other domains."""
    sh = obj.chunk.chunk("sh") if obj.chunk is not None else None
    if sh is None or sh.chunk("elms") is None:
        return None
    domains = sh.chunk("elms").chunks()
    primary = next((d for d in domains if d.tag == "sprm"), None)
    smap = _color_channel(primary) if primary is not None else None
    if smap is None or smap.tag != "smap":
        return None
    warnings = []
    minf = smap.chunk("MInf")
    placement = {k: minf.leaf(k) for k in VERIFIED_PLACEMENT} if minf is not None else {}
    if placement != VERIFIED_PLACEMENT:
        warnings.append(f"{obj.name}: unverified texture placement {placement}")
    refn = smap.chunk("CMAp").chunk("MDTA").leaf("REFN")
    data = _find_image(scene, refn)
    if data is None:
        warnings.append(f"{obj.name}: texture image REFN {refn} not found")
        return None
    decoded = rdimage.decode(data)
    pal = rdimage.palette(data)
    texels = decoded if decoded.ndim == 3 else pal[decoded]   # 32-bit images carry RGB directly
    rgb = texels.astype(np.float64) / 255.0
    # "whit 1": pure white texels are transparent and show the surface colour underneath, which is
    # the master's primary colour (yellow for Blupi). Near-white (254) eye whites stay white.
    if smap.leaf("whit") == "1":
        master = scene.masters.get(obj.master)
        base = primary_color(master.chunk("sh") if master is not None else None)
        if not isinstance(base, tuple):
            base = (1.0, 1.0, 1.0)
        rgb[(texels == 255).all(axis=-1)] = base
        # Near-white background around a face drawn on a smaller sheet (blupi-ballon.rd4's face
        # sits on a 254-white page that covers his back) is not part of the drawing either: the
        # sprites never show his back, and Blupi is yellow all round. Near-white touching the
        # image border becomes the surface colour; eye whites, enclosed in their outlines, stay.
        near = (texels >= 250).all(axis=-1) & ~(texels == 255).all(axis=-1)
        page = _border_connected(near)
        if page.any():
            rgb[page] = base
            warnings.append(f"{obj.name}: {page.mean():.0%} of the texture is near-white page; "
                            "shown as the surface colour")
    u0, v0, u1, v1 = primary.floats("bbox")
    h, w = texels.shape[:2]
    # Other domains lie on top of the primary one (they precede it in elms).
    for d in domains:
        if d is primary:
            continue
        ch = _color_channel(d)
        if d.tag not in ("srct", "sovl") or ch is None or ch.tag != "sclr":
            warnings.append(f"{obj.name}: shading domain {d.tag} not converted")
            continue
        du0, dv0, du1, dv1 = d.floats("bbox")
        xs = (np.arange(w) + 0.5) / w * (u1 - u0) + u0
        ys = (np.arange(h) + 0.5) / h * (v1 - v0) + v0
        # u wraps around the cross-section, so test the texel's u and u +- one perimeter.
        period = u1 - u0
        best = np.full(w, np.inf)
        for k in (-1, 0, 1):
            x = xs + k * period
            cu = (x - (du0 + du1) / 2) / max((du1 - du0) / 2, 1e-9)
            best = np.minimum(best, np.abs(cu))
        cv = (ys - (dv0 + dv1) / 2) / max((dv1 - dv0) / 2, 1e-9)
        if d.tag == "srct":
            inside = (np.abs(cv)[:, None] <= 1) & (best[None, :] <= 1)
        else:
            inside = cv[:, None] ** 2 + best[None, :] ** 2 <= 1
        rgb[inside] = ch.floats("colr")
    return TextureMap((rgb * 255).round().astype(np.uint8), (u0, v0, u1, v1),
                      int(placement.get("rotc") or 0), warnings)


def textured_material_json(name: str, texture: int) -> dict:
    return {
        "name": f"rd_tex_{name.replace(' ', '_')}",
        "pbrMetallicRoughness": {
            "baseColorTexture": {"index": texture},
            "metallicFactor": 0.0,
            "roughnessFactor": 0.5,
        },
    }
