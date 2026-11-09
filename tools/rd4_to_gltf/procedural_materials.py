#!/usr/bin/env python3
"""The Ray Dream procedural materials of the converted objects, for the game's 3D effects.

The converter (shading.py) flattens a mix shader (``smix``: colours blended by a pattern -- wood
``swod``, marble ``smrb``, spots ``sspt``) to the mean of its colours, and leaves the bump channel
out; the sprites show the patterns. This writes, beside each object model of objects.txt
(``avatars3d/objects/<name>_<scene>/materials.txt``), what each flat part's shader was: its colours
(toned and recoloured as the converter toned the part) and the patterns that mix them, and its bump
(a spots pattern, or an image: ``grain``). The game's model shader draws them at the "3D effects"
level high (src/GalaxyEggbert/Game/ProceduralMaterial.hpp reads the file).

    material <part suffix rd_rrggbb>
    colour <r> <g> <b>                      the first colour (0..1)
    layer <kind> <r> <g> <b> <scale> <spacing> <perturbation> <axis> <blend>
                                            mixed over what comes before by the pattern
                                            (kind: marble, wood, spots; axis 0 x, 1 y, 2 z)
    bump <kind> <amplitude> <scale> <threshold>   (kind: spots, grain)
    surface <highlight> <shininess> <reflection> <glow r> <glow g> <glow b>
                                            the shader's other channels (every part has one; a
                                            channel that is a pattern or animated: its default)

Usage: procedural_materials.py --originals <speedyblupi-data/originals> --objects objects.txt
       --out <avatars3d/objects>
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

import build_object_library as bol
import rd4
import scene as sc
import shading

PATTERNS = {"smrb": "marble", "swod": "wood", "sspt": "spots"}


def channels(sh):
    """The primary domain's component channels (colour, highlight, shininess, bump, ...), or None."""
    if sh is None or sh.chunk("elms") is None:
        return None
    primary = next((d for d in sh.chunk("elms").chunks() if d.tag == "sprm"), None)
    if primary is None or primary.chunk("shdr") is None:
        return None
    scmp = primary.chunk("shdr").chunk("scmp")
    if scmp is None or scmp.chunk("ssub") is None:
        return None
    return scmp.chunk("ssub").chunks()


def toned(colour) -> tuple[float, float, float]:
    key = tuple(round(float(c), 3) for c in colour)
    key = shading.COLOUR_MAP.get(key, key)
    return tuple(round(float(c), 4) for c in shading.apply_tone(np.array(key, dtype=np.float64)))


def pattern_params(chunk) -> tuple[float, float, float, int, float]:
    def value(tag, default):
        v = chunk.leaf(tag)
        try:
            return float(v) if v is not None else default
        except ValueError:
            return default
    # Ray Dream's DIR counts axes from 1 (and 4 is along Y again in the files seen): to 0..2.
    axis = int(value("DIR", 1)) - 1
    return value("SCAL", 3.0), value("SPAC", 4.0), value("PERT", 1.0), axis % 3, value("BLND", 0.5)


def flatten(mix, layers: list) -> tuple[float, float, float] | None:
    """A mix as its first colour plus layers mixed over it; nested mixes on the first input only."""
    sub = mix.chunk("ssub")
    if sub is None:
        return None
    inputs = [c for c in sub.chunks() if c.tag in ("sclr", "smix")]
    pattern = next((c for c in sub.chunks() if c.tag in PATTERNS), None)
    if len(inputs) < 2 or pattern is None:
        return None
    first, second = inputs[0], inputs[1]
    if first.tag == "smix":
        base = flatten(first, layers)
    elif first.leaf("colr"):
        base = toned(first.floats("colr"))
    else:
        return None
    if base is None:
        return None
    if second.tag == "sclr" and second.leaf("colr"):
        top = toned(second.floats("colr"))
    elif second.tag == "smix":
        mixed = shading._mix_colour(second)
        if mixed is None:
            return None
        top = toned(mixed)
    else:
        return None
    layers.append((PATTERNS[pattern.tag], top, pattern_params(pattern)))
    return base


def bump_of(chunk) -> tuple[str, float, float, float] | None:
    if chunk is None:
        return None
    if chunk.tag == "smix":
        sub = chunk.chunk("ssub")
        if sub is None:
            return None
        cols = [c.floats("colr") for c in sub.chunks() if c.tag == "sclr" and c.leaf("colr")]
        spots = next((c for c in sub.chunks() if c.tag == "sspt"), None)
        if spots is not None and len(cols) >= 2:
            amplitude = abs(float(np.mean(cols[1])) - float(np.mean(cols[0])))
            threshold = float(spots.leaf("TRSH") or 0.5)
            return ("spots", round(amplitude, 4), 1.0, round(threshold, 4))
        if any(c.tag == "smap" for c in sub.chunks()):
            return ("grain", 0.35, 1.0, 0.5)
    if chunk.tag == "smap":
        return ("grain", 0.35, 1.0, 0.5)
    return None


def channel_value(chunk, default: float) -> float:
    """A plain value channel's value (``sval``), or the default (a pattern, an animation)."""
    if chunk is None or chunk.tag != "sval":
        return default
    try:
        return float(chunk.leaf("Val")) if chunk.leaf("Val") is not None else default
    except ValueError:
        return default


def surface_of(chans) -> tuple[float, float, float, tuple[float, float, float]]:
    """Highlight, shininess, reflection (channels 1, 2, 4) and glow (7: a colour)."""
    def at(i):
        return chans[i] if len(chans) > i else None
    glow = (0.0, 0.0, 0.0)
    if at(7) is not None and at(7).tag == "sclr" and at(7).leaf("colr"):
        glow = tuple(float(c) for c in at(7).floats("colr")[:3])
    return channel_value(at(1), 0.5), channel_value(at(2), 0.5), channel_value(at(4), 0.0), glow


def materials_of(scene: sc.Scene) -> dict[str, tuple[list[str], bool]]:
    out: dict[str, tuple[list[str], bool]] = {}
    stack = [scene.universe]
    while stack:
        obj = stack.pop()
        stack.extend(obj.children)
        sh = obj.chunk.chunk("sh") if obj.chunk is not None else None
        chans = channels(sh)
        if chans is None:
            master = scene.masters.get(obj.master)
            chans = channels(master.chunk("sh") if master is not None else None)
        if not chans:
            continue
        layers: list = []
        base = flatten(chans[0], layers) if chans[0].tag == "smix" else None
        bump = bump_of(chans[3]) if len(chans) > 3 else None
        highlight, shininess, reflection, glow = surface_of(chans)
        r, g, b = shading.material_key(scene, obj)
        name = f"rd_{int(r * 255):02x}{int(g * 255):02x}{int(b * 255):02x}"
        patterned = base is not None or bump is not None
        if name in out and (out[name][1] or not patterned):
            # Parts of one flat colour are one part: the first shader with a pattern or a bump stands
            # for them, else the first.
            continue
        lines = [f"material {name}"]
        if base is not None:
            lines.append("colour {:.4f} {:.4f} {:.4f}".format(*base))
            for kind, (cr, cg, cb), (scale, spacing, perturbation, axis, blend) in layers:
                lines.append(f"layer {kind} {cr:.4f} {cg:.4f} {cb:.4f} {scale:.4f} {spacing:.4f} {perturbation:.4f}"
                             f" {axis} {blend:.4f}")
        if bump is not None:
            kind, amplitude, scale, threshold = bump
            lines.append(f"bump {kind} {amplitude:.4f} {scale:.4f} {threshold:.4f}")
        lines.append(f"surface {highlight:.4f} {shininess:.4f} {reflection:.4f} {glow[0]:.4f} {glow[1]:.4f} {glow[2]:.4f}")
        out[name] = (lines, patterned)
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--originals", required=True, type=Path)
    ap.add_argument("--objects", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args(argv)
    written = 0
    for spec in bol.read_specs(args.objects):
        shading.COLOUR_MAP.clear()
        shading.COLOUR_MAP.update(spec.recolour)
        shading.TONE = spec.tone
        for scene_path in spec.scenes:
            target = args.out / f"{spec.name}_{Path(scene_path).stem}"
            if not target.is_dir():
                continue
            scene = sc.read_scene(rd4.parse_file(args.originals / scene_path))
            materials = materials_of(scene)
            path = target / "materials.txt"
            if not materials:
                if path.exists():
                    path.unlink()
                continue
            text = [f"# {spec.name}: the Ray Dream procedural materials of {scene_path};"
                    " generated by procedural_materials.py"]
            for name in sorted(materials):
                text.extend(materials[name][0])
            path.write_text("\n".join(text) + "\n")
            written += 1
    print(f"{written} materials.txt written")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
