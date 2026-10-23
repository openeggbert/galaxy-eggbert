"""Scene model and forward kinematics for Ray Dream ASCII scenes.

Conventions established against marche-profil.rd4 (see README.md in this directory):

* World is Z-up. A 3x3 ``mat`` maps local to parent coordinates as ``M @ v`` (row-major text);
  cameras look down their local -Z with +Y up, like OpenGL.
* ``ROTA w,x,y,z,spins`` orientation keys store the inverse rotation: ``mat == quat_to_matrix(q).T``.
* An object is placed by its hot point ``hp`` (local, after the ``affi`` deformer): the hot point
  lands on ``po`` in the parent, and the object rotates about it.
* ``Scal`` scales the object's own geometry about its hot point. It does NOT scale children:
  a child's ``po`` is in the parent's local frame in unscaled units.
* A ``link`` joint rotates the object about its hot point, inside its ``mat`` base frame.
* Key interpolation without an explicit tweener (``void``) is linear; scene time runs at
  1800 ticks per second.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from rd4 import Chunk, parse_floats

TICKS_PER_SECOND = 1800.0

# FourCC-style integers used by axis links.
AXIS_CODES = {0x58415849: 0, 0x59415849: 1, 0x5A415849: 2}  # 'XAXI', 'YAXI', 'ZAXI'

# Generic-joint rotation order ``ORDE``: the composed matrix is R[order[0]] @ R[order[1]] @ R[order[2]].
# Only ORDE 6 occurs in the Speedy Blupi scenes; it was verified against blupi.dxf.
EULER_ORDERS = {6: "ZYX"}


def quat_to_matrix(w: float, x: float, y: float, z: float) -> np.ndarray:
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
        [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
        [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)],
    ])


def orthonormalize(m: np.ndarray) -> np.ndarray:
    """Nearest orthogonal matrix (keeps a reflection's sign); ``mat`` is stored to 5 decimals."""
    u, _s, vt = np.linalg.svd(m)
    return u @ vt


def axis_rotation(axis: int, angle: float) -> np.ndarray:
    c, s = np.cos(angle), np.sin(angle)
    if axis == 0:
        return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])
    if axis == 1:
        return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


@dataclass
class Track:
    """Keyframes of one channel: times in ticks, values as float arrays."""
    times: list[float]
    values: list[np.ndarray]

    def sample(self, t: float) -> np.ndarray:
        if t <= self.times[0]:
            return self.values[0]
        if t >= self.times[-1]:
            return self.values[-1]
        for i in range(len(self.times) - 1):
            t0, t1 = self.times[i], self.times[i + 1]
            if t0 <= t <= t1:
                f = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
                return self.values[i] * (1 - f) + self.values[i + 1] * f
        return self.values[-1]


def slerp(a: np.ndarray, b: np.ndarray, f: float) -> np.ndarray:
    if a @ b < 0:
        b = -b
    d = float(np.clip(a @ b, -1.0, 1.0))
    th = np.arccos(d)
    if th < 1e-9:
        return a
    return (np.sin((1 - f) * th) * a + np.sin(f * th) * b) / np.sin(th)


@dataclass
class QuatTrack(Track):
    def sample(self, t: float) -> np.ndarray:
        if t <= self.times[0]:
            return self.values[0]
        if t >= self.times[-1]:
            return self.values[-1]
        for i in range(len(self.times) - 1):
            t0, t1 = self.times[i], self.times[i + 1]
            if t0 <= t <= t1:
                f = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
                return slerp(self.values[i], self.values[i + 1], f)
        return self.values[-1]


def read_track(chunk: Chunk, quat: bool = False) -> Track | None:
    times, values = [], []
    tweeners = set()
    for item in chunk.items:
        if isinstance(item, tuple):
            tag, v = item
            if tag == "time":
                times.append(float(v))
            elif tag == "void":
                tweeners.add("void")
            elif len(values) < len(times):
                values.append(np.array(parse_floats(v)))
        else:
            tweeners.add(item.tag)
    if not times:
        return None
    if tweeners - {"void"}:
        raise NotImplementedError(f"tweeners {sorted(tweeners)} in {chunk.tag}")
    if quat:
        return QuatTrack(times, [v[:4] / np.linalg.norm(v[:4]) for v in values])
    return Track(times, values)


@dataclass
class Joint:
    kind: str                      # 'gene', 'axis', 'ball', 'lock'
    axis: int = 0                  # for 'axis'
    order: int = 0                 # ORDE for 'gene'
    ball: np.ndarray = field(default_factory=lambda: np.eye(3))  # matR for 'ball'
    angles: np.ndarray = field(default_factory=lambda: np.zeros(3))
    slides: np.ndarray = field(default_factory=lambda: np.zeros(3))
    angle_tracks: list = field(default_factory=lambda: [None, None, None])
    slide_tracks: list = field(default_factory=lambda: [None, None, None])


@dataclass
class Obj:
    name: str
    kind: str                      # 'gr', 'inpr', 'ca', 'li', ...
    master: str | None
    po: np.ndarray
    mat: np.ndarray
    scal: float
    hp: np.ndarray
    affi_scale: np.ndarray
    affi_rot: np.ndarray
    affi_tra: np.ndarray
    joint: Joint | None
    pos_tracks: list               # [X, Y, Z] Track | None
    orie_track: QuatTrack | None
    hotp_track: Track | None
    scal_track: Track | None
    affi_tracks: list              # [ScaX, ScaY, ScaZ] Track | None
    children: list["Obj"] = field(default_factory=list)
    chunk: Chunk | None = None

    # ---- evaluation -------------------------------------------------------------
    def position(self, t: float | None) -> np.ndarray:
        p = self.po.copy()
        if t is not None:
            for i, tr in enumerate(self.pos_tracks):
                if tr is not None:
                    p[i] = tr.sample(t)[0]
        return p

    def orientation(self, t: float | None) -> np.ndarray:
        if t is not None and self.orie_track is not None:
            w, x, y, z = self.orie_track.sample(t)
            return quat_to_matrix(w, x, y, z).T
        return orthonormalize(self.mat)

    def hot_point(self, t: float | None) -> np.ndarray:
        if t is not None and self.hotp_track is not None:
            return self.hotp_track.sample(t)
        return self.hp

    def scale(self, t: float | None) -> float:
        if t is not None and self.scal_track is not None:
            return float(self.scal_track.sample(t)[0])
        return self.scal

    def deformer(self, t: float | None) -> np.ndarray:
        """3x3 linear part of the affine deformer (translation kept separately)."""
        s = self.affi_scale.copy()
        if t is not None:
            for i, tr in enumerate(self.affi_tracks):
                if tr is not None:
                    s[i] = tr.sample(t)[0]
        return self.affi_rot @ np.diag(s)

    def joint_rotation(self, t: float | None) -> np.ndarray:
        j = self.joint
        if j is None or j.kind == "lock":
            return np.eye(3)
        if j.kind == "ball":
            return j.ball
        if j.kind == "axis":
            a = j.angles[0]
            if t is not None and j.angle_tracks[0] is not None:
                a = j.angle_tracks[0].sample(t)[0]
            return axis_rotation(j.axis, a)
        ang = j.angles.copy()
        if t is not None:
            for i, tr in enumerate(j.angle_tracks):
                if tr is not None:
                    ang[i] = tr.sample(t)[0]
        if j.order not in EULER_ORDERS:
            raise NotImplementedError(f"{self.name}: generic joint ORDE {j.order}")
        r = np.eye(3)
        for ax in EULER_ORDERS[j.order]:
            i = "XYZ".index(ax)
            r = r @ axis_rotation(i, ang[i])
        return r


def _vec(chunk: Chunk | None, tag: str, default) -> np.ndarray:
    if chunk is None or chunk.leaf(tag) is None:
        return np.array(default, dtype=float)
    return np.array(chunk.floats(tag), dtype=float)


def _read_joint(link: Chunk | None) -> Joint | None:
    if link is None:
        return None
    kinds = link.chunks()
    if not kinds:
        return None
    k = kinds[0]
    j = Joint(kind=k.tag)
    ac = k.chunk("ACom")
    if k.tag == "axis":
        j.axis = AXIS_CODES.get(int(k.leaf("axis", "0")), 0)
        j.angles = np.array([k.float("AAVa", 0.0), 0.0, 0.0])
        if ac and ac.chunk("AAVa"):
            j.angle_tracks[0] = read_track(ac.chunk("AAVa"))
    elif k.tag == "ball":
        if k.leaf("matR"):
            j.ball = np.array(k.floats("matR")).reshape(3, 3)
        if ac and ac.chunks():
            raise NotImplementedError("animated ball joints")
    elif k.tag == "gene":
        j.order = int(k.leaf("ORDE", "0"))
        j.angles = np.array([k.float(f"A{a}Va", 0.0) for a in "XYZ"])
        j.slides = np.array([k.float(f"S{a}Va", 0.0) for a in "XYZ"])
        if ac:
            for i, a in enumerate("XYZ"):
                if ac.chunk(f"A{a}Va"):
                    j.angle_tracks[i] = read_track(ac.chunk(f"A{a}Va"))
                if ac.chunk(f"S{a}Va"):
                    j.slide_tracks[i] = read_track(ac.chunk(f"S{a}Va"))
    elif k.tag != "lock":
        raise NotImplementedError(f"link kind {k.tag!r}")
    return j


def read_obj(ch: Chunk) -> Obj:
    tran = ch.chunk("tran")
    mat = np.eye(3)
    if tran is not None and tran.leaf("mat"):
        mat = np.array(tran.floats("mat")).reshape(3, 3)
    pos_tracks = [None, None, None]
    orie = hotp = scal = None
    if tran is not None:
        ap = tran.chunk("APos")
        if ap:
            for i, a in enumerate(("XCOO", "YCOO", "ZCOO")):
                if ap.chunk(a):
                    pos_tracks[i] = read_track(ap.chunk(a))
        at = tran.chunk("ATra")
        if at:
            if at.chunk("ORIE"):
                orie = read_track(at.chunk("ORIE"), quat=True)
            if at.chunk("HOTP"):
                hotp = read_track(at.chunk("HOTP"))
            if at.chunk("SCAL"):
                scal = read_track(at.chunk("SCAL"))
    affi = ch.chunk("defo").chunk("affi") if ch.chunk("defo") else None
    affi_scale = np.ones(3)
    affi_rot = np.eye(3)
    affi_tra = np.zeros(3)
    affi_tracks = [None, None, None]
    if affi is not None:
        affi_scale = np.array([affi.float(f"Sca{a}", 1.0) for a in "XYZ"])
        affi_rot = np.array([[affi.float(f"R{r},{c}", 1.0 if r == c else 0.0) for c in (1, 2, 3)]
                             for r in (1, 2, 3)])
        affi_tra = np.array([affi.float(f"Tra{i}", 0.0) for i in (1, 2, 3)])
        ac = affi.chunk("ACom")
        if ac:
            for i, a in enumerate("XYZ"):
                if ac.chunk(f"Sca{a}"):
                    affi_tracks[i] = read_track(ac.chunk(f"Sca{a}"))
    obj = Obj(
        name=ch.string("nm", ""),
        kind=ch.tag,
        master=ch.string("ob"),
        po=_vec(tran, "po", [0, 0, 0]),
        mat=mat,
        scal=tran.float("Scal", 1.0) if tran is not None else 1.0,
        hp=_vec(tran, "hp", [0, 0, 0]),
        affi_scale=affi_scale,
        affi_rot=affi_rot,
        affi_tra=affi_tra,
        joint=_read_joint(ch.chunk("link")),
        pos_tracks=pos_tracks,
        orie_track=orie,
        hotp_track=hotp,
        scal_track=scal,
        affi_tracks=affi_tracks,
        chunk=ch,
    )
    so = ch.chunk("so")
    if so is not None:
        obj.children = [read_obj(c) for c in so.chunks() if c.chunk("tran") is not None or c.leaf("nm")]
    return obj


@dataclass
class Scene:
    root: Chunk
    universe: Obj
    masters: dict[str, Chunk]
    begin: float
    end: float
    fps: float

    def find(self, name: str) -> Obj | None:
        stack = [self.universe]
        while stack:
            o = stack.pop()
            if o.name == name:
                return o
            stack.extend(o.children)
        return None


def _time(v: str | None, default: float) -> float:
    if v is None:
        return default
    a = parse_floats(v)
    return a[0] / a[1] * TICKS_PER_SECOND if len(a) > 1 and a[1] else a[0]


def read_scene(root: Chunk) -> Scene:
    sc = root.chunk("sc")
    masters = {}
    ob = sc.chunk("ob")
    if ob is not None:
        for m in ob.chunks():
            masters[m.string("nm")] = m
    universe = read_obj(sc.chunk("tr").chunks()[0])
    anim = root.chunk("3DC").chunk("RSet").chunk("anim") if root.chunk("3DC") else None
    begin = _time(anim.leaf("begT") if anim else None, 0.0)
    end = _time(anim.leaf("endT") if anim else None, 0.0)
    fps = anim.float("FSec", 1.0) if anim else 1.0
    return Scene(root, universe, masters, begin, end, fps)


# ---- forward kinematics -------------------------------------------------------------

def affine(lin: np.ndarray, tra: np.ndarray) -> np.ndarray:
    m = np.eye(4)
    m[:3, :3] = lin
    m[:3, 3] = tra
    return m


def child_frame(o: Obj, t: float | None) -> np.ndarray:
    """4x4 mapping this object's local (child-space) coordinates to its parent's local coordinates."""
    lin = o.orientation(t) @ o.joint_rotation(t)
    hp = o.hot_point(t)
    return affine(lin, o.position(t) - lin @ hp)


def geometry_frame(o: Obj, t: float | None) -> np.ndarray:
    """4x4 mapping the object's canonical master geometry to its parent's local coordinates."""
    lin = o.orientation(t) @ o.joint_rotation(t)
    s = o.scale(t)
    hp = o.hot_point(t)
    deform = affine(o.deformer(t), o.affi_tra)
    return affine(lin * s, o.position(t) - (lin * s) @ hp) @ deform


def world_frames(scene: Scene, t: float | None):
    """Yield (obj, parent_obj, child_space_to_world, geometry_to_world) for every object."""
    out = []

    def rec(o: Obj, parent: Obj | None, parent_world: np.ndarray):
        cw = parent_world @ child_frame(o, t)
        gw = parent_world @ geometry_frame(o, t)
        out.append((o, parent, cw, gw))
        for c in o.children:
            rec(c, o, cw)

    rec(scene.universe, None, np.eye(4))
    return out
