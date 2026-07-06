# Otázky ke zbývajícím dlaždicím (třetí kolo, nepojmenované/nepoužité ikony)

**Účel:** stejné jako `questionnaire-unidentified-tiles.md` (kolo 1, 34 ikon) a
`questionnaire-all-remaining-tiles.md` (kolo 2, 280 ikon), ale tentokrát pro ikony, které
`02-tiles.md` eviduje jako "nepojmenované/nepoužité" (žádné jméno v `BlockTypes.hpp`, nulové
použití ve 78 skenovaných level souborech) — sekce "Unused/unnamed icons (128 of 441)". Kola 1+2
dohromady pokryla všech 314 pojmenovaných/chovajících se dlaždic; toto kolo je nad rámec toho, na
uživatelovo explicitní přání (2026-07-08) i přesto, že tyto ikony jsou dokumentovaně nepoužité.

**Vynechané ikony (nepotřebují vlastní odpověď):**
- Ikona 0 = `Air` (prázdno/žádný blok) — není skutečná viditelná dlaždice.
- Ikona 440 — potvrzeno, že nemá žádná reálná pixelová data (sheet končí na indexu 439).
- Animační sub-snímky již identifikovaných objektů (dědí odpověď ze základního snímku, nemá cenu
  se ptát znovu): Lava 70-72 (viz ikona 68/69), Fan 127-128/130-131/133-134/136-137 (viz ikony
  126/129/132/135), Crusher 318-323 (viz ikona 317), Temp 325-329 (viz ikona 324), Saw 380-383
  (viz ikona 378), Water2 97-98 (viz ikony 91/96 — render mód zatím otevřená otázka, task "Decide
  water tile render mode").

**Oprava (2026-07-08):** tvrzení "nepoužité/0-78 souborů" bylo založeno jen na skenu statické
`Decor:`/`BigDecor:` mřížky — nezahrnovalo `MoveObject:` řádky, které mají vlastní `icon=` pole a
mohou odkazovat do stejného `object-m.png` sheetu (`PixmapChannel::Object`). Křížovou kontrolou
`Decor.cpp`/`Tables.cpp` bylo potvrzeno reálné použití u ikon 32-34 (bedny/`ObjectType12`),
99-102+244 (šplouchnutí vody), 103-106 (bublinky Blupiho), 238-243 (Charge pickup), 311-316
(chenille/běžící pás), 365-372 (konstrukce mostu) — viz `02-tiles.md`'s "Unused/unnamed icons"
sekce pro plné detaily. Všechny tyto identity **odpovídají** odpovědím, které jsi níže dal ještě
předtím, než byla tato oprava nalezena — render módy zůstávají v platnosti beze změny.

Vyplň odpovědi za `Odpověď:` u každé dlaždice, kterou poznáš — klidně napiš "nevím" nebo přeskoč ty,
u kterých si nejsi jistý (jsou to nepoužité sloty, takže "nevím/nepoužité" je zcela validní
odpověď). Sloupec "Vizuální signál" níže je mechanické měření (průměrná alpha), ne odhad obsahu —
nízká hodnota znamená, že slot je většinou/úplně průhledný.

## Co rozlišujeme (render mód)

- **`UniformCube`** (krychle) — textura na všech 6 stranách. Výchozí varianta pro hromadný materiál.
- **`Billboard`** — plochá tabule/sprite, vždy otočená čelem ke kameře.
- **`DirectionalCube`** — krychle, textura jen na některých stranách, zbylé strany mají plnou barvu
  nebo jsou průhledné (řekni na kolika/kterých stranách a jakou barvu mají zbylé).
- **`InnerPillarBox`** — vnější krychle průhledná, uvnitř menší kvádr/sloup s texturou na jedné
  nebo více stranách.
- **`InnerFlatPlate`** — deska uprostřed průhledné krychle, textura na jedné nebo obou stranách.
- **`TripleCrossBillboard`** — stejná textura 3× pod úhlem 60°, tvoří v půdorysu trojúhelník.
- **speciální povrch** — voda, rostlina — plochý povrch, ani krychle, ani billboard.
- **jiná/nová geometrie** — pokud tvar neodpovídá ničemu výše, popiš ho vlastními slovy.
- **prázdné/nepoužité** — pokud je slot skutečně jen prázdný/nevyužitý grafický zbytek, napiš to —
  je to nejpravděpodobnější odpověď u většiny těchto ikon.

U každé dlaždice napiš, prosím:
1. **Co to podle tebe je** (vlastními slovy, nebo "nevím"/"nepoužité")
2. **Jaký render mód by měl mít** (pokud vůbec nějaký)

---

## Icon 32

![icon32](images/tile-full-032.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: bedny

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 33

![icon33](images/tile-full-033.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: bedny

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 34

![icon34](images/tile-full-034.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: bedny

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 93

![icon93](images/tile-full-093.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 94

![icon94](images/tile-full-094.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 95

![icon95](images/tile-full-095.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 99

![icon99](images/tile-full-099.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: šplouchnutí vody

**Render mód?**
Odpověď: Billboard

---

## Icon 100

![icon100](images/tile-full-100.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: šplouchnutí vody

**Render mód?**
Odpověď: Billboard

---

## Icon 101

![icon101](images/tile-full-101.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: šplouchnutí vody

**Render mód?**
Odpověď: Billboard

---

## Icon 102

![icon102](images/tile-full-102.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: šplouchnutí vody

**Render mód?**
Odpověď: Billboard

---

## Icon 103

![icon103](images/tile-full-103.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: bublinky od Blupiho, když je ve vodě a dýchá

**Render mód?**
Odpověď: Billboard

---

## Icon 104

![icon104](images/tile-full-104.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: bublinky od Blupiho, když je ve vodě a dýchá

**Render mód?**
Odpověď: Billboard

---

## Icon 105

![icon105](images/tile-full-105.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: bublinky od Blupiho, když je ve vodě a dýchá

**Render mód?**
Odpověď: Billboard

---

## Icon 106

![icon106](images/tile-full-106.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: bublinky od Blupiho, když je ve vodě a dýchá

**Render mód?**
Odpověď: Billboard

---

## Icon 111

![icon111](images/tile-full-111.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 112

![icon112](images/tile-full-112.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 113

![icon113](images/tile-full-113.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 115

![icon115](images/tile-full-115.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 116

![icon116](images/tile-full-116.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 117

![icon117](images/tile-full-117.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 119

![icon119](images/tile-full-119.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 120

![icon120](images/tile-full-120.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 121

![icon121](images/tile-full-121.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 123

![icon123](images/tile-full-123.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 124

![icon124](images/tile-full-124.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 125

![icon125](images/tile-full-125.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: vítr od větráku (stejné jako ikony 110/114/118/122)

**Render mód?**
Odpověď: NENÍ klasický Billboard — je statické. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách (stejně jako 110/114/118/122)

---

## Icon 157

![icon157](images/tile-full-157.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: hrouda země, samostatná (bez sousedních), nahoře tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 166

![icon166](images/tile-full-166.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 167

![icon167](images/tile-full-167.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 168

![icon168](images/tile-full-168.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 169

![icon169](images/tile-full-169.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 170

![icon170](images/tile-full-170.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 171

![icon171](images/tile-full-171.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 172

![icon172](images/tile-full-172.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 173

![icon173](images/tile-full-173.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 184

![icon184](images/tile-full-184.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro teleportaci do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 204

![icon204](images/tile-full-204.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 205

![icon205](images/tile-full-205.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 206

![icon206](images/tile-full-206.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 207

![icon207](images/tile-full-207.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 208

![icon208](images/tile-full-208.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: Billboard

---

## Icon 209

![icon209](images/tile-full-209.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: pružina (Spring)

**Render mód?**
Odpověď: Billboard

---

## Icon 210

![icon210](images/tile-full-210.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: pružina (Spring)

**Render mód?**
Odpověď: Billboard

---

## Icon 212

![icon212](images/tile-full-212.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: pružina (Spring)

**Render mód?**
Odpověď: Billboard

---

## Icon 213

![icon213](images/tile-full-213.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: pružina (Spring)

**Render mód?**
Odpověď: Billboard

---

## Icon 223

![icon223](images/tile-full-223.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: blok krychle s dětským motivem

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 224

![icon224](images/tile-full-224.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 225

![icon225](images/tile-full-225.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora i zdola plná barva (oranžový odstín)

---

## Icon 226

![icon226](images/tile-full-226.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora zelená barva, zdola fialová barva

---

## Icon 227

![icon227](images/tile-full-227.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora zelená barva, zdola fialová barva

---

## Icon 228

![icon228](images/tile-full-228.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora modrá barva, zdola fialová barva

---

## Icon 229

![icon229](images/tile-full-229.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 232

![icon232](images/tile-full-232.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: krychle

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (modrá), zdola plná barva (žlutá)

---

## Icon 237

![icon237](images/tile-full-237.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: nevím

**Render mód?**
Odpověď: nevím

---

## Icon 238

![icon238](images/tile-full-238.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 239

![icon239](images/tile-full-239.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 240

![icon240](images/tile-full-240.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 241

![icon241](images/tile-full-241.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 242

![icon242](images/tile-full-242.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 243

![icon243](images/tile-full-243.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: (nejisté)

**Render mód?**
Odpověď: Billboard

---

## Icon 244

![icon244](images/tile-full-244.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): empty sheet slot (near-fully transparent)

**Co to je?**
Odpověď: kapka vody

**Render mód?**
Odpověď: Billboard

---

## Icon 306

![icon306](images/tile-full-306.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: blesk (Blitz)

**Render mód?**
Odpověď: Billboard

---

## Icon 307

![icon307](images/tile-full-307.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: blesk (Blitz)

**Render mód?**
Odpověď: Billboard

---

## Icon 308

![icon308](images/tile-full-308.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: blesk (Blitz)

**Render mód?**
Odpověď: Billboard

---

## Icon 310

![icon310](images/tile-full-310.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 311

![icon311](images/tile-full-311.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 312

![icon312](images/tile-full-312.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 313

![icon313](images/tile-full-313.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 314

![icon314](images/tile-full-314.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 315

![icon315](images/tile-full-315.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 316

![icon316](images/tile-full-316.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: plošina, běžící pás (caterpillar/chenille tread — potvrzeno reálně použito přes MoveObject icon=, viz task "Fix 02-tiles.md")

**Render mód?**
Odpověď: Billboard

---

## Icon 338

![icon338](images/tile-full-338.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: hroudka hlínky

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 340

![icon340](images/tile-full-340.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: hrouda sýra

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 365

![icon365](images/tile-full-365.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: Bridge — jiný animační snímek stejného objektu jako ikona 364 (už vyřešeno v předchozím dotazníku)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné (stejně jako ikona 364)

---

## Icon 366

![icon366](images/tile-full-366.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: Bridge — jiný animační snímek stejného objektu jako ikona 364 (už vyřešeno v předchozím dotazníku)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné (stejně jako ikona 364)

---

## Icon 367

![icon367](images/tile-full-367.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 368

![icon368](images/tile-full-368.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 369

![icon369](images/tile-full-369.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 370

![icon370](images/tile-full-370.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 371

![icon371](images/tile-full-371.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 372

![icon372](images/tile-full-372.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: okraj hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — deska uprostřed krychle, textura na desce (stejný vzor jako ostatní "okraj hroudy" ikony v tomto kole)

---

## Icon 374

![icon374](images/tile-full-374.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: kus zeleného slizu

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 405

![icon405](images/tile-full-405.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: kapka zeleného slizu

**Render mód?**
Odpověď: Billboard

---

## Icon 406

![icon406](images/tile-full-406.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: kapka zeleného slizu

**Render mód?**
Odpověď: Billboard

---

## Icon 407

![icon407](images/tile-full-407.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: kapka zeleného slizu

**Render mód?**
Odpověď: Billboard

---

## Icon 408

![icon408](images/tile-full-408.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: kapka zeleného slizu

**Render mód?**
Odpověď: Billboard

---

## Icon 409

![icon409](images/tile-full-409.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: kapka zeleného slizu

**Render mód?**
Odpověď: Billboard

---

## Icon 414

![icon414](images/tile-full-414.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 415

![icon415](images/tile-full-415.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 416

![icon416](images/tile-full-416.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 417

![icon417](images/tile-full-417.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 418

![icon418](images/tile-full-418.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 419

![icon419](images/tile-full-419.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 420

![icon420](images/tile-full-420.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: yes
- Vizuální signál (průměrná alpha): solid/textured tile, uncategorized

**Co to je?**
Odpověď: budka pro přesun do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 436

![icon436](images/tile-full-436.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 438

![icon438](images/tile-full-438.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 439

![icon439](images/tile-full-439.png)

- Aktuální jméno v katalogu: (unnamed/unused)
- Katalogová poznámka (`02-tiles.md`): žádné jméno v `BlockTypes.hpp`, 0/78 souborů (nepoužito)
- Passable: no
- Vizuální signál (průměrná alpha): sparse/decorative (mostly transparent)

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---
