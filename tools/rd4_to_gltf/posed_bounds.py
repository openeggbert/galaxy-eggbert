#!/usr/bin/env python3
"""A converted model's size as the game draws it: the bounds of each mesh of a skinned model.glb
(build_object_library.py's output) at a time of its clip, its vertices skinned by the clip's
pose. The accessor bounds are the bind pose, which can lie elsewhere (the clay bridge's sat 0.109
off along Z, OR-0055).

    python3 tools/rd4_to_gltf/posed_bounds.py build-probe/rd4-objects-probe/<name>/model.glb [seconds]
"""
import json, struct, sys
import numpy as np

def load(path):
    data = open(path, 'rb').read()
    n = struct.unpack('<I', data[12:16])[0]
    doc = json.loads(data[20:20 + n])
    blob = data[20 + n + 8:]
    return doc, blob

def acc(doc, blob, i):
    a = doc['accessors'][i]
    bv = doc['bufferViews'][a['bufferView']]
    ct = {5126: np.float32, 5121: np.uint8, 5123: np.uint16, 5125: np.uint32}[a['componentType']]
    nc = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
    off = bv.get('byteOffset', 0) + a.get('byteOffset', 0)
    stride = bv.get('byteStride')
    if stride and stride != nc * np.dtype(ct).itemsize:
        raw = np.frombuffer(blob, dtype=np.uint8, count=stride * a['count'], offset=off).reshape(a['count'], stride)
        return np.frombuffer(raw[:, :nc * np.dtype(ct).itemsize].tobytes(), dtype=ct).reshape(a['count'], nc)
    return np.frombuffer(blob, dtype=ct, count=a['count'] * nc, offset=off).reshape(a['count'], nc)

def quat_mat(q):
    x, y, z, w = q
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                     [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                     [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])

def node_local(node, anim):
    if 'matrix' in node:
        return np.array(node['matrix']).reshape(4, 4).T
    t = np.array(anim.get('translation', node.get('translation', [0, 0, 0])))
    r = np.array(anim.get('rotation', node.get('rotation', [0, 0, 0, 1])))
    s = np.array(anim.get('scale', node.get('scale', [1, 1, 1])))
    m = np.eye(4); m[:3, :3] = quat_mat(r) * s; m[:3, 3] = t
    return m

def main(path, time):
    doc, blob = load(path)
    nodes = doc['nodes']
    parent = {c: i for i, nd in enumerate(nodes) for c in nd.get('children', [])}
    anim = {}
    for ch in doc.get('animations', [{}])[0].get('channels', []):
        smp = doc['animations'][0]['samplers'][ch['sampler']]
        ts = acc(doc, blob, smp['input'])[:, 0]
        vs = acc(doc, blob, smp['output'])
        k = int(np.argmin(np.abs(ts - time)))
        anim.setdefault(ch['target']['node'], {})[ch['target']['path']] = vs[k]
    def glob(i):
        m = node_local(nodes[i], anim.get(i, {}))
        return glob(parent[i]) @ m if i in parent else m
    skin = doc['skins'][0]
    ibm = acc(doc, blob, skin['inverseBindMatrices']).reshape(-1, 4, 4).transpose(0, 2, 1)
    joints = [glob(j) @ ibm[k] for k, j in enumerate(skin['joints'])]
    for m in doc['meshes']:
        p = m['primitives'][0]
        pos = acc(doc, blob, p['attributes']['POSITION']).astype(float)
        jn = acc(doc, blob, p['attributes']['JOINTS_0']).astype(int)
        wt = acc(doc, blob, p['attributes']['WEIGHTS_0']).astype(float)
        if wt.max() > 1.5: wt = wt / 255.0
        hp = np.c_[pos, np.ones(len(pos))]
        out = np.zeros((len(pos), 4))
        for c in range(4):
            mats = np.array(joints)[jn[:, c]]
            out += wt[:, c:c+1] * np.einsum('nij,nj->ni', mats, hp)
        print(m['name'], 'min', out[:, :3].min(0).round(3), 'max', out[:, :3].max(0).round(3))

if __name__ == '__main__':
    main(sys.argv[1], float(sys.argv[2]) if len(sys.argv) > 2 else 0.0)
