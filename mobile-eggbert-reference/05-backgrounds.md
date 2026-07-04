# Backgrounds / Sky Regions

**Status: `region=` → filename mapping RESOLVED (`DOC-247`, `DOC-248`); 3 of 38 background images
shown so far, batch thumbnailing in progress.** Tracked as `DOC-006` in `plan.md`.

`../mobile-eggbert/Content/backgrounds/` contains 38 images total: 28 level backgrounds
(`decor000.png` through `decor031.png`, non-contiguous — ids 005, 014, 017, 023 are missing from
the set) plus **10** non-level UI-screen backgrounds — `init.png`, `lost.png`, `pause.png`,
`setup.png`, `speedyblupi.png`, `trial.png`, `wait.png`, `win.png`, `blupiyoupie.png`, `gear.png`.
**Correction: earlier drafts of this file only mentioned `blupiyoupie.png`/`gear.png`** — there are
8 more UI-screen files, confirmed below.

## The `region=` → filename mapping (`DOC-247`, resolved)

The mapping is **direct, not a lookup table**: `Decor::LoadImages()` (`Decor.cpp` ~line 244) builds
the filename straight from the level's region number —

```cpp
oss << "decor" << std::setw(3) << std::setfill('0') << m_region;   // e.g. region=7 -> "decor007"
m_pixmap->BackgroundCache(name);
```

— and `m_region` is read directly from the level file's `region=` field
(`Worlds::GetIntField(..., "region")`, `Decor.cpp` ~line 11323). There is no indirection through
world number, level index, or any other selector — `region=N` always means `decorNNN.png`, full
stop. This closes the single most important open question this file previously flagged.

**Cross-check against all 78 real level files (`DOC-248`):** extracted every `region=` value from
`../mobile-eggbert/worlds/*.txt` (28 distinct values used: 0-4, 6-13, 15, 16, 18-22, 24-31) and
confirmed every single one has a real, existing `decorNNN.png` — zero missing backgrounds are ever
actually referenced by a real level. The 4 "missing" ids (005, 014, 017, 023) are exactly the ones
no level ever uses; not a coincidence, and not something to fill in.

**The 10 non-level UI-screen backgrounds are loaded the same way, by literal name, not a region
number** — `Pixmap::BackgroundCache("wait"/"init"/"pause"/"lost"/"win"/"setup"/"trial")` is called
directly from `Game1.cpp`'s screen-state switch (not gameplay code); `speedyblupi.png` (title
screen) and `blupiyoupie.png`/`gear.png` are loaded elsewhere by the same mechanism. None of these
10 ever go through `Decor::LoadImages()`'s `region=` path.

**What galaxy-eggbert currently does instead:** `GEWorldRuntime` parses `region=` into `skyRegion_`
but the Simple3D target does not use it — it instead picks one of 5 hardcoded flat sky colors
indexed by **world number** (1–5, "Grassland/Forest/Ice Caves/Lava Fields/Space Station"), explicitly
marked `TODO(S3D-sky)` as a rough proxy since Simple3D lacks a fog/zone API. No real background
*image* (parallax `decorNNN.png`) is loaded by either galaxy-eggbert target today.

Thumbnails (resized to 240×180, same method as `DOC-233`: plain `convert -resize`, not a
sprite-sheet crop, so unaffected by the `object-m.png` leading-margin bug):

![decor000](images/bg-decor000.png)
![decor001](images/bg-decor001.png)
![decor002](images/bg-decor002.png)
![decor003](images/bg-decor003.png)
![decor004](images/bg-decor004.png)
![decor006](images/bg-decor006.png)
![decor007](images/bg-decor007.png)
![decor008](images/bg-decor008.png)
![decor009](images/bg-decor009.png)
![decor010](images/bg-decor010.png)
![decor011](images/bg-decor011.png)
![decor012](images/bg-decor012.png)
![decor013](images/bg-decor013.png)
![decor015](images/bg-decor015.png)
![decor016](images/bg-decor016.png)
![decor018](images/bg-decor018.png)
![decor019](images/bg-decor019.png)
![decor020](images/bg-decor020.png)
![decor021](images/bg-decor021.png)
![decor022](images/bg-decor022.png)

**`DOC-233`/`DOC-249`-`DOC-254` (2026-07-04):** `decor000`-`decor002` re-verified (`DOC-233`);
`decor003` (`DOC-249`), `decor004`/`006`/`007` (`DOC-250`, skipping missing id `005`),
`decor008`-`011` (`DOC-251`), `decor012`/`013`/`015` (`DOC-252`, skipping missing id `014`),
`decor016`/`018`/`019` (`DOC-253`, skipping missing id `017`), and `decor020`-`022` (`DOC-254`,
skipping missing id `023`) added. All 20 confirmed pixel-exact (`compare -metric AE`=0) against a
fresh `convert decorNNN.png -resize 240x180`. `decor015` is a genuinely grayscale image (not a
bug) — a stone-cave background.
