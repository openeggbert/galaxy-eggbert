# Every original 3D source and whether Galaxy draws it (generated)

`tools/audit/model_sources.py` (OR-0050). A scene is matched when the survey found one of its frames rendered as a game sprite at IoU >= 0.80 (object-m, element, explo) or the Blupi clip library uses it. *Matched, not converted* is the work left: sprites still drawn flat that have a scene.

| State | Scenes |
|---|---|
| in game | 162 |
| in game otherwise | 10 |
| converted, not exported | 0 |
| terrain tile (Q-004) | 129 |
| matched, not converted | 0 |
| no game sprite | 227 |

## Matched, not converted

| Scene | Sprites rendered from it |
|---|---|

## Terrain tile (q-004)

| Scene | Sprites rendered from it |
|---|---|
| `Decor/bois1.rd4` | object-m:41 |
| `Decor/bois2.rd4` | object-m:42 |
| `Decor/bois3.rd4` | object-m:43 |
| `Decor/bois4.rd4` | object-m:44 |
| `Decor/bois5.rd4` | object-m:45 |
| `Decor/bois6.rd4` | object-m:46 |
| `Decor/bois7.rd4` | object-m:47 |
| `Decor/enfant01.rd4` | object-m:215 |
| `Decor/enfant02.rd4` | object-m:216 |
| `Decor/enfant03.rd4` | object-m:79 object-m:82 object-m:84 object-m:186 object-m:194 object-m:195 object-m:217 |
| `Decor/enfant04.rd4` | object-m:34 object-m:139 object-m:140 object-m:141 object-m:284 |
| `Decor/enfant05.rd4` | object-m:219 |
| `Decor/enfant06.rd4` | object-m:78 object-m:80 object-m:81 object-m:83 object-m:193 object-m:220 |
| `Decor/enfant07.rd4` | object-m:221 |
| `Decor/enfant08.rd4` | object-m:222 |
| `Decor/enfant09.rd4` | object-m:223 |
| `Decor/enfant10.rd4` | object-m:224 |
| `Decor/enfant11.rd4` | object-m:225 |
| `Decor/enfant12.rd4` | object-m:226 |
| `Decor/enfant13.rd4` | object-m:227 |
| `Decor/enfant14.rd4` | object-m:228 |
| `Decor/enfant15.rd4` | object-m:229 |
| `Decor/enfant16.rd4` | object-m:156 object-m:237 |
| `Decor/enfant18.rd4` | object-m:232 |
| `Decor/enfant19.rd4` | object-m:233 |
| `Decor/enfant20.rd4` | object-m:234 |
| `Decor/enfant21.rd4` | object-m:235 |
| `Decor/enfant22.rd4` | object-m:236 object-m:376 object-m:421 object-m:422 object-m:423 object-m:424 object-m:425 object-m:426 object-m:427 object-m:428 object-m:429 object-m:430 (+9) |
| `Decor/fromage2.rds` | object-m:246 object-m:339 |
| `Decor/fromage3.rds` | object-m:247 object-m:340 |
| `Decor/fromage4.rds` | object-m:248 |
| `Decor/fromage5.rds` | object-m:249 |
| `Decor/grotte00.rd4` | object-m:32 object-m:33 object-m:261 object-m:262 object-m:263 object-m:283 |
| `Decor/grotte01.rd4` | object-m:285 |
| `Decor/grotte02.rd4` | object-m:286 |
| `Decor/grotte03.rd4` | object-m:287 |
| `Decor/grotte04.rd4` | object-m:288 |
| `Decor/grotte05.rd4` | object-m:289 |
| `Decor/grotte06.rd4` | object-m:290 |
| `Decor/grotte07.rd4` | object-m:291 |
| `Decor/grotte08.rd4` | object-m:292 |
| `Decor/grotte09.rd4` | object-m:293 |
| `Decor/grotte10.rd4` | object-m:294 |
| `Decor/grotte11.rd4` | object-m:295 |
| `Decor/grotte12.rd4` | object-m:296 |
| `Decor/grotte13.rd4` | object-m:297 |
| `Decor/grotte14.rd4` | object-m:298 |
| `Decor/grotte15.rd4` | object-m:299 |
| `Decor/grotte16.rd4` | object-m:300 |
| `Decor/grotte17.rd4` | object-m:142 object-m:143 object-m:187 object-m:188 object-m:189 object-m:190 object-m:191 object-m:192 object-m:218 object-m:230 object-m:301 |
| `Decor/grotte18.rd4` | object-m:302 |
| `Decor/grotte19.rd4` | object-m:303 |
| `Decor/grotte20.rd4` | object-m:337 |
| `Decor/grotte21.rd4` | object-m:338 |
| `Decor/herbe1.rd4` | object-m:107 object-m:108 object-m:109 |
| `Decor/maison07.rd4` | object-m:388 |
| `Decor/maison12.rd4` | object-m:198 object-m:400 |
| `Decor/meca10.rd4` | object-m:87 |
| `Decor/meca12.rd4` | object-m:89 |
| `Decor/meca8.rd4` | object-m:85 |
| `Decor/palais/mur06.rds` | object-m:398 |
| `Decor/roc1.rd4` | object-m:245 object-m:386 object-m:391 object-m:392 object-m:393 object-m:394 object-m:396 object-m:397 |
| `Decor/roc4a.rd4` | object-m:35 object-m:387 object-m:390 object-m:395 |
| `Decor/roc4b.rd4` | object-m:36 object-m:389 |
| `Decor/roc4c.rd4` | object-m:37 |
| `Decor/roc4d.rd4` | object-m:38 |
| `Decor/roc4e.rd4` | object-m:39 |
| `Decor/roc4f.rd4` | object-m:40 |
| `Decor/roc4g.rd4` | object-m:144 |
| `Decor/roc4h.rd4` | object-m:145 |
| `Decor/roc4i.rd4` | object-m:146 |
| `Decor/roc4j.rd4` | object-m:147 |
| `Decor/roc4k.rd4` | object-m:148 |
| `Decor/roc4l.rd4` | object-m:149 |
| `Decor/roc4m.rd4` | object-m:150 |
| `Decor/roc4n.rd4` | object-m:151 |
| `Decor/roc4o.rd4` | object-m:152 |
| `Decor/roc4p.rd4` | object-m:153 |
| `Decor/roc4q.rd4` | object-m:154 |
| `Decor/roc4r.rd4` | object-m:155 |
| `Decor/roc4s.rd4` | object-m:74 |
| `Decor/roc4t.rd4` | object-m:75 |
| `Decor/roc4u.rd4` | object-m:185 |
| `Decor/smooth/smooth-d.rds` | object-m:348 |
| `Decor/smooth/smooth-d2.rds` | object-m:363 |
| `Decor/smooth/smooth-dl.rds` | object-m:356 |
| `Decor/smooth/smooth-dlr.rds` | object-m:360 |
| `Decor/smooth/smooth-dr.rds` | object-m:355 |
| `Decor/smooth/smooth-lr.rds` | object-m:352 |
| `Decor/smooth/smooth-r.rds` | object-m:349 object-m:350 |
| `Decor/smooth/smooth-ud.rds` | object-m:351 |
| `Decor/smooth/smooth-udl.rds` | object-m:358 |
| `Decor/smooth/smooth-udlr.rds` | object-m:361 |
| `Decor/smooth/smooth-udr.rds` | object-m:357 |
| `Decor/smooth/smooth-ul.rds` | object-m:354 |
| `Decor/smooth/smooth-ulr.rds` | object-m:359 |
| `Decor/smooth/smooth-ur.rds` | object-m:353 |
| `Decor/smooth/smooth01.rds` | object-m:341 |
| `Decor/smooth/smooth03.rds` | object-m:343 |
| `Decor/smooth/smooth04.rds` | object-m:342 object-m:344 object-m:345 object-m:346 object-m:347 object-m:362 object-m:373 object-m:374 |
| `Decor/techno01.rd4` | object-m:28 |
| `Decor/techno03.rd4` | object-m:1 object-m:4 object-m:5 object-m:7 object-m:11 object-m:13 object-m:86 object-m:196 object-m:197 object-m:231 |
| `Decor/techno04.rd4` | object-m:12 |
| `Decor/techno05.rd4` | object-m:8 object-m:9 object-m:14 |
| `Decor/techno11.rd4` | object-m:2 object-m:3 object-m:48 |
| `Decor/techno12.rd4` | object-m:6 object-m:10 object-m:73 |
| `Decor/techno17.rd4` | object-m:16 |
| `Decor/techno18.rd4` | object-m:15 object-m:17 |
| `Decor/techno19.rd4` | object-m:18 |
| `Decor/techno20.rd4` | object-m:19 |
| `Decor/techno21.rd4` | object-m:20 |
| `Decor/techno22.rd4` | object-m:21 |
| `Decor/techno23.rd4` | object-m:22 |
| `Decor/techno24.rd4` | object-m:23 |
| `Decor/techno25.rd4` | object-m:24 |
| `Decor/techno26.rd4` | object-m:25 |
| `Decor/techno27.rd4` | object-m:26 |
| `Decor/techno28.rd4` | object-m:27 |
| `Decor/techno29.rd4` | object-m:29 |
| `Decor/tuyau-lr1.rds` | object-m:251 |
| `Decor/tuyau-lr2.rds` | object-m:252 |
| `Decor/tuyau-lr3.rds` | object-m:253 |
| `Decor/tuyau-ud1.rds` | object-m:254 |
| `Decor/tuyau-ud2.rds` | object-m:255 |
| `Decor/tuyau-udl.rds` | object-m:257 |
| `Decor/tuyau-udlr.rds` | object-m:250 object-m:258 |
| `Decor/tuyau-ul.rds` | object-m:259 |
| `Decor/tuyau-ulr.rds` | object-m:256 |
| `Decor/tuyau-ur.rds` | object-m:260 |

## In game otherwise

| Scene | Sprites rendered from it |
|---|---|
| `Blupi/aero-seul.rd4` | element:208 the parked overcraft: its model from Blupi's clip library (vehicles/over) |
| `Blupi/jeep-seul.rd4` | element:89 the parked jeep: its model from Blupi's clip library (vehicles/jeep) |
| `Blupi/skate.rd4` | element:129 element:130 element:131 element:132 element:133 element:134 the parked skateboard: its model from Blupi's clip library (vehicles/skate) |
| `Decor/colonne1.rds` | object-m:375 kept the sprite: a box carrying a painted column (an embedded image and mask), the column 375 reverted to it 2026-11-02 |
| `Decor/electro.rd4` | object-m:238 object-m:239 object-m:240 object-m:242 kept a billboard: the Cloud's charger (ObjectType31); the scene holds only the ball and its spring, the sprites' sparks are painted on (converted 2026-11-03: IoU 0.77-0.82, no sparks); no original or campaign world places it (the gallery does) |
| `Decor/lave/lave1.rds` | object-m:68 the lava pool from the .a3d wave (LavaSurface.hpp) |
| `Decor/lave/lave2.rds` | object-m:69 the lava pool from the .a3d wave (LavaSurface.hpp) |
| `Decor/lave/lave3.rds` | object-m:70 the lava pool from the .a3d wave (LavaSurface.hpp) |
| `Decor/lave/lave4.rds` | object-m:71 the lava pool from the .a3d wave (LavaSurface.hpp) |
| `Decor/lave/lave5.rds` | object-m:72 the lava pool from the .a3d wave (LavaSurface.hpp) |

## Converted, not exported

| Scene | Sprites rendered from it |
|---|---|

## In game

| Scene | Sprites rendered from it |
|---|---|
| `Blupi/aero-left.rd4` | blupi:303 blupi:304 blupi:305 blupi:306 blupi:307 blupi:308 blupi:309 |
| `Blupi/aero-right.rd4` | blupi:297 blupi:298 blupi:299 blupi:300 blupi:301 blupi:302 |
| `Blupi/aero-stop.rd4` | blupi:315 blupi:316 |
| `Blupi/aero-turn.rd4` | blupi:296 blupi:310 blupi:311 blupi:312 blupi:313 blupi:314 |
| `Blupi/arret-profil.rd4` | blupi:44 blupi:45 |
| `Blupi/balle-glu.rd4` | element:176 |
| `Blupi/blupi-ange.rd4` | - |
| `Blupi/blupi-ballon.rd4` | blupi:291 blupi:292 blupi:293 blupi:294 blupi:295 |
| `Blupi/blupi-bouteille.rd4` | blupi:253 blupi:254 blupi:255 blupi:256 |
| `Blupi/blupi-demi.rds` | blupi:4 blupi:134 |
| `Blupi/blupi-electro.rd4` | blupi:167 |
| `Blupi/blupi-ouf1.rd4` | blupi:222 blupi:223 blupi:224 blupi:225 |
| `Blupi/blupi-ouf2.rd4` | blupi:226 blupi:227 blupi:228 blupi:229 |
| `Blupi/blupi-ouf3.rd4` | blupi:230 blupi:231 |
| `Blupi/blupi-ouf4.rd4` | blupi:232 blupi:233 |
| `Blupi/blupi-ouf5.rd4` | blupi:288 blupi:289 blupi:290 |
| `Blupi/blupi-plat.rd4` | blupi:317 blupi:318 blupi:319 blupi:320 blupi:321 blupi:322 blupi:323 |
| `Blupi/blupi-scie.rds` | blupi:0 blupi:23 blupi:24 blupi:133 blupi:324 blupi:325 blupi:326 blupi:327 blupi:328 blupi:329 blupi:330 blupi:331 (+3) |
| `Blupi/blupi-skate-jump.rd4` | blupi:211 blupi:212 blupi:217 blupi:218 |
| `Blupi/blupi-skate-jump2.rd4` | blupi:210 blupi:213 blupi:214 blupi:215 blupi:216 blupi:219 blupi:220 blupi:221 |
| `Blupi/blupi-skate-stop.rd4` | blupi:208 blupi:209 |
| `Blupi/blupi-skate-tourne.rd4` | blupi:168 blupi:194 blupi:196 blupi:197 blupi:198 blupi:199 blupi:200 blupi:201 blupi:203 blupi:204 blupi:205 blupi:206 (+1) |
| `Blupi/blupi-skate.rd4` | blupi:182 blupi:183 blupi:184 blupi:185 blupi:186 blupi:187 blupi:188 blupi:189 blupi:190 blupi:191 blupi:192 blupi:193 |
| `Blupi/blupi-sucette.rd4` | blupi:234 blupi:235 blupi:236 |
| `Blupi/bombe-follow.rd4` | element:256 element:257 |
| `Blupi/bombe-move.rd4` | element:69 element:70 element:71 element:72 element:73 element:74 element:75 element:76 element:77 |
| `Blupi/bombe.rd4` | element:12 element:13 element:14 element:18 element:19 element:20 |
| `Blupi/bouteille.rd4` | element:178 |
| `Blupi/bye.rd4` | blupi:29 blupi:46 blupi:47 |
| `Blupi/cle.rd4` | element:122 element:123 element:124 element:125 element:126 element:127 element:128 |
| `Blupi/cle1.rd4` | element:209 element:210 element:211 element:212 element:213 element:214 element:215 |
| `Blupi/cle2.rd4` | element:216 element:217 element:218 element:219 element:220 element:221 element:222 |
| `Blupi/cle3.rd4` | element:223 element:224 element:225 element:226 element:227 element:228 element:229 |
| `Blupi/coffre.rd4` | element:0 element:1 element:2 element:3 element:4 element:5 element:6 element:7 element:8 element:9 element:10 element:11 |
| `Blupi/cogne-tete.rd4` | blupi:177 blupi:178 blupi:181 |
| `Blupi/creature1.rds` | element:247 |
| `Blupi/creature1y.rds` | element:242 element:243 element:244 element:245 element:246 |
| `Blupi/creature2.rds` | element:248 |
| `Blupi/creature3.rds` | element:249 |
| `Blupi/creature4.rds` | element:250 |
| `Blupi/creature5.rds` | element:251 |
| `Blupi/fleche.rd4` | element:29 element:30 element:33 element:34 |
| `Blupi/gagne.rd4` | blupi:41 blupi:42 blupi:43 blupi:179 |
| `Blupi/genou-profil.rd4` | blupi:2 blupi:33 blupi:34 blupi:35 |
| `Blupi/glu.rd4` | - |
| `Blupi/guepe-tourne.rd4` | element:195 element:199 element:203 element:204 element:205 element:206 element:207 |
| `Blupi/guepe.rd4` | element:196 element:197 element:200 element:201 |
| `Blupi/helico-seul2.rd4` | element:68 |
| `Blupi/helico2-glu.rd4` | blupi:74 blupi:271 blupi:272 blupi:273 |
| `Blupi/helico2.rd4` | blupi:61 blupi:62 blupi:63 blupi:64 blupi:65 blupi:66 blupi:67 blupi:68 blupi:69 blupi:70 blupi:71 blupi:72 (+1) |
| `Blupi/jeep.rd4` | blupi:110 blupi:111 blupi:112 blupi:113 blupi:114 blupi:115 blupi:116 blupi:117 |
| `Blupi/malade.rd4` | blupi:180 |
| `Blupi/marche-profil.rd4` | blupi:6 blupi:7 blupi:8 blupi:13 blupi:14 |
| `Blupi/marche-start.rd4` | blupi:5 blupi:9 blupi:10 blupi:11 |
| `Blupi/munitions-glu.rd4` | element:177 |
| `Blupi/nage.rd4` | blupi:76 blupi:92 |
| `Blupi/nage2.rd4` | blupi:39 blupi:40 blupi:77 blupi:78 blupi:79 blupi:80 blupi:81 blupi:82 blupi:83 blupi:84 blupi:85 blupi:86 (+3) |
| `Blupi/naissance.rd4` | blupi:276 blupi:277 blupi:278 blupi:279 blupi:280 blupi:281 blupi:282 blupi:283 blupi:284 |
| `Blupi/oeuf.rd4` | element:21 element:22 element:24 element:25 element:26 element:28 |
| `Blupi/oiseau.rd4` | element:100 element:101 element:102 element:103 element:114 |
| `Blupi/oups-profil.rd4` | blupi:26 blupi:27 blupi:28 blupi:30 blupi:31 blupi:32 blupi:36 blupi:169 blupi:170 blupi:171 blupi:172 blupi:173 (+3) |
| `Blupi/panneau-invert.rd4` | element:187 element:188 element:189 element:190 element:191 element:192 element:193 element:194 |
| `Blupi/pied-de-nez.rd4` | blupi:263 blupi:264 blupi:265 blupi:285 blupi:286 blupi:287 |
| `Blupi/poisson.rd4` | element:78 element:79 element:80 element:81 element:82 element:83 |
| `Blupi/pousse-profil.rd4` | blupi:49 blupi:50 blupi:51 blupi:52 blupi:53 blupi:54 blupi:55 blupi:56 blupi:57 blupi:58 blupi:59 blupi:60 |
| `Blupi/rale.rd4` | blupi:12 blupi:15 blupi:16 blupi:135 blupi:136 blupi:137 blupi:138 blupi:139 blupi:140 blupi:141 blupi:142 |
| `Blupi/saute-profil.rd4` | blupi:17 blupi:18 blupi:19 blupi:20 blupi:21 blupi:22 |
| `Blupi/stop-profil.rd4` | blupi:1 blupi:3 blupi:268 blupi:269 blupi:270 |
| `Blupi/sucette.rd4` | element:136 element:137 element:138 element:139 element:140 element:141 element:142 element:143 |
| `Blupi/surface.rd4` | blupi:93 blupi:94 blupi:95 blupi:96 blupi:97 blupi:98 blupi:99 blupi:100 blupi:101 blupi:102 blupi:103 blupi:104 (+3) |
| `Blupi/suspendu-jump.rd4` | blupi:165 blupi:166 |
| `Blupi/suspendu-singe.rd4` | blupi:162 blupi:163 blupi:164 |
| `Blupi/suspendu-stop.rd4` | blupi:156 blupi:157 blupi:158 blupi:159 blupi:160 blupi:161 |
| `Blupi/suspendu-tourne.rd4` | blupi:143 blupi:147 blupi:151 blupi:152 blupi:153 blupi:154 blupi:155 |
| `Blupi/suspendu.rd4` | blupi:144 blupi:145 blupi:146 blupi:148 blupi:149 blupi:150 |
| `Blupi/tank.rd4` | blupi:237 blupi:238 blupi:239 blupi:240 blupi:241 |
| `Blupi/tire-profil.rd4` | blupi:121 blupi:122 blupi:123 blupi:124 blupi:125 blupi:126 blupi:127 blupi:128 blupi:129 blupi:130 blupi:131 blupi:132 |
| `Blupi/tracks-profil.rd4` | element:57 |
| `Decor/aplatisseur.rd4` | object-m:317 object-m:318 object-m:319 object-m:320 |
| `Decor/barre.rd4` | object-m:138 |
| `Decor/chenille.rds` | object-m:311 object-m:312 object-m:313 object-m:314 object-m:315 object-m:316 |
| `Decor/dynamite.rd4` | element:252 element:253 element:254 element:255 |
| `Decor/eclair.rd4` | object-m:304 |
| `Decor/goutte/goutte01.rds` | object-m:404 |
| `Decor/goutte/goutte02.rds` | object-m:405 |
| `Decor/goutte/goutte03.rds` | object-m:406 |
| `Decor/goutte/goutte04.rds` | object-m:407 |
| `Decor/grotte-fragile1.rd4` | object-m:364 object-m:365 object-m:366 object-m:367 |
| `Decor/interrupteur.rd4` | object-m:384 object-m:385 |
| `Decor/palais/big0108.rds` | object-m:399 |
| `Decor/palais/big02.rds` | - |
| `Decor/palais/statue.rds` | - |
| `Decor/pile1.rd4` | object-m:377 |
| `Decor/porte-b.rd4` | object-m:336 |
| `Decor/porte-r.rd4` | object-m:334 |
| `Decor/porte-v.rd4` | object-m:335 |
| `Decor/porte1.rd4` | object-m:182 |
| `Decor/porte2.rd4` | object-m:183 |
| `Decor/ressort.rd4` | object-m:209 object-m:210 object-m:211 object-m:212 object-m:213 |
| `Decor/scie.rd4` | object-m:378 object-m:379 object-m:380 object-m:381 object-m:382 object-m:383 |
| `Decor/smooth/colonne1.rds` | - |
| `Decor/smooth/colonne2.rds` | - |
| `Decor/smooth/colonne3.rds` | - |
| `Decor/smooth/vase1.rds` | - |
| `Decor/structure1.rd4` | - |
| `Decor/support1.rd4` | object-m:76 |
| `Decor/support2.rd4` | object-m:77 |
| `Decor/support3.rd4` | object-m:199 |
| `Decor/teleporteur1.rd4` | object-m:330 object-m:331 object-m:332 object-m:333 |
| `Decor/teleporteur2.rd4` | - |
| `Decor/teleporteur3.rd4` | - |
| `Decor/teleporteur4.rd4` | - |
| `Decor/temporaire.rd4` | object-m:324 object-m:325 object-m:326 |
| `Decor/tentacule/tenta01.rds` | - |
| `Decor/tentacule/tenta02.rds` | - |
| `Decor/tentacule/tenta03.rds` | - |
| `Decor/tentacule/tenta04.rds` | - |
| `Decor/tentacule/tenta05.rds` | - |
| `Decor/tentacule/tenta06.rds` | - |
| `Decor/tentacule/tenta07.rds` | - |
| `Decor/tentacule/tenta08.rds` | - |
| `Decor/tentacule/tenta09.rds` | - |
| `Decor/tentacule/tenta10.rds` | - |
| `Decor/tentacule/tenta11.rds` | - |
| `Decor/tentacule/tenta12.rds` | - |
| `Decor/tentacule/tenta13.rds` | - |
| `Decor/tentacule/tenta14.rds` | - |
| `Decor/tentacule/tenta15.rds` | - |
| `Decor/tentacule/tenta16.rds` | - |
| `Decor/tentacule/tenta17.rds` | - |
| `Decor/ventillo-h.rd4` | object-m:132 object-m:133 object-m:134 |
| `Decor/world1.rd4` | object-m:174 object-m:175 object-m:176 object-m:177 object-m:178 object-m:179 object-m:180 object-m:181 |
| `Decor/world2a.rd4` | object-m:158 object-m:159 object-m:161 object-m:165 object-m:184 object-m:309 object-m:411 object-m:412 object-m:414 object-m:415 |
| `Decor/world2b.rd4` | - |
| `Explo/cabane.rd4` | explo:27 |
| `Explo/enfant01.rd4` | explo:36 |
| `Explo/enfant02.rd4` | explo:37 |
| `Explo/enfant03.rd4` | explo:38 |
| `Explo/enfant04.rd4` | explo:39 |
| `Explo/enfant05.rd4` | explo:40 |
| `Explo/enfant06.rd4` | explo:41 |
| `Explo/enfant07.rd4` | explo:42 |
| `Explo/enfant08.rd4` | explo:43 |
| `Explo/enfant10.rd4` | explo:44 |
| `Explo/enfant11.rd4` | explo:45 |
| `Explo/enfant12.rd4` | explo:46 |
| `Explo/enfant13.rd4` | explo:47 |
| `Explo/maison.rd4` | explo:26 |
| `Explo/meca1.rd4` | explo:28 |
| `Explo/meca2.rd4` | explo:29 |
| `Explo/meca3.rd4` | explo:30 |
| `Explo/meca4.rd4` | explo:31 |
| `Explo/plante1a.rd4` | - |
| `Explo/plante1b.rd4` | - |
| `Explo/plante1c.rd4` | - |
| `Explo/plante1d.rd4` | - |
| `Explo/plante2a.rd4` | explo:20 |
| `Explo/plante2b.rd4` | explo:21 |
| `Explo/plante2c.rd4` | explo:22 |
| `Explo/plante3a.rd4` | - |
| `Explo/plante3b.rd4` | - |
| `Explo/plante3c.rd4` | - |

## Other formats

- `.max`: 56 files, BLOCKED until 3D Studio MAX with a genuine licence (MR-0001).
- `.a3d`: 105 files, decoded (`tools/a3d/`): the lava's wave and the columns are converted; the rest are frames of the same objects.

The 227 scenes without a game sprite are backdrops, menu and editor art, revisions and tests: nothing in the game is drawn from them.
