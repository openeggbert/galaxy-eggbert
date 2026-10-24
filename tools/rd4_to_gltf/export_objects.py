#!/usr/bin/env python3
"""Turn build_object_library.py's models into the game's ``avatars3d/objects/``.

Every ``<library>/<object>_<scene>/model.glb`` becomes ``<out>/<object>_<scene>/`` through
``export_cna_avatar.py`` (CNA's SkinnedModelEXT directory), with its ``icons.txt`` beside it;
``<out>/index.txt`` lists the directories, which the game loads in that order. Directories of
``<out>`` that the library no longer has are removed.

    python3 tools/rd4_to_gltf/export_objects.py --library build-probe/rd4-objects \\
        --cna-home ../../libcna/cna --out avatars3d/objects
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import export_cna_avatar  # noqa: E402


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--library", required=True, type=Path)
    ap.add_argument("--cna-home", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args(argv)

    names = sorted(d.name for d in args.library.iterdir() if (d / "model.glb").exists())
    args.out.mkdir(parents=True, exist_ok=True)
    for stale in args.out.iterdir():
        if stale.is_dir() and stale.name not in names:
            shutil.rmtree(stale)
    for name in names:
        out = args.out / name
        if out.exists():
            shutil.rmtree(out)
        export_cna_avatar.main(["--glb", str(args.library / name / "model.glb"), "--cna-home", str(args.cna_home),
                                "--out", str(out)])
        shutil.copyfile(args.library / name / "icons.txt", out / "icons.txt")
    (args.out / "index.txt").write_text("# avatars3d/objects/<directory>/, one per line; export_objects.py\n"
                                        + "".join(f"{n}\n" for n in names))
    print(f"{len(names)} object model(s) -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
