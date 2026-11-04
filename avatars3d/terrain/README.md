# Terrain reliefs (Q-001/Q-004/Q-012)

Each `.gtr` is a terrain tile's original Ray Dream scene (`../speedyblupi-data/originals/Decor/`) as
the relief the game lays on a cell's exposed faces near the camera (`TerrainRelief.hpp`,
`TerrainRenderer::UpdateRelief`): the scene's front half, decimated to about 700 triangles, cut
along the cell's edges and textured by the tile's sprite projected from the front. `index.txt` maps
the tile icons to them. Generated; do not edit by hand:

```bash
python3 tools/terrain_relief/build_relief.py --library build-probe/rd4-terrain \
    --write-objects build-probe/rd4-terrain/objects-r.txt
python3 tools/rd4_to_gltf/build_object_library.py --originals ../speedyblupi-data/originals \
    --sheets ../mobile-eggbert/Content/icons --objects build-probe/rd4-terrain/objects-r.txt \
    --out build-probe/rd4-terrain
python3 tools/terrain_relief/build_relief.py --library build-probe/rd4-terrain --out avatars3d/terrain
```

Which tile is which scene is `tools/terrain_relief/tiles.txt`, reviewed by eye from the ranking of
`tools/terrain_relief/match_tiles.py`. Tiles without a relief stay textured cubes.
