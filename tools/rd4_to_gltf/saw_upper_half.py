#!/usr/bin/env python3
"""Clip the posed original saw at its centre, cap the cut and bake its three icon poses.

Run after export_cna_avatar/lower_spheres, before shipping the saw378_scie directory.
The source scene and icons stay unchanged; this is the owner's upper-half-only display rule.
Only baking needs numpy. --check uses the standard library and verifies the shipped result.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

CUT_Y = -0.45815  # centre of Decor/scie.rd4's converted disk, in sprite-origin space
STRIDE = 52


def read_clip(path):
    data = path.read_bytes()
    duration, count = struct.unpack_from('<di', data)
    offset, tracks = 12, {}
    for _ in range(count):
        bone, keys = struct.unpack_from('<ii', data, offset)
        offset += 8
        tracks[bone] = [struct.unpack_from('<d10f', data, offset + i * 48) for i in range(keys)]
        offset += keys * 48
    return duration, tracks


def cap_triangles(segments):
    """Closed cut contours, triangulated without filling outside their concave boundaries."""
    points, edges = {}, set()
    for a, b in segments:
        ka = tuple(round(float(v) * 100000) for v in a[[0, 2]])
        kb = tuple(round(float(v) * 100000) for v in b[[0, 2]])
        if ka == kb:
            continue
        points.setdefault(ka, a)
        points.setdefault(kb, b)
        edge = tuple(sorted((ka, kb)))
        if edge in edges:
            edges.remove(edge)
        else:
            edges.add(edge)
    neighbours = {}
    for a, b in edges:
        neighbours.setdefault(a, set()).add(b)
        neighbours.setdefault(b, set()).add(a)
    assert all(len(n) == 2 for n in neighbours.values()), 'open/ambiguous saw cut contour'

    def cross(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])

    triangles = []
    while edges:
        start, at = min(edges)
        loop, previous = [start], start
        edges.remove(tuple(sorted((start, at))))
        while at != start:
            loop.append(at)
            following = next(v for v in sorted(neighbours[at]) if v != previous)
            edges.remove(tuple(sorted((at, following))))
            previous, at = at, following
        # Ignore collinear points introduced by triangulated side walls.
        changed = True
        while changed and len(loop) > 3:
            changed = False
            for i in range(len(loop)):
                if abs(cross(loop[i-1], loop[i], loop[(i+1) % len(loop)])) < 2:
                    loop.pop(i)
                    changed = True
                    break
        area = sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(loop, loop[1:]+loop[:1]))
        if area < 0:
            loop.reverse()
        while len(loop) > 3:
            for i, b in enumerate(loop):
                a, c = loop[i-1], loop[(i+1) % len(loop)]
                if cross(a, b, c) <= 0:
                    continue
                inside = lambda p: cross(a,b,p) >= 0 and cross(b,c,p) >= 0 and cross(c,a,p) >= 0
                if any(inside(p) for p in loop if p not in (a,b,c)):
                    continue
                triangles.append((points[a], points[b], points[c]))
                loop.pop(i)
                break
            else:
                raise ValueError('cannot triangulate saw cut contour')
        triangles.append(tuple(points[p] for p in loop))
    return triangles


def bake(directory):
    import numpy as np
    from posed_bounds import quat_mat

    manifest_path = directory / 'avatar.skinnedmodel.json'
    manifest = json.loads(manifest_path.read_text())
    if all(p['name'].startswith('saw_upper_') for p in manifest['parts']):
        return check(directory)
    times = sorted({float(line.split()[3]) for line in (directory/'icons.txt').read_text().splitlines()
                    if line.startswith('icon ')})
    data = (directory/manifest['skeleton']).read_bytes()
    bones = struct.unpack_from('<i', data)[0]
    parents = struct.unpack_from('<'+'i'*bones, data, 4)
    bind = np.frombuffer(data, '<f4', bones*16, 4+bones*4).reshape(bones,4,4).copy()
    inverse = np.frombuffer(data, '<f4', bones*16, 4+bones*4+bones*64).reshape(bones,4,4)
    _, tracks = read_clip(directory/'clips/Frames.clip.bin')
    original_parts, parts, bounds = manifest['parts'], [], []
    provenance = {'version': 1, 'cutY': CUT_Y, 'times': times, 'sourceHashes': {}}
    for path in [directory/manifest['skeleton'], directory/'clips/Frames.clip.bin',
                 *[directory/p[k] for p in original_parts for k in ('vertices','indices')]]:
        provenance['sourceHashes'][str(path.relative_to(directory))] = hashlib.sha256(path.read_bytes()).hexdigest()

    for pose, time in enumerate(times):
        local = bind.copy()
        for bone, keys in tracks.items():
            key = min(keys, key=lambda k: abs(k[0]-time))
            assert abs(key[0]-time) < 1e-6, 'saw icon must use a source keyframe'
            matrix = np.eye(4)
            matrix[:3,:3] = np.diag(key[8:11]) @ quat_mat(key[4:8]).T
            matrix[3,:3] = key[1:4]
            local[bone] = matrix
        global_matrices = []
        for bone in range(bones):
            global_matrices.append(local[bone] if parents[bone]<0 else local[bone] @ global_matrices[parents[bone]])
        skin = np.array([inverse[i] @ global_matrices[i] for i in range(bones)])
        for part_index, part in enumerate(original_parts):
            raw = (directory/part['vertices']).read_bytes()
            vertices = []
            for offset in range(0,len(raw),STRIDE):
                pos = np.array([*struct.unpack_from('<3f',raw,offset),1])
                normal = np.array([*struct.unpack_from('<3f',raw,offset+12),0])
                weights = struct.unpack_from('<4f',raw,offset+32)
                joints = struct.unpack_from('<4B',raw,offset+48)
                matrix = sum(weights[i]*skin[joints[i]] for i in range(4))
                p, n = pos@matrix, normal@matrix
                n = n[:3]/max(1e-12,np.linalg.norm(n[:3]))
                vertices.append(np.r_[p[:3],n,struct.unpack_from('<2f',raw,offset+24)])
            index_data = (directory/part['indices']).read_bytes()
            indices = struct.unpack('<'+'H'*(len(index_data)//2),index_data)
            triangles, segments, winding = [], [], 0.0
            for at in range(0,len(indices),3):
                tri = [vertices[i].copy() for i in indices[at:at+3]]
                winding += float(np.dot(np.cross(tri[1][:3]-tri[0][:3],tri[2][:3]-tri[0][:3]),sum(v[3:6] for v in tri)))
                polygon, cut = [], []
                for a,b in zip(tri,tri[1:]+tri[:1]):
                    if a[1] >= CUT_Y:
                        polygon.append(a)
                    if (a[1] >= CUT_Y) != (b[1] >= CUT_Y):
                        p = a+(b-a)*((CUT_Y-a[1])/(b[1]-a[1]))
                        p[1] = CUT_Y
                        p[3:6] /= max(1e-12,np.linalg.norm(p[3:6]))
                        polygon.append(p)
                        cut.append(p[:3].copy())
                if len(cut)==2:
                    segments.append(tuple(cut))
                for i in range(1,len(polygon)-1):
                    triangles.append((polygon[0],polygon[i],polygon[i+1]))
            for a,b,c in cap_triangles(segments):
                cap = [np.r_[p,[0,-1,0],[.5,.5]] for p in (a,b,c)]
                if float(np.dot(np.cross(b-a,c-a),[0,-1,0])) * winding < 0:
                    cap[1],cap[2] = cap[2],cap[1]
                triangles.append(tuple(cap))
            packed, lookup, out_indices = [], {}, []
            for tri in triangles:
                for v in tri:
                    v = v.copy()
                    v[1] = max(0.0,v[1]-CUT_Y)
                    value = struct.pack('<3f3f2f4f4B',*v,1,0,0,0,pose,0,0,0)
                    if value not in lookup:
                        lookup[value] = len(packed)
                        packed.append(value)
                    out_indices.append(lookup[value])
            assert len(packed)<65536
            name = f'saw_upper_{pose}_{part_index}'
            (directory/f'{name}.verts.bin').write_bytes(b''.join(packed))
            (directory/f'{name}.idx.bin').write_bytes(struct.pack('<'+'H'*len(out_indices),*out_indices))
            parts.append({**part,'name':name,'vertices':f'{name}.verts.bin','indices':f'{name}.idx.bin'})
            ys = [struct.unpack_from('<f',v,4)[0] for v in packed]
            bounds.append([min(ys),max(ys)])

    count = len(times)
    identity = struct.pack('<16f',1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
    (directory/'skeleton.bin').write_bytes(struct.pack('<i',count)+struct.pack('<'+'i'*count,*([-1]*count))+identity*(count*2))
    clip = bytearray(struct.pack('<di',max(times)+.05,count))
    for bone in range(count):
        keys = [(0.0,0),*[(t,i) for i,t in enumerate(times)]]
        clip.extend(struct.pack('<ii',bone,len(keys)))
        for time,active in keys:
            scale = 1.0 if active==bone else 0.0
            clip.extend(struct.pack('<d10f',time,0,0,0,0,0,0,1,scale,scale,scale))
    (directory/'clips/Frames.clip.bin').write_bytes(clip)
    manifest['parts'] = parts
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    for part in original_parts:
        for key in ('vertices','indices'):
            (directory/part[key]).unlink()
    provenance['boundsY'] = bounds
    (directory/'upper-half.json').write_text(json.dumps(provenance,indent=2)+'\n')
    return check(directory)


def check(directory):
    manifest = json.loads((directory/'avatar.skinnedmodel.json').read_text())
    info = json.loads((directory/'upper-half.json').read_text())
    _, tracks = read_clip(directory/'clips/Frames.clip.bin')
    assert info['version']==1 and info['times']==[.05,.25,.3] and len(tracks)==len(info['times'])
    cap_vertices = 0
    for part in manifest['parts']:
        assert part['name'].startswith('saw_upper_') and part['vertexStride']==STRIDE
        raw = (directory/part['vertices']).read_bytes()
        assert raw and len(raw)%STRIDE==0
        pose = int(part['name'].split('_')[2])
        part_caps = 0
        for offset in range(0,len(raw),STRIDE):
            values = struct.unpack_from('<3f3f2f4f4B',raw,offset)
            assert all(math.isfinite(v) for v in values)
            assert 0<=values[1]<.49, 'hidden lower half leaked, or wrong half/scale'
            assert values[8:12]==(1,0,0,0) and values[12]==pose
            if values[1]==0 and values[4]<-.99:
                cap_vertices+=1
                part_caps+=1
        assert part_caps>0, 'every pose must have a closed cut base'
        indices = (directory/part['indices']).read_bytes()
        assert indices and len(indices)%6==0
        assert max(struct.unpack('<'+'H'*(len(indices)//2),indices))<len(raw)//STRIDE
        for active,time in enumerate(info['times']):
            for bone,keys in tracks.items():
                key = next(k for k in keys if abs(k[0]-time)<1e-8)
                assert key[8:11]==((1,1,1) if bone==active else (0,0,0))
    assert cap_vertices>0, 'cut base must be closed from below'
    print(f'{directory}: upper-half saw verified ({len(info["times"])} poses, closed cut base)')
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    raise SystemExit(check(args.directory) if args.check else bake(args.directory))
