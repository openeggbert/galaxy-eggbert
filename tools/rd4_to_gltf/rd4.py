"""Reader for Ray Dream Designer 3 / Studio 4 ASCII scene files (``3DC { asci 1 ...``).

The format is a tree of tagged chunks, one item per line:

    tag {          opens a chunk
    tag value      a leaf; the value is the rest of the line
    }              closes the innermost chunk

Indentation is cosmetic and not reliable (some chunks restart at column 0).
Text is Latin-1 with CRLF line endings (plus stray bare CRs, see parse_text).
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Chunk:
    tag: str
    items: list = field(default_factory=list)  # (tag, str) leaves and Chunk children, in file order

    def chunks(self, tag: str | None = None) -> list["Chunk"]:
        return [c for c in self.items if isinstance(c, Chunk) and (tag is None or c.tag == tag)]

    def chunk(self, tag: str) -> "Chunk | None":
        for c in self.items:
            if isinstance(c, Chunk) and c.tag == tag:
                return c
        return None

    def leaves(self, tag: str) -> list[str]:
        return [v for c in self.items if isinstance(c, tuple) and c[0] == tag for v in (c[1],)]

    def leaf(self, tag: str, default: str | None = None) -> str | None:
        for c in self.items:
            if isinstance(c, tuple) and c[0] == tag:
                return c[1]
        return default

    def string(self, tag: str, default: str | None = None) -> str | None:
        v = self.leaf(tag)
        if v is None:
            return default
        return unquote(v)

    def floats(self, tag: str) -> list[float] | None:
        v = self.leaf(tag)
        return None if v is None else parse_floats(v)

    def float(self, tag: str, default: float | None = None) -> float | None:
        v = self.leaf(tag)
        return default if v is None else float(v)


def unquote(v: str) -> str:
    v = v.strip()
    if len(v) >= 2 and v[0] == '"' and v[-1] == '"':
        return v[1:-1]
    return v


def parse_floats(v: str) -> list[float]:
    return [float(x) for x in v.split(",") if x.strip()]


# Leaves whose value is "$" + 8 hex digits of length, a CR, that many raw bytes (which may contain
# CR and LF) and a closing "$" (the older lowercase bitmap chunks, e.g. t.rd4's tdta/tile, ctab).
BINARY_TAGS = {"tile", "ctab"}
_BINARY = re.compile(r"[ \t]*(tile|ctab) \$([0-9A-Fa-f]{8})")


def parse_text(text: str) -> Chunk:
    root = Chunk("<root>")
    stack = [root]
    # Lines end in CRLF; a bare CR also appears inside TILE values (after the $FFFFFFFF tile
    # header), so split on LF only and drop every CR.
    pos = 0
    while pos < len(text):
        end = text.find("\n", pos)
        if end < 0:
            end = len(text)
        binary = _BINARY.match(text, pos)
        if binary is not None:
            start = binary.end() + (text[binary.end():binary.end() + 1] == "\r")
            length = int(binary.group(2), 16)
            stack[-1].items.append((binary.group(1), "$" + binary.group(2) + text[start:start + length]))
            end = text.find("\n", start + length)
            pos = len(text) if end < 0 else end + 1
            continue
        raw = text[pos:end]
        pos = end + 1
        line = raw.replace("\r", "").strip()
        if not line:
            continue
        if line == "}":
            if len(stack) == 1:
                raise ValueError("unbalanced '}'")
            stack.pop()
            continue
        if line.endswith("{") and '"' not in line:
            child = Chunk(line[:-1].strip())
            stack[-1].items.append(child)
            stack.append(child)
            continue
        parts = line.split(None, 1)
        stack[-1].items.append((parts[0], parts[1] if len(parts) > 1 else ""))
    return root


def parse_file(path: str | Path) -> Chunk:
    return parse_text(Path(path).read_bytes().decode("latin-1"))
