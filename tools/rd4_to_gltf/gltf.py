"""Small self-contained glTF 2.0 binary (.glb) writer and reader (no third-party dependency)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

import numpy as np

FLOAT, UBYTE, USHORT, UINT = 5126, 5121, 5123, 5125
ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER = 34962, 34963
_TYPES = {1: "SCALAR", 2: "VEC2", 3: "VEC3", 4: "VEC4", 16: "MAT4"}
_NP = {FLOAT: np.float32, UBYTE: np.uint8, USHORT: np.uint16, UINT: np.uint32}


class GltfBuilder:
    def __init__(self, generator: str):
        self.doc = {"asset": {"version": "2.0", "generator": generator}, "buffers": [],
                    "bufferViews": [], "accessors": []}
        self.blob = bytearray()

    def _list(self, key: str) -> list:
        return self.doc.setdefault(key, [])

    def add(self, key: str, item: dict) -> int:
        lst = self._list(key)
        lst.append(item)
        return len(lst) - 1

    def accessor(self, data: np.ndarray, component: int, target: int | None = None,
                 minmax: bool = False) -> int:
        arr = np.ascontiguousarray(data, dtype=_NP[component])
        count = arr.shape[0]
        width = 1 if arr.ndim == 1 else int(np.prod(arr.shape[1:]))
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": arr.nbytes}
        if target is not None:
            view["target"] = target
        self.blob += arr.tobytes()
        acc = {"bufferView": self.add("bufferViews", view), "componentType": component,
               "count": count, "type": _TYPES[width]}
        if minmax:
            flat = arr.reshape(count, width)
            acc["min"] = [float(x) for x in flat.min(0)]
            acc["max"] = [float(x) for x in flat.max(0)]
        return self.add("accessors", acc)

    def add_blob(self, data: bytes) -> int:
        """Append raw bytes (e.g. an image) as a buffer view; returns its index."""
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": len(data)}
        self.blob += data
        return self.add("bufferViews", view)

    def write(self, path: str | Path) -> None:
        while len(self.blob) % 4:
            self.blob.append(0)
        self.doc["buffers"] = [{"byteLength": len(self.blob)}]
        js = json.dumps(self.doc, separators=(",", ":")).encode("utf-8")
        js += b" " * (-len(js) % 4)
        out = bytearray(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(self.blob)))
        out += struct.pack("<II", len(js), 0x4E4F534A) + js
        out += struct.pack("<II", len(self.blob), 0x004E4942) + self.blob
        Path(path).write_bytes(bytes(out))


def read_glb(path: str | Path) -> tuple[dict, bytes]:
    data = Path(path).read_bytes()
    magic, version, _length = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67 or version != 2:
        raise ValueError(f"{path}: not a glTF 2.0 binary")
    jlen, jtype = struct.unpack_from("<II", data, 12)
    doc = json.loads(data[20:20 + jlen])
    blen, _btype = struct.unpack_from("<II", data, 20 + jlen)
    blob = data[28 + jlen:28 + jlen + blen]
    return doc, blob


def read_accessor(doc: dict, blob: bytes, index: int) -> np.ndarray:
    acc = doc["accessors"][index]
    view = doc["bufferViews"][acc["bufferView"]]
    width = {v: k for k, v in _TYPES.items()}[acc["type"]]
    dtype = _NP[acc["componentType"]]
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    arr = np.frombuffer(blob, dtype=dtype, count=acc["count"] * width, offset=start)
    return arr.reshape(acc["count"], width) if width > 1 else arr
