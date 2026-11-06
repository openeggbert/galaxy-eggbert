#!/usr/bin/env python3
"""usage: slice.py x0 x1 z0 z1 y [y ...]  -> ASCII plan of the probe world (. air, # block, o object, ~ water/lava)"""
import sys
from pathlib import Path
sys.path.insert(0, '/rv/data/development/github.com/openeggbert/galaxy-eggbert/tools')
import vwr_inspect
wd = vwr_inspect.read_world(Path('/rv/data/development/github.com/openeggbert/galaxy-eggbert/build-probe/world199-rebuild/worlds/world199.vwr'))
b = wd['blocks']
objs = {}
for o in wd['objects']:
    c = tuple(int(v // 1) for v in o['start'])
    objs[c] = o['type']
x0, x1, z0, z1 = map(int, sys.argv[1:5])
for y in map(int, sys.argv[5:]):
    print("y", y)
    for z in range(z0, z1 + 1):
        row = ""
        for x in range(x0, x1 + 1):
            t = b.get((x, y, z), 0) >> 4
            ch = "." if t == 0 else ("~" if t in (68, 91, 92) else "#")
            if (x, y, z) in objs:
                ch = "o"
            row += ch
        print(row, z)
