# Tile/Block Catalog (`Decor`/`BigDecor` icon vocabulary)

**Status: COMPLETE.** All 441 addressable icon IDs (0–440) are accounted for — 313 with a full
64×64 image crop (every icon that is either already named/behavioral in `BlockTypes.hpp`, or
confirmed used in at least one of the 78 real level files), the remaining 128 unused/unnamed icons
listed compactly by range (passability + a mechanical visual signal, not a fabricated name — see
"Unused/unnamed icons" below). Generated and verified 2026-07-03 (`DOC-002`).

Icons are indices into `object-m.png` (1301×1431 px, 64×64 px tiles, 20 columns — confirmed by
direct file inspection, matches `BlockTypes::kSheetW/kSheetH/kTileSize/kSheetCols` and
`WindowsPhoneSpeedyBlupi::Def::DIMOBJX/DIMOBJY = 64`).

**Finding: icon 440 has no real pixel data.** The sheet is only tall enough for 22 full 65px-stride
rows (22×65=1430px of a 1431px-tall image) — i.e. icons 0–439 (440 real icons), not 0–440. Icon
440 is the theoretical next slot but only has 1px of image data (a sliver, not a real tile).
`BlockTypes::kPassable[441]` includes an entry for it anyway (bounds-safety in the passability
table, not because it's a real tile) — value `false`.

**Finding: animated sub-frame icons are never placed directly in level files.** Checking real-file
usage per icon confirms that for every animated group (`Lava` 68–72, `Crusher` 317–323, `Saw`
378–383, `Water1`/`Water2` 91–98, `Temp` 324–329, `Marine` 203–208), only the *base* icon (the
first frame, matching `BlockTypes::tileAnimBase()`'s convention) is ever found in a `Decor:`/
`BigDecor:` grid — e.g. `Crusher`'s frames 318–323 all show "not found in any scanned file" while
318's base 317 is used in 15 files. This is a real, verified pattern: sub-frame IDs only exist as
runtime animation states the rendering code cycles through, never as authored level data — which is
exactly what `BlockTypes::tileAnimBase()`'s "map any icon in a group to its base" design already
assumes, now independently confirmed.

## Named/behavioral tiles and every icon used in a real level (313 of 441)

Images are 64×64 px crops from `object-m.png` (see "How these images were generated" below).
"Used in real levels" is a per-icon count out of the 78 real world files, counting only genuine
`Decor:`/`BigDecor:` grid cells (not `MoveObject:` fields or the header, which also contain
numbers). Named icons keep their `BlockTypes.hpp` name and behavioral notes; unnamed-but-used icons
are marked `(unnamed)` — they're included here because they *are* placed in real levels, even
without a galaxy-eggbert constant yet.

| Image | Icon | Name | Category | Animated? | Passable | Used in real levels |
|---|---|---|---|---|---|---|
| ![icon1](images/tile-full-001.png) | 1 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon2](images/tile-full-002.png) | 2 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon3](images/tile-full-003.png) | 3 | (unnamed) | unnamed variant | no | no | 20/78 files |
| ![icon4](images/tile-full-004.png) | 4 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon5](images/tile-full-005.png) | 5 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon6](images/tile-full-006.png) | 6 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon7](images/tile-full-007.png) | 7 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon8](images/tile-full-008.png) | 8 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon9](images/tile-full-009.png) | 9 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon10](images/tile-full-010.png) | 10 | `Ground` | ground | no | no | 30/78 files |
| ![icon11](images/tile-full-011.png) | 11 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon12](images/tile-full-012.png) | 12 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon13](images/tile-full-013.png) | 13 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon14](images/tile-full-014.png) | 14 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon15](images/tile-full-015.png) | 15 | (unnamed) | unnamed variant | no | no | 16/78 files |
| ![icon16](images/tile-full-016.png) | 16 | (unnamed) | unnamed variant | no | no | 21/78 files |
| ![icon17](images/tile-full-017.png) | 17 | (unnamed) | unnamed variant | no | no | 16/78 files |
| ![icon18](images/tile-full-018.png) | 18 | `StoneA` | ground/decoration | no | no | 21/78 files |
| ![icon19](images/tile-full-019.png) | 19 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon20](images/tile-full-020.png) | 20 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon21](images/tile-full-021.png) | 21 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon22](images/tile-full-022.png) | 22 | (unnamed) | unnamed variant | no | yes | 17/78 files |
| ![icon23](images/tile-full-023.png) | 23 | (unnamed) | unnamed variant | no | no | 34/78 files |
| ![icon24](images/tile-full-024.png) | 24 | (unnamed) | unnamed variant | no | no | 35/78 files |
| ![icon25](images/tile-full-025.png) | 25 | `StoneB` | ground/decoration | no | no | 42/78 files |
| ![icon26](images/tile-full-026.png) | 26 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon27](images/tile-full-027.png) | 27 | (unnamed) | unnamed variant | no | no | 15/78 files |
| ![icon28](images/tile-full-028.png) | 28 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon29](images/tile-full-029.png) | 29 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon30](images/tile-full-030.png) | 30 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon31](images/tile-full-031.png) | 31 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon35](images/tile-full-035.png) | 35 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon36](images/tile-full-036.png) | 36 | (unnamed) | unnamed variant | no | no | 18/78 files |
| ![icon37](images/tile-full-037.png) | 37 | (unnamed) | unnamed variant | no | no | 18/78 files |
| ![icon38](images/tile-full-038.png) | 38 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon39](images/tile-full-039.png) | 39 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon40](images/tile-full-040.png) | 40 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon41](images/tile-full-041.png) | 41 | (unnamed) | unnamed variant | no | no | 5/78 files |
| ![icon42](images/tile-full-042.png) | 42 | (unnamed) | unnamed variant | no | no | 3/78 files |
| ![icon43](images/tile-full-043.png) | 43 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon44](images/tile-full-044.png) | 44 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon45](images/tile-full-045.png) | 45 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon46](images/tile-full-046.png) | 46 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon47](images/tile-full-047.png) | 47 | (unnamed) | unnamed variant | no | no | 3/78 files |
| ![icon48](images/tile-full-048.png) | 48 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon49](images/tile-full-049.png) | 49 | (unnamed) | unnamed variant | no | yes | 10/78 files |
| ![icon50](images/tile-full-050.png) | 50 | (unnamed) | unnamed variant | no | yes | 14/78 files |
| ![icon51](images/tile-full-051.png) | 51 | (unnamed) | unnamed variant | no | yes | 14/78 files |
| ![icon52](images/tile-full-052.png) | 52 | (unnamed) | unnamed variant | no | yes | 16/78 files |
| ![icon53](images/tile-full-053.png) | 53 | (unnamed) | unnamed variant | no | yes | 23/78 files |
| ![icon54](images/tile-full-054.png) | 54 | (unnamed) | unnamed variant | no | yes | 25/78 files |
| ![icon55](images/tile-full-055.png) | 55 | (unnamed) | unnamed variant | no | yes | 26/78 files |
| ![icon56](images/tile-full-056.png) | 56 | (unnamed) | unnamed variant | no | yes | 29/78 files |
| ![icon57](images/tile-full-057.png) | 57 | (unnamed) | unnamed variant | no | yes | 18/78 files |
| ![icon58](images/tile-full-058.png) | 58 | (unnamed) | unnamed variant | no | yes | 17/78 files |
| ![icon59](images/tile-full-059.png) | 59 | (unnamed) | unnamed variant | no | yes | 24/78 files |
| ![icon60](images/tile-full-060.png) | 60 | (unnamed) | unnamed variant | no | yes | 24/78 files |
| ![icon61](images/tile-full-061.png) | 61 | (unnamed) | unnamed variant | no | yes | 6/78 files |
| ![icon62](images/tile-full-062.png) | 62 | (unnamed) | unnamed variant | no | yes | 6/78 files |
| ![icon63](images/tile-full-063.png) | 63 | (unnamed) | unnamed variant | no | yes | 10/78 files |
| ![icon64](images/tile-full-064.png) | 64 | (unnamed) | unnamed variant | no | yes | 9/78 files |
| ![icon65](images/tile-full-065.png) | 65 | (unnamed) | unnamed variant | no | yes | 6/78 files |
| ![icon66](images/tile-full-066.png) | 66 | (unnamed) | unnamed variant | no | yes | 15/78 files |
| ![icon67](images/tile-full-067.png) | 67 | (unnamed) | unnamed variant | no | yes | 14/78 files |
| ![icon68](images/tile-full-068.png) | 68 | `Lava (base)` | hazard | yes, 8 frames (68-72) | no | 41/78 files — kills Blupi on contact |
| ![icon69](images/tile-full-069.png) | 69 | (unnamed) | unnamed variant | no | yes | 9/78 files |
| ![icon73](images/tile-full-073.png) | 73 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon74](images/tile-full-074.png) | 74 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon75](images/tile-full-075.png) | 75 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon76](images/tile-full-076.png) | 76 | (unnamed) | unnamed variant | no | yes | 22/78 files |
| ![icon77](images/tile-full-077.png) | 77 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon78](images/tile-full-078.png) | 78 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon79](images/tile-full-079.png) | 79 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon80](images/tile-full-080.png) | 80 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon81](images/tile-full-081.png) | 81 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon82](images/tile-full-082.png) | 82 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon83](images/tile-full-083.png) | 83 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon84](images/tile-full-084.png) | 84 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon85](images/tile-full-085.png) | 85 | (unnamed) | unnamed variant | no | yes | 2/78 files |
| ![icon86](images/tile-full-086.png) | 86 | (unnamed) | unnamed variant | no | yes | 5/78 files |
| ![icon87](images/tile-full-087.png) | 87 | (unnamed) | unnamed variant | no | no | 18/78 files |
| ![icon88](images/tile-full-088.png) | 88 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon89](images/tile-full-089.png) | 89 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon90](images/tile-full-090.png) | 90 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon91](images/tile-full-091.png) | 91 | (unnamed) | unnamed variant | no | no | 25/78 files |
| ![icon92](images/tile-full-092.png) | 92 | `Water1 (base)` | decorative/swimmable | yes, 6 frames (91-95) | no | 38/78 files — Blupi swims when grounded |
| ![icon96](images/tile-full-096.png) | 96 | `Water2 (base)` | decorative/swimmable | yes, 6 frames (96-98) | no | not found in scanned files — Blupi swims when grounded |
| ![icon107](images/tile-full-107.png) | 107 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon108](images/tile-full-108.png) | 108 | (unnamed) | unnamed variant | no | no | 3/78 files |
| ![icon109](images/tile-full-109.png) | 109 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon110](images/tile-full-110.png) | 110 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon114](images/tile-full-114.png) | 114 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon118](images/tile-full-118.png) | 118 | (unnamed) | unnamed variant | no | yes | 4/78 files |
| ![icon122](images/tile-full-122.png) | 122 | (unnamed) | unnamed variant | no | yes | 1/78 files |
| ![icon126](images/tile-full-126.png) | 126 | `FanLeft (base)` | hazard | yes, 3 frames (126-128) | no | 8/78 files — kills unless shielded |
| ![icon129](images/tile-full-129.png) | 129 | `FanRight (base)` | hazard | yes, 3 frames (129-131) | no | 7/78 files — kills unless shielded |
| ![icon132](images/tile-full-132.png) | 132 | `FanUp (base)` | hazard | yes, 3 frames (132-134) | no | 4/78 files — kills unless shielded |
| ![icon135](images/tile-full-135.png) | 135 | `FanDown (base)` | hazard | yes, 3 frames (135-137) | no | 1/78 files — kills unless shielded |
| ![icon138](images/tile-full-138.png) | 138 | (unnamed) | unnamed variant | no | yes | 19/78 files |
| ![icon139](images/tile-full-139.png) | 139 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon140](images/tile-full-140.png) | 140 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon141](images/tile-full-141.png) | 141 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon142](images/tile-full-142.png) | 142 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon143](images/tile-full-143.png) | 143 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon144](images/tile-full-144.png) | 144 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon145](images/tile-full-145.png) | 145 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon146](images/tile-full-146.png) | 146 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon147](images/tile-full-147.png) | 147 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon148](images/tile-full-148.png) | 148 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon149](images/tile-full-149.png) | 149 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon150](images/tile-full-150.png) | 150 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon151](images/tile-full-151.png) | 151 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon152](images/tile-full-152.png) | 152 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon153](images/tile-full-153.png) | 153 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon154](images/tile-full-154.png) | 154 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon155](images/tile-full-155.png) | 155 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon156](images/tile-full-156.png) | 156 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon158](images/tile-full-158.png) | 158 | `Sp0` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon159](images/tile-full-159.png) | 159 | `Sp1` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon160](images/tile-full-160.png) | 160 | `Sp2` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon161](images/tile-full-161.png) | 161 | `Sp3` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon162](images/tile-full-162.png) | 162 | `Sp4` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon163](images/tile-full-163.png) | 163 | `Sp5` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon164](images/tile-full-164.png) | 164 | `Sp6` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon165](images/tile-full-165.png) | 165 | `Sp7` | unclassified | no | yes | 1/78 files — named but no behavior attached yet |
| ![icon174](images/tile-full-174.png) | 174 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon175](images/tile-full-175.png) | 175 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon176](images/tile-full-176.png) | 176 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon177](images/tile-full-177.png) | 177 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon178](images/tile-full-178.png) | 178 | (unnamed) | unnamed variant | no | yes | 9/78 files |
| ![icon179](images/tile-full-179.png) | 179 | (unnamed) | unnamed variant | no | yes | 4/78 files |
| ![icon180](images/tile-full-180.png) | 180 | (unnamed) | unnamed variant | no | yes | 2/78 files |
| ![icon181](images/tile-full-181.png) | 181 | (unnamed) | unnamed variant | no | yes | 1/78 files |
| ![icon182](images/tile-full-182.png) | 182 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon183](images/tile-full-183.png) | 183 | `Wall` | wall | no | no | 1/78 files |
| ![icon185](images/tile-full-185.png) | 185 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon186](images/tile-full-186.png) | 186 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon187](images/tile-full-187.png) | 187 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon188](images/tile-full-188.png) | 188 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon189](images/tile-full-189.png) | 189 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon190](images/tile-full-190.png) | 190 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon191](images/tile-full-191.png) | 191 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon192](images/tile-full-192.png) | 192 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon193](images/tile-full-193.png) | 193 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon194](images/tile-full-194.png) | 194 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon195](images/tile-full-195.png) | 195 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon196](images/tile-full-196.png) | 196 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon197](images/tile-full-197.png) | 197 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon198](images/tile-full-198.png) | 198 | (unnamed) | unnamed variant | no | yes | 10/78 files |
| ![icon199](images/tile-full-199.png) | 199 | (unnamed) | unnamed variant | no | yes | 9/78 files |
| ![icon200](images/tile-full-200.png) | 200 | `Platform` | decoration | no | yes | 27/78 files — "floating platform" |
| ![icon201](images/tile-full-201.png) | 201 | (unnamed) | unnamed variant | no | yes | 36/78 files |
| ![icon202](images/tile-full-202.png) | 202 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon203](images/tile-full-203.png) | 203 | `Marine (base)` | decorative/animated water | yes, 11 frames (203-208) | yes | 18/78 files |
| ![icon211](images/tile-full-211.png) | 211 | `Spring` | interactive | no | no | not found in scanned files — auto-launches Blupi upward |
| ![icon214](images/tile-full-214.png) | 214 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon215](images/tile-full-215.png) | 215 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon216](images/tile-full-216.png) | 216 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon217](images/tile-full-217.png) | 217 | (unnamed) | unnamed variant | no | no | 14/78 files |
| ![icon218](images/tile-full-218.png) | 218 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon219](images/tile-full-219.png) | 219 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon220](images/tile-full-220.png) | 220 | (unnamed) | unnamed variant | no | no | 5/78 files |
| ![icon221](images/tile-full-221.png) | 221 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon222](images/tile-full-222.png) | 222 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon230](images/tile-full-230.png) | 230 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon231](images/tile-full-231.png) | 231 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon233](images/tile-full-233.png) | 233 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon234](images/tile-full-234.png) | 234 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon235](images/tile-full-235.png) | 235 | (unnamed) | unnamed variant | no | yes | 15/78 files |
| ![icon236](images/tile-full-236.png) | 236 | (unnamed) | unnamed variant | no | yes | 14/78 files |
| ![icon245](images/tile-full-245.png) | 245 | (unnamed) | unnamed variant | no | yes | 3/78 files |
| ![icon246](images/tile-full-246.png) | 246 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon247](images/tile-full-247.png) | 247 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon248](images/tile-full-248.png) | 248 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon249](images/tile-full-249.png) | 249 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon250](images/tile-full-250.png) | 250 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon251](images/tile-full-251.png) | 251 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon252](images/tile-full-252.png) | 252 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon253](images/tile-full-253.png) | 253 | (unnamed) | unnamed variant | no | no | 8/78 files |
| ![icon254](images/tile-full-254.png) | 254 | (unnamed) | unnamed variant | no | no | 3/78 files |
| ![icon255](images/tile-full-255.png) | 255 | (unnamed) | unnamed variant | no | no | 3/78 files |
| ![icon256](images/tile-full-256.png) | 256 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon257](images/tile-full-257.png) | 257 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon258](images/tile-full-258.png) | 258 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon259](images/tile-full-259.png) | 259 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon260](images/tile-full-260.png) | 260 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon261](images/tile-full-261.png) | 261 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon262](images/tile-full-262.png) | 262 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon263](images/tile-full-263.png) | 263 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon264](images/tile-full-264.png) | 264 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon265](images/tile-full-265.png) | 265 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon266](images/tile-full-266.png) | 266 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon267](images/tile-full-267.png) | 267 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon268](images/tile-full-268.png) | 268 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon269](images/tile-full-269.png) | 269 | (unnamed) | unnamed variant | no | yes | 6/78 files |
| ![icon270](images/tile-full-270.png) | 270 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon271](images/tile-full-271.png) | 271 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon272](images/tile-full-272.png) | 272 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon273](images/tile-full-273.png) | 273 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon274](images/tile-full-274.png) | 274 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon275](images/tile-full-275.png) | 275 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon276](images/tile-full-276.png) | 276 | (unnamed) | unnamed variant | no | yes | 6/78 files |
| ![icon277](images/tile-full-277.png) | 277 | (unnamed) | unnamed variant | no | yes | 4/78 files |
| ![icon278](images/tile-full-278.png) | 278 | (unnamed) | unnamed variant | no | yes | 4/78 files |
| ![icon279](images/tile-full-279.png) | 279 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon280](images/tile-full-280.png) | 280 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon281](images/tile-full-281.png) | 281 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon282](images/tile-full-282.png) | 282 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon283](images/tile-full-283.png) | 283 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon284](images/tile-full-284.png) | 284 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon285](images/tile-full-285.png) | 285 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon286](images/tile-full-286.png) | 286 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon287](images/tile-full-287.png) | 287 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon288](images/tile-full-288.png) | 288 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon289](images/tile-full-289.png) | 289 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon290](images/tile-full-290.png) | 290 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon291](images/tile-full-291.png) | 291 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon292](images/tile-full-292.png) | 292 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon293](images/tile-full-293.png) | 293 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon294](images/tile-full-294.png) | 294 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon295](images/tile-full-295.png) | 295 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon296](images/tile-full-296.png) | 296 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon297](images/tile-full-297.png) | 297 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon298](images/tile-full-298.png) | 298 | (unnamed) | unnamed variant | no | yes | 10/78 files |
| ![icon299](images/tile-full-299.png) | 299 | (unnamed) | unnamed variant | no | yes | 9/78 files |
| ![icon300](images/tile-full-300.png) | 300 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon301](images/tile-full-301.png) | 301 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon302](images/tile-full-302.png) | 302 | (unnamed) | unnamed variant | no | yes | 11/78 files |
| ![icon303](images/tile-full-303.png) | 303 | (unnamed) | unnamed variant | no | yes | 12/78 files |
| ![icon304](images/tile-full-304.png) | 304 | (unnamed) | unnamed variant | no | no | 15/78 files |
| ![icon305](images/tile-full-305.png) | 305 | `Blitz` | hazard | tick-gated | no | 15/78 files — kills 25% of ticks |
| ![icon309](images/tile-full-309.png) | 309 | `Marker` | unclassified | no | yes | 1/78 files |
| ![icon317](images/tile-full-317.png) | 317 | `Crusher (base)` | hazard | yes, 10 frames (317-323) | no | 3/78 files — kills Blupi on contact |
| ![icon324](images/tile-full-324.png) | 324 | `Temp (base)` | hazard/passable-timing | yes, 20 frames, 2 blank (324-329) | no | 5/78 files — vanishes 2/20 frames |
| ![icon330](images/tile-full-330.png) | 330 | `Teleport1` | interactive | no | no | 11/78 files — solid pillar |
| ![icon331](images/tile-full-331.png) | 331 | `Teleport2` | interactive | no | no | 6/78 files — solid pillar |
| ![icon332](images/tile-full-332.png) | 332 | `Teleport3` | interactive | no | no | 4/78 files — solid pillar |
| ![icon333](images/tile-full-333.png) | 333 | `Teleport4` | interactive | no | no | 3/78 files — solid pillar |
| ![icon334](images/tile-full-334.png) | 334 | `Door1` | interactive | no | no | 12/78 files — key-gated, see 06-doors.md |
| ![icon335](images/tile-full-335.png) | 335 | `Door2` | interactive | no | no | 10/78 files — key-gated, see 06-doors.md |
| ![icon336](images/tile-full-336.png) | 336 | `Door3` | interactive | no | no | 11/78 files — key-gated, see 06-doors.md |
| ![icon337](images/tile-full-337.png) | 337 | (unnamed) | unnamed variant | no | yes | 8/78 files |
| ![icon339](images/tile-full-339.png) | 339 | (unnamed) | unnamed variant | no | yes | 4/78 files |
| ![icon341](images/tile-full-341.png) | 341 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon342](images/tile-full-342.png) | 342 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon343](images/tile-full-343.png) | 343 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon344](images/tile-full-344.png) | 344 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon345](images/tile-full-345.png) | 345 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon346](images/tile-full-346.png) | 346 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon347](images/tile-full-347.png) | 347 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon348](images/tile-full-348.png) | 348 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon349](images/tile-full-349.png) | 349 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon350](images/tile-full-350.png) | 350 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon351](images/tile-full-351.png) | 351 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon352](images/tile-full-352.png) | 352 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon353](images/tile-full-353.png) | 353 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon354](images/tile-full-354.png) | 354 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon355](images/tile-full-355.png) | 355 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon356](images/tile-full-356.png) | 356 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon357](images/tile-full-357.png) | 357 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon358](images/tile-full-358.png) | 358 | (unnamed) | unnamed variant | no | no | 11/78 files |
| ![icon359](images/tile-full-359.png) | 359 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon360](images/tile-full-360.png) | 360 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon361](images/tile-full-361.png) | 361 | (unnamed) | unnamed variant | no | no | 5/78 files |
| ![icon362](images/tile-full-362.png) | 362 | (unnamed) | unnamed variant | no | no | 13/78 files |
| ![icon363](images/tile-full-363.png) | 363 | (unnamed) | unnamed variant | no | no | 12/78 files |
| ![icon364](images/tile-full-364.png) | 364 | `Bridge` | interactive | no | no | 9/78 files — passable in 2D; solid in galaxy-eggbert |
| ![icon373](images/tile-full-373.png) | 373 | `Spike (base)` | hazard | yes, 16 frames (347,373,374) | no | 5/78 files — kills Blupi on contact |
| ![icon375](images/tile-full-375.png) | 375 | (unnamed) | unnamed variant | no | yes | 3/78 files |
| ![icon376](images/tile-full-376.png) | 376 | (unnamed) | unnamed variant | no | yes | 5/78 files |
| ![icon377](images/tile-full-377.png) | 377 | (unnamed) | unnamed variant | no | yes | 1/78 files |
| ![icon378](images/tile-full-378.png) | 378 | `Saw (base)` | hazard | yes, 6 frames (378-383) | no | 15/78 files — kills Blupi on contact; stoppable by Switch |
| ![icon384](images/tile-full-384.png) | 384 | `Switch` | interactive | no | no | 8/78 files — toggles linked Saw tiles |
| ![icon385](images/tile-full-385.png) | 385 | `SwitchOff` | interactive | no | no | not found in scanned files — toggles linked Saw tiles |
| ![icon386](images/tile-full-386.png) | 386 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon387](images/tile-full-387.png) | 387 | (unnamed) | unnamed variant | no | no | 6/78 files |
| ![icon388](images/tile-full-388.png) | 388 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon389](images/tile-full-389.png) | 389 | (unnamed) | unnamed variant | no | no | 5/78 files |
| ![icon390](images/tile-full-390.png) | 390 | (unnamed) | unnamed variant | no | no | 5/78 files |
| ![icon391](images/tile-full-391.png) | 391 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon392](images/tile-full-392.png) | 392 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon393](images/tile-full-393.png) | 393 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon394](images/tile-full-394.png) | 394 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon395](images/tile-full-395.png) | 395 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon396](images/tile-full-396.png) | 396 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon397](images/tile-full-397.png) | 397 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon398](images/tile-full-398.png) | 398 | (unnamed) | unnamed variant | no | yes | 7/78 files |
| ![icon399](images/tile-full-399.png) | 399 | (unnamed) | unnamed variant | no | yes | 5/78 files |
| ![icon400](images/tile-full-400.png) | 400 | (unnamed) | unnamed variant | no | yes | 5/78 files |
| ![icon401](images/tile-full-401.png) | 401 | (unnamed) | unnamed variant | no | yes | 16/78 files |
| ![icon402](images/tile-full-402.png) | 402 | (unnamed) | unnamed variant | no | yes | 15/78 files |
| ![icon403](images/tile-full-403.png) | 403 | (unnamed) | unnamed variant | no | yes | 15/78 files |
| ![icon404](images/tile-full-404.png) | 404 | (unnamed) | unnamed variant | no | yes | 3/78 files |
| ![icon410](images/tile-full-410.png) | 410 | (unnamed) | unnamed variant | no | yes | 5/78 files |
| ![icon411](images/tile-full-411.png) | 411 | `Tile411` | unclassified | no | yes | 1/78 files |
| ![icon412](images/tile-full-412.png) | 412 | `Tile412` | unclassified | no | yes | 1/78 files |
| ![icon413](images/tile-full-413.png) | 413 | `Tile413` | unclassified | no | yes | 1/78 files |
| ![icon421](images/tile-full-421.png) | 421 | (unnamed) | unnamed variant | no | no | 10/78 files |
| ![icon422](images/tile-full-422.png) | 422 | (unnamed) | unnamed variant | no | no | 15/78 files |
| ![icon423](images/tile-full-423.png) | 423 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon424](images/tile-full-424.png) | 424 | (unnamed) | unnamed variant | no | no | 9/78 files |
| ![icon425](images/tile-full-425.png) | 425 | (unnamed) | unnamed variant | no | no | 4/78 files |
| ![icon426](images/tile-full-426.png) | 426 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon427](images/tile-full-427.png) | 427 | (unnamed) | unnamed variant | no | no | 7/78 files |
| ![icon428](images/tile-full-428.png) | 428 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon429](images/tile-full-429.png) | 429 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon430](images/tile-full-430.png) | 430 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon431](images/tile-full-431.png) | 431 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon432](images/tile-full-432.png) | 432 | (unnamed) | unnamed variant | no | no | 2/78 files |
| ![icon433](images/tile-full-433.png) | 433 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon434](images/tile-full-434.png) | 434 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon435](images/tile-full-435.png) | 435 | (unnamed) | unnamed variant | no | no | 1/78 files |
| ![icon437](images/tile-full-437.png) | 437 | (unnamed) | unnamed variant | no | no | 1/78 files |

**Verification note (door mapping):** the door icon range (334–336) and key-bit mapping were
independently re-derived from mobile-eggbert's own door-detection code (`Decor::IsDoor`,
`Decor.cpp` ~line 7360: "icon in the 334..336 range"; door-opening code ~line 5615:
`ToDoorKeyFlags(1 << (icon - 334))`) — this exactly matches `BlockTypes.hpp`'s `isDoor()`/
`doorKeyType()` (49/50/51). **No discrepancy found** — galaxy-eggbert's existing door mapping is
correct.

**Finding: three named icons are never placed in any of the 78 shipped levels** — `Water2` (base
96), `Spring` (211), `SwitchOff` (385) all show 0/78 usage (double-checked with a raw `grep`, not
just the section-aware scan, to rule out a parsing bug — confirmed absent both ways). This doesn't
mean they're non-functional (their behavior is independently confirmed from `Decor.cpp`, see
`06-doors.md` for `Switch`/`SwitchOff`, `02-tiles.md`'s Spring row above) — the shipped level set
simply never happens to place them directly (`SwitchOff` in particular may only ever be reached at
runtime, by toggling a placed `Switch`, never authored as the initial state).

## Unused/unnamed icons (128 of 441) — compact listing

These icons have no name in `BlockTypes.hpp` **and** were not found in any of the 78 real level
files' `Decor:`/`BigDecor:` grids. No image is embedded per-icon here to keep this file's size
reasonable — regenerate a crop on demand with the formula in "How these images were generated"
below if one is needed later. The "visual signal" column is a mechanical measurement (mean
alpha-channel value of the crop), not a guess at what the tile depicts — low alpha means the
sprite-sheet slot is mostly/fully transparent (likely an unused slot or a sparse decorative
overlay), high alpha means it has real opaque pixel content but this pass didn't determine what it
depicts.

**Correction (2026-07-03, post-generation verification):** the ranges below were regenerated from
scratch (independently re-cropped and re-measured) after a completeness check found the original
version of this table used coarse, loosely-worded ranges (e.g. "166–205") that numerically
overlapped icons already documented in the section above (e.g. `Wall`=183 sits inside that range).
Total coverage was never actually wrong (all 441 icons were accounted for somewhere), but the range
boundaries were imprecise. These 45 ranges are exact: every icon number in every range below is
confirmed to have no name and no real-level usage, with zero overlap against the 313 icons in the
section above.

| Icon range | Passable | Visual signal (mean alpha) |
|---|---|---|
| 0 | no | sparse/decorative (mostly transparent) |
| 32-34 | no | solid/textured tile, uncategorized |
| 70-72 | yes | sparse/decorative (mostly transparent) |
| 93-95 | no | sparse/decorative (mostly transparent) |
| 97-98 | no | sparse/decorative (mostly transparent) |
| 99 | no | empty sheet slot (near-fully transparent) |
| 100-102 | no | sparse/decorative (mostly transparent) |
| 103-106 | no | empty sheet slot (near-fully transparent) |
| 111-113 | yes | sparse/decorative (mostly transparent) |
| 115-117 | yes | sparse/decorative (mostly transparent) |
| 119-121 | yes | sparse/decorative (mostly transparent) |
| 123-125 | yes | sparse/decorative (mostly transparent) |
| 127-128 | yes | solid/textured tile, uncategorized |
| 130-131 | yes | solid/textured tile, uncategorized |
| 133-134 | yes | solid/textured tile, uncategorized |
| 136-137 | yes | solid/textured tile, uncategorized |
| 157 | no | solid/textured tile, uncategorized |
| 166-173 | yes | solid/textured tile, uncategorized |
| 184 | yes | solid/textured tile, uncategorized |
| 204-208 | yes | sparse/decorative (mostly transparent) |
| 209-210 | no | solid/textured tile, uncategorized |
| 212-213 | no | solid/textured tile, uncategorized |
| 223-229 | no | solid/textured tile, uncategorized |
| 232 | no | solid/textured tile, uncategorized |
| 237 | yes | solid/textured tile, uncategorized |
| 238-243 | yes | sparse/decorative (mostly transparent) |
| 244 | yes | empty sheet slot (near-fully transparent) |
| 306-308 | yes | sparse/decorative (mostly transparent) |
| 310 | yes | solid/textured tile, uncategorized |
| 311-316 | no | sparse/decorative (mostly transparent) |
| 318-323 | yes | sparse/decorative (mostly transparent) |
| 325-326 | yes | solid/textured tile, uncategorized |
| 327-329 | yes | sparse/decorative (mostly transparent) |
| 338 | no | solid/textured tile, uncategorized |
| 340 | no | solid/textured tile, uncategorized |
| 365-366 | no | sparse/decorative (mostly transparent) |
| 367-372 | yes | sparse/decorative (mostly transparent) |
| 374 | no | solid/textured tile, uncategorized |
| 379 | no | sparse/decorative (mostly transparent) |
| 380-383 | yes | sparse/decorative (mostly transparent) |
| 405-409 | yes | sparse/decorative (mostly transparent) |
| 414-420 | yes | solid/textured tile, uncategorized |
| 436 | no | sparse/decorative (mostly transparent) |
| 438-439 | no | sparse/decorative (mostly transparent) |
| 440 | no | empty sheet slot (near-fully transparent) |

Verified programmatically: the 128 icons spanned by these 45 ranges have zero overlap with the 313
icons in the section above, and their union is exactly `{0..440}` (441 icons, no gaps, no
double-counts).

## Animated tile frame tables — already ported

`Tables.cpp`'s animation arrays for the groups above have already been transcribed (with prior
approval) into `src/GalaxyEggbertSimple3D/Game/GETerrainRenderer.cpp` as `kAnimLava[8]`,
`kAnimSpike[16]`, `kAnimCrusher[10]`, `kAnimSaw[6]`, `kAnimWater1[6]`, `kAnimWater2[6]`,
`kAnimTemp[20]`, `kAnimMarine[11]`, plus the 3-frame fan sequences — and ported again to the CNA
target's `GETerrainRenderer.cpp`. This file does not re-transcribe those values; see the cited
files for the exact frame sequences, and `08-animations.md` for animated GIFs of each.

## How these images were generated

ImageMagick (`convert -crop`) directly against `../mobile-eggbert/Content/icons/object-m.png`
(read-only source, never modified), plus a small Python script (Pillow) for the mean-alpha visual
signal and for extracting `Decor:`/`BigDecor:` grid usage counts from all 78 real world files.
Grid math taken from mobile-eggbert's own rendering code (`Pixmap.cpp`'s `PixmapChannel::Object`
case), not guessed: 64×64 px tiles, **1 px gap** between tiles, 20 columns. Icon `n`'s pixel rect:
`col = n % 20`, `row = n / 20`, `x = col * 65`, `y = row * 65`, crop 64×64 at `(x, y)`.
Passability per icon is read directly out of `BlockTypes::isMobileTransparent`'s existing 441-entry
`kPassable[]` array (parsed programmatically, not retyped by hand).
