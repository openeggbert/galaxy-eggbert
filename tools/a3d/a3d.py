#!/usr/bin/env python3
"""Reader for the original Amapi 3D (.a3d) sources in speedyblupi-data (plans/audit/a3d_format.md).

File: "AMAP" + big-endian u32 version (1), then chunks: 8-byte ASCII tag, big-endian u32 size, body.
A BLOKSHAP chunk is one shape; its body holds sub-chunks:
  blokgeom  3 bytes, u32 V, u32 E, u32 F, u32 I, 16 zero bytes, u32 (an id), then
            V vertices of 4 big-endian floats (x, y, z, w) in inches,
            E edges of 2 u32 vertex indices,
            I u32 face references: each face is a count n followed by n vertex indices.
  blokname  the shape's name (NUL-padded), blokinfo (layer name, flags), blokmatx (60 bytes).
Shapes on the "Courbes" layer are construction curves (no faces).

Usage:
  python3 tools/a3d/a3d.py info FILE...            per shape: name, layer, V/E/F, bounds (metres)
  python3 tools/a3d/a3d.py obj FILE OUT.obj        all faced shapes as one Wavefront OBJ (metres, Y up)
  python3 tools/a3d/a3d.py survey DIR              every .a3d under DIR: shapes, faces, parse errors
"""
import os
import struct
import sys

INCH = 0.0254


class Shape:
    def __init__(self):
        self.name = ""
        self.layer = ""
        self.vertices = []   # (x, y, z) inches
        self.edges = []
        self.faces = []      # lists of vertex indices
        self.matrix = None   # the 15 floats of blokmatx


def _chunks(data, start, end):
    off = start
    while off + 12 <= end:
        tag = data[off:off + 8]
        size = struct.unpack_from(">I", data, off + 8)[0]
        body = off + 12
        if body + size > end:
            raise ValueError("chunk %r at %d overruns its parent" % (tag, off))
        yield tag, body, size
        off = body + size


def _cstr(raw):
    return raw.split(b"\0", 1)[0].decode("latin-1")


def _geometry(shape, data, body, size):
    v, e, f, i = struct.unpack_from(">4I", data, body + 3)
    p = body + 39
    need = 39 + 16 * v + 8 * e + 4 * i
    if need != size:
        raise ValueError("blokgeom: %d bytes for V=%d E=%d F=%d I=%d, expected %d" % (size, v, e, f, i, need))
    for k in range(v):
        x, y, z, _ = struct.unpack_from(">4f", data, p + 16 * k)
        shape.vertices.append((x, y, z))
    p += 16 * v
    shape.edges = [struct.unpack_from(">2I", data, p + 8 * k) for k in range(e)]
    p += 8 * e
    refs = struct.unpack_from(">%di" % i, data, p)
    k = 0
    while k < len(refs):
        n = refs[k]
        if n < 3 or k + 1 + n > len(refs):
            raise ValueError("blokgeom: bad face count %d at reference %d" % (n, k))
        face = list(refs[k + 1:k + 1 + n])
        if any(ix < 0 or ix >= v for ix in face):
            raise ValueError("blokgeom: face index out of range")
        shape.faces.append(face)
        k += 1 + n
    if len(shape.faces) != f:
        raise ValueError("blokgeom: %d faces read, header says %d" % (len(shape.faces), f))


def read(path):
    data = open(path, "rb").read()
    if data[:4] != b"AMAP":
        raise ValueError("not an Amapi file")
    shapes = []
    for tag, body, size in _chunks(data, 8, len(data)):
        if tag != b"BLOKSHAP":
            continue
        shape = Shape()
        for sub, sbody, ssize in _chunks(data, body, body + size):
            if sub == b"blokgeom":
                _geometry(shape, data, sbody, ssize)
            elif sub == b"blokname":
                shape.name = _cstr(data[sbody:sbody + ssize])
            elif sub == b"blokinfo":
                raw = data[sbody:sbody + ssize]
                if raw.startswith(b"layrname"):
                    shape.layer = _cstr(raw[12:])
            elif sub == b"blokmatx" and ssize == 60:
                shape.matrix = struct.unpack_from(">15f", data, sbody)
        shapes.append(shape)
    return shapes


def bounds(shape):
    xs, ys, zs = zip(*shape.vertices) if shape.vertices else ((0,), (0,), (0,))
    return [(min(c) * INCH, max(c) * INCH) for c in (xs, ys, zs)]


def write_obj(shapes, out):
    with open(out, "w", encoding="utf-8") as f:
        base = 1
        for s in shapes:
            if not s.faces:
                continue
            f.write("o %s\n" % (s.name or "shape"))
            for x, y, z in s.vertices:
                f.write("v %.5f %.5f %.5f\n" % (x * INCH, y * INCH, z * INCH))
            for face in s.faces:
                f.write("f %s\n" % " ".join(str(base + ix) for ix in face))
            base += len(s.vertices)


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cmd = argv[1]
    if cmd == "info":
        for path in argv[2:]:
            print(path)
            for s in read(path):
                b = bounds(s)
                print("  %-12s %-8s V=%4d E=%4d F=%4d  x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f m" %
                      (s.name, s.layer, len(s.vertices), len(s.edges), len(s.faces), *[c for r in b for c in r]))
        return 0
    if cmd == "obj":
        write_obj(read(argv[2]), argv[3])
        return 0
    if cmd == "survey":
        ok = bad = shapes = faced = faces = 0
        for dirpath, _, names in sorted(os.walk(argv[2])):
            for name in sorted(names):
                if name.lower().endswith(".a3d"):
                    path = os.path.join(dirpath, name)
                    try:
                        ss = read(path)
                        ok += 1
                        shapes += len(ss)
                        faced += sum(1 for s in ss if s.faces)
                        faces += sum(len(s.faces) for s in ss)
                    except Exception as exc:  # report and go on
                        bad += 1
                        print("FAIL %s: %s" % (os.path.relpath(path, argv[2]), exc))
        print("files read %d, failed %d; shapes %d (with faces %d), faces %d" % (ok, bad, shapes, faced, faces))
        return 0 if bad == 0 else 1
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
