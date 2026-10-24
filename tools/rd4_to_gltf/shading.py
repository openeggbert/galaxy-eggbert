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
    if ch.tag == "smix":
        mixed = _mix_colour(ch)
        if mixed is not None:
            r, g, b = mixed
            return (round(float(r), 3), round(float(g), 3), round(float(b), 3))
    return None


def _mix_colour(mix: rd4.Chunk):
    """Flat colour of a mix shader (``smix``: two colours or mixes blended by a pattern such as
    wood ``swod`` or marble ``smrb``): the mean of its colour inputs, mixes nested in it taken
    the same way, inputs without a colour skipped. The invert sign's post (two golds by a wood
    grain); the palm trunk (Explo/plante*: a dark brown and a mix of empty colours by marble)."""
    sub = mix.chunk("ssub")
    cols = []
    for c in (sub.chunks() if sub is not None else []):
        if c.tag == "sclr" and c.leaf("colr"):
            cols.append(np.array(c.floats("colr")))
        elif c.tag == "smix":
            inner = _mix_colour(c)
            if inner is not None:
                cols.append(np.array(inner))
        if len(cols) == 2:
            break
    return np.mean(cols, axis=0) if cols else None


# Flat colours replaced while converting one object (build_object_library.py's ``recolour``):
# the decoy mines of blupi.png / blupi1.png are bombe.rd4's red mine in yellow and green.
COLOUR_MAP: dict[tuple, tuple] = {}


def material_key(scene: sc.Scene, obj: sc.Obj) -> tuple[float, float, float]:
    inst = primary_color(obj.chunk.chunk("sh") if obj.chunk is not None else None)
    if not isinstance(inst, tuple):
        master = scene.masters.get(obj.master)
        inst = primary_color(master.chunk("sh") if master is not None else None)
    key = inst if isinstance(inst, tuple) else (1.0, 1.0, 1.0)
    return COLOUR_MAP.get(key, key)


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
    counts: tuple = (1, 1)   # cnth, cntv: repeats across and along (tile 1)
    flips: tuple = (False, True)  # flph, flpv


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
    counts = (int(placement.get("cnth") or 1), int(placement.get("cntv") or 1)) \
        if placement.get("tile") == "1" else (1, 1)
    flips = (placement.get("flph") == "1", placement.get("flpv") == "1")
    return TextureMap((rgb * 255).round().astype(np.uint8), (u0, v0, u1, v1),
                      int(placement.get("rotc") or 0), warnings, counts, flips)


def _white_texels(scene: sc.Scene, smap: rd4.Chunk) -> np.ndarray | None:
    """(H, W) mask of a texture map's pure white texels."""
    data = _find_image(scene, smap.chunk("CMAp").chunk("MDTA").leaf("REFN"))
    if data is None:
        return None
    decoded = rdimage.decode(data)
    texels = decoded if decoded.ndim == 3 else rdimage.palette(data)[decoded]
    return (texels == 255).all(axis=-1)


def _texels(scene: sc.Scene, obj: sc.Obj, smap: rd4.Chunk, base: tuple) -> np.ndarray | None:
    """A texture map's image as (H, W, 3) floats with "whit 1" resolved to ``base``."""
    data = _find_image(scene, smap.chunk("CMAp").chunk("MDTA").leaf("REFN"))
    if data is None:
        return None
    decoded = rdimage.decode(data)
    texels = decoded if decoded.ndim == 3 else rdimage.palette(data)[decoded]
    rgb = texels.astype(np.float64) / 255.0
    if smap.leaf("whit") == "1":
        rgb[(texels == 255).all(axis=-1)] = base
    return rgb


def mesh_texture(scene: sc.Scene, obj: sc.Obj, uvmesh) -> tuple[np.ndarray, np.ndarray] | None:
    """(image, (V, 2) texture coordinates) for a mesh master (geometry.MeshUV) whose shader keeps
    a plain primary colour and a texture map in a rectangle domain (``srct``): the saw scene's
    half body wears its half face that way (Ray Dream Studio 5). The rectangle's ``bbox`` is
    (around, along, around, along) in the surface's own coordinates, like Ray Dream 4's domains;
    the whole surface becomes one image, the texture inside the rectangle (unflipped: the eye
    lands where Clear4's icons have it, the 7 other orientations do worse), the primary colour
    elsewhere and on the cut faces."""
    sh = obj.chunk.chunk("sh") if obj.chunk is not None else None
    if sh is None or sh.chunk("elms") is None:
        return None
    rect = next((d for d in sh.chunk("elms").chunks() if d.tag == "srct" and _color_channel(d) is not None
                 and _color_channel(d).tag == "smap"), None)
    if rect is None:
        return None
    base = material_key(scene, obj)
    image = _texels(scene, obj, _color_channel(rect), base)
    if image is None:
        return None
    a0, p0, a1, p1 = rect.floats("bbox")
    along_max, around_max = uvmesh.extent
    h, w = image.shape[:2]
    width, height = int(round(w * around_max / (a1 - a0))), int(round(h * along_max / (p1 - p0)))
    canvas = np.empty((height, width, 3))
    canvas[:] = base
    x0, y0 = int(round(a0 / around_max * width)), int(round(p0 / along_max * height))
    canvas[y0:y0 + h, x0:x0 + w] = image[:max(0, min(h, height - y0)), :max(0, min(w, width - x0))]
    st = np.stack([uvmesh.uv[:, 1] / around_max, uvmesh.uv[:, 0] / along_max], axis=1)
    # Cut faces take the primary colour from the canvas's corner, away from the rectangle.
    corner = np.array([0.5 / width, 0.5 / height]) if (x0, y0) != (0, 0) else np.array([1 - 0.5 / width,
                                                                                            1 - 0.5 / height])
    st[uvmesh.domain != 0] = corner
    return (canvas * 255).round().astype(np.uint8), st


CUBE_ATLAS_PX = 128      # atlas pixels per unit of the cube's surface coordinates
# An image's first row lies at a domain's low v (flpv 0): the invert sign's picture shows Blupi
# upside down as in icon 187 only so (colour 0.850 against 0.815).
CUBE_IMAGE_ROWS_UP = False


def cube_surface_coordinates(positions: np.ndarray, normals: np.ndarray) -> np.ndarray:
    """(N, 2) Ray Dream surface coordinates (u in [0, 4], v in [0, 3]) of a cube primitive's
    vertices, one face per vertex by its normal: the four sides form the middle row, -Y, +X, +Y,
    -X (u 0-1, 1-2, 2-3, 3-4; each seen from outside, u to the right, v up along +Z except on
    -X, where v runs down), the top (+Z) above +X (v 2-3) and the bottom below it (v 0-1). +X at
    u 1-2 is the face toward the scenes' cameras: Decor/techno02.rd4's screen and the switch's
    picture lie there. The -X face's downward v is fitted to the invert sign: its two pictures
    (+X unflipped, -X ``flpv 1``) both show Blupi upside down in icons 187-194, as the sprites'
    front and back views do."""
    import primitives
    h = primitives.CUBE_HALF_SIZE
    x, y, z = (positions / h).T          # -1..1
    ax = np.argmax(np.abs(normals), axis=1)
    sign = np.sign(normals[np.arange(len(normals)), ax])
    u = np.zeros(len(positions))
    v = np.zeros(len(positions))
    side = ax != 2
    s = np.select([(ax == 0) & (sign > 0), (ax == 1) & (sign > 0), (ax == 0) & (sign < 0), (ax == 1) & (sign < 0)],
                  [y, -x, -y, x])
    k = np.select([(ax == 0) & (sign > 0), (ax == 1) & (sign > 0), (ax == 0) & (sign < 0), (ax == 1) & (sign < 0)],
                  [1, 2, 3, 0])
    u[side] = (k + (s + 1) / 2)[side]
    v[side] = (1 + np.where(k == 3, 1 - z, z + 1) / 2)[side]
    top = (ax == 2) & (sign > 0)
    bottom = (ax == 2) & (sign < 0)
    u[top | bottom] = 1 + (y[top | bottom] + 1) / 2
    v[top] = 2 + (1 - x[top]) / 2
    v[bottom] = (1 + x[bottom]) / 2
    return np.stack([u, v], axis=1)


def cube_texture(scene: sc.Scene, obj: sc.Obj, positions: np.ndarray, normals: np.ndarray):
    """(image, (N, 2) texture coordinates) of a cube primitive whose shader has a texture map in
    any domain, or None: the whole surface (``cube_surface_coordinates``) becomes one atlas,
    the primary domain's colour or image first (an image spans the domain's ``bbox`` and repeats
    beyond it), then the rectangle (``srct``) and oval (``sovl``) domains on top, each its colour
    or its image stretched over its ``bbox`` (the invert sign's picture on both board faces)."""
    sh = obj.chunk.chunk("sh") if obj.chunk is not None else None
    if sh is None or sh.chunk("elms") is None:
        return None
    domains = sh.chunk("elms").chunks()
    channels = [(d, _color_channel(d)) for d in domains]
    if not any(ch is not None and ch.tag == "smap" for _d, ch in channels):
        return None
    base = material_key(scene, obj)
    base = base if isinstance(base, tuple) else (1.0, 1.0, 1.0)
    w, h = 4 * CUBE_ATLAS_PX, 3 * CUBE_ATLAS_PX
    us = (np.arange(w) + 0.5) / CUBE_ATLAS_PX
    vs = 3.0 - (np.arange(h) + 0.5) / CUBE_ATLAS_PX       # row 0 at the top (v = 3)
    atlas = np.empty((h, w, 3))
    atlas[:] = base

    def paint(domain, ch, mask_fn, repeat):
        u0, v0, u1, v1 = domain.floats("bbox")
        fu = (us - u0) / (u1 - u0)
        fv = (vs - v0) / (v1 - v0)
        inside = mask_fn(fu[None, :], fv[:, None])
        if ch.tag == "sclr":
            atlas[inside] = ch.floats("colr")
            return
        if ch.tag != "smap":
            return
        img = _texels(scene, obj, ch, base)
        if img is None:
            return
        # "whit 1": white texels let the surface beneath show (the sign's wood around its picture).
        white = _white_texels(scene, ch) if ch.leaf("whit") == "1" and domain.tag != "sprm" else None
        minf = ch.chunk("MInf")
        flph = minf is not None and minf.leaf("flph") == "1"
        flpv = minf is not None and minf.leaf("flpv") == "1"
        ih, iw = img.shape[:2]
        cu = np.mod(fu, 1.0) if repeat else np.clip(fu, 0.0, 1.0 - 1e-9)
        cv = np.mod(fv, 1.0) if repeat else np.clip(fv, 0.0, 1.0 - 1e-9)
        cu = 1.0 - cu if flph else cu
        cv = 1.0 - cv if (flpv != CUBE_IMAGE_ROWS_UP) else cv
        cols = np.minimum((cu * iw).astype(int), iw - 1)
        rows = np.minimum((cv * ih).astype(int), ih - 1)
        sampled = img[rows[:, None], cols[None, :]]
        if white is not None:
            inside = inside & ~white[rows[:, None], cols[None, :]]
        atlas[inside] = sampled[inside]

    full = lambda fu, fv: np.ones((len(fv), fu.shape[1]), dtype=bool)
    rect = lambda fu, fv: (fu >= 0) & (fu <= 1) & (fv >= 0) & (fv <= 1)
    oval = lambda fu, fv: (2 * fu - 1) ** 2 + (2 * fv - 1) ** 2 <= 1
    for d, ch in channels:
        if d.tag == "sprm" and ch is not None:
            paint(d, ch, full, True)
    for d, ch in reversed(channels):         # earlier domains lie on top
        if d.tag in ("srct", "sovl") and ch is not None:
            paint(d, ch, rect if d.tag == "srct" else oval, False)
    uv = cube_surface_coordinates(positions, normals)
    st = np.stack([uv[:, 0] / 4.0, (3.0 - uv[:, 1]) / 3.0], axis=1)
    return (atlas * 255).round().clip(0, 255).astype(np.uint8), st


FREEFORM_ATLAS_PX = 64           # atlas pixels per unit of a free-form's surface coordinates
FREEFORM_IMAGE_COLUMNS_ALONG = True  # a domain image's columns run along the path (fitted below)


def freeform_texture(scene: sc.Scene, obj: sc.Obj, ff):
    """(image, (N, 2) texture coordinates of ``ff``'s vertices) for a free-form whose primary
    domain keeps a colour while rectangle (``srct``) or oval (``sovl``) domains carry texture
    maps, or None: the bulldozer's blade (tracks-profil.rd4) wears its hazard stripes so. The
    surface becomes one atlas over (around, along) = (0..perimeter, 0..path length), the
    domains' ``bbox`` being (around, along, around, along) as for mesh masters; an image fills
    its rectangle with its repeats (``cnth``/``cntv`` when ``tile 1``), its columns along the
    path. Free-forms whose primary domain is a texture map (Blupi's face) keep
    ``freeform_mesh.texture_coordinates``."""
    sh = obj.chunk.chunk("sh") if obj.chunk is not None else None
    if sh is None or sh.chunk("elms") is None:
        return None
    channels = [(d, _color_channel(d)) for d in sh.chunk("elms").chunks()]
    if not any(d.tag in ("srct", "sovl") and ch is not None and ch.tag == "smap" for d, ch in channels):
        return None
    base = material_key(scene, obj)
    base = base if isinstance(base, tuple) else (1.0, 1.0, 1.0)
    around_max = float(ff.perimeter)
    along_max = float(max(ff.uv[:, 1].max(), 1e-6))
    w = int(np.clip(round(around_max * FREEFORM_ATLAS_PX), 16, 1024))
    h = int(np.clip(round(along_max * FREEFORM_ATLAS_PX), 16, 1024))
    us = (np.arange(w) + 0.5) / w * around_max
    vs = (np.arange(h) + 0.5) / h * along_max
    atlas = np.empty((h, w, 3))
    atlas[:] = base
    for d, ch in reversed(channels):         # earlier domains lie on top
        if d.tag not in ("srct", "sovl") or ch is None:
            continue
        a0, p0, a1, p1 = d.floats("bbox")
        fa = (us - a0) / (a1 - a0)
        fp = (vs - p0) / (p1 - p0)
        if d.tag == "srct":
            inside = (fp[:, None] >= 0) & (fp[:, None] <= 1) & (fa[None, :] >= 0) & (fa[None, :] <= 1)
        else:
            inside = (2 * fp[:, None] - 1) ** 2 + (2 * fa[None, :] - 1) ** 2 <= 1
        if ch.tag == "sclr":
            atlas[inside] = ch.floats("colr")
            continue
        if ch.tag != "smap":
            continue
        img = _texels(scene, obj, ch, base)
        if img is None:
            continue
        minf = ch.chunk("MInf")
        leaf = (lambda k: minf.leaf(k)) if minf is not None else (lambda k: None)
        reps = (int(leaf("cnth") or 1), int(leaf("cntv") or 1)) if leaf("tile") == "1" else (1, 1)
        # Image axes: columns (horizontal, cnth) and rows (vertical, cntv).
        fx, fy = (fp, fa) if FREEFORM_IMAGE_COLUMNS_ALONG else (fa, fp)
        cx = np.mod(np.clip(fx, 0, 1 - 1e-9) * reps[0], 1.0)
        cy = np.mod(np.clip(fy, 0, 1 - 1e-9) * reps[1], 1.0)
        cx = 1.0 - cx if leaf("flph") == "1" else cx
        cy = 1.0 - cy if leaf("flpv") == "1" else cy
        ih, iw = img.shape[:2]
        cols = np.minimum((cx * iw).astype(int), iw - 1)
        rows = np.minimum((cy * ih).astype(int), ih - 1)
        if FREEFORM_IMAGE_COLUMNS_ALONG:
            sampled = img[rows[None, :], cols[:, None]]       # (along, around)
        else:
            sampled = img[rows[:, None], cols[None, :]]
        atlas[inside] = sampled[inside]
    # Ray Dream's "around" runs against the section loop's direction (with the right-handed
    # section frame): measured so, the blade's stripes land on its curved front face, as in
    # the front icons 60-63; along the loop they sat on the flat face toward the cab.
    around = np.mod(around_max - ff.uv[:, 0], around_max)
    st = np.stack([around / around_max, ff.uv[:, 1] / along_max], axis=1)
    return (atlas * 255).round().clip(0, 255).astype(np.uint8), st


def primitive_texture(tex: TextureMap, kind: str, positions: np.ndarray, normals: np.ndarray):
    """(image, (N, 2) texture coordinates) of a texture map on a cylinder, sphere or cone: around the
    axis ``s`` = angle; ``t`` is the meridian fraction of the canonical cylinder (radius 3,
    height 5): the -Z cap from its centre out (0 to 3/11), the side along +Z (3/11 to 8/11),
    the +Z cap back to its centre (to 1), as free-forms' end caps count. The pickup lollipop's
    disc (sucette.rd4, ``cnth 2 cntv 2``) so shows the sprites' four-arm pinwheel twisting
    about a quarter turn (icons 136-143 IoU x colour 0.821; 0.807 with ``t`` the cap's radius,
    a tight many-turn spiral), the lollipop Blupi holds is unchanged (234-236: 0.850 / 0.851).
    Spheres and cones take the same rule: a sphere's latitude from its -Z pole, a cone's base
    from its centre then its side to the apex. The map's repeats are baked into the image (so
    ``t`` stays in [0, 1]); flips apply; the other placements (rotc) are not read. Returns None
    for cubes (``cube_texture``) and other primitives."""
    import primitives
    x, y, z = positions.T
    s = np.mod(np.arctan2(y, x) / (2 * np.pi), 1.0)
    rad = np.hypot(x, y)
    if kind == "cyli":
        r, h = primitives.CYLINDER_RADIUS, primitives.CYLINDER_HALF_HEIGHT
        length = 2 * r + 2 * h
        cap = np.abs(normals[:, 2]) > 0.5
        top = cap & (normals[:, 2] > 0)
        bottom = cap & ~top
        t = np.where(bottom, rad / length, np.where(top, (length - rad) / length, (r + z + h) / length))
    elif kind == "sp":
        # Latitude from the -Z pole, the sphere's meridian fraction.
        t = np.arccos(np.clip(-z / primitives.SPHERE_RADIUS, -1.0, 1.0)) / np.pi
    elif kind == "co":
        # The cone's meridian: its base from the centre out, then the side up to the apex.
        r, h = primitives.CONE_RADIUS, primitives.CONE_HALF_HEIGHT
        slant = np.hypot(r, 2 * h)
        base = normals[:, 2] < -0.5
        side_t = (r + (1.0 - rad / r) * slant) / (r + slant)
        t = np.where(base, rad / (r + slant), side_t)
    else:
        return None
    flph, flpv = tex.flips
    s = 1.0 - s if flph else s
    t = 1.0 - t if flpv else t
    cnth, cntv = tex.counts
    return np.tile(tex.image, (cntv, cnth, 1)), np.stack([s, np.clip(t, 0.0, 1.0)], axis=1)


def textured_material_json(name: str, texture: int) -> dict:
    return {
        "name": f"rd_tex_{name.replace(' ', '_')}",
        "pbrMetallicRoughness": {
            "baseColorTexture": {"index": texture},
            "metallicFactor": 0.0,
            "roughnessFactor": 0.5,
        },
    }
