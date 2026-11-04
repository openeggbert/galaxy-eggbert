"""Blender (headless) half of build_relief.py: weld, decimate to a triangle budget, smooth by angle.

    blender -b --python tools/terrain_relief/blender_decimate.py -- IN.obj OUT.obj BUDGET ANGLE

Reads and writes Wavefront OBJ (positions and triangles in; positions, normals and triangles out).
Blender's OBJ importer and exporter keep the file's axes when told forward -Z, up Y.
"""
import math
import sys

import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
src, dst, budget, angle = argv[0], argv[1], int(argv[2]), float(argv[3])
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=src, forward_axis="NEGATIVE_Z", up_axis="Y")
obj = [o for o in bpy.context.scene.objects if o.type == "MESH"][0]
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.remove_doubles(threshold=1e-5)
bpy.ops.object.mode_set(mode="OBJECT")
before = len(obj.data.polygons)
if before > budget:
    mod = obj.modifiers.new("decimate", "DECIMATE")
    mod.ratio = budget / before
    mod.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=mod.name)
bpy.ops.object.shade_smooth_by_angle(angle=math.radians(angle))
print("RELIEF_DECIMATE", before, len(obj.data.polygons))
bpy.ops.wm.obj_export(filepath=dst, forward_axis="NEGATIVE_Z", up_axis="Y", export_selected_objects=True,
                      export_uv=False, export_normals=True, export_materials=False, export_triangulated_mesh=True)
