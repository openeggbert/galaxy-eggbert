# renderers.md — Jak dát Galaxy Eggbertu lehký render i „2026" hi-fi render zároveň

_Poznámka: návrhový/směrový dokument, ne změna herního kódu. Popisuje, jak architektonicky
umožnit dva zaměnitelné renderery za jedním rozhraním. Vše respektuje stávající pravidla
(faithful remake, `../mobile-eggbert` se nikdy nemění, žádné kopírování kódu/dat, žádné `#ifdef`
pro rozdíly enginů). Volba směru je na vlastníkovi projektu — tento dokument nic nezavazuje, jen
navrhuje._

---

## Krátká odpověď

Je to reálné a **půlka práce, která to umožňuje, je už hotová.** Simulace (`GEBlupiController`,
`GEInteractionSystem`, `GEWorldRuntime`) je čistě bez grafiky — to je přesně účel decouplingu /
`*ThisFrame()` sběrnice. Veškerý render sedí v `GalaxyEggbertCnaGame::Draw()` (~817 řádků od
ř. 3100) plus `GETerrainRenderer` / `GEHud` / `GEObjectIcons` / tile-helpery / `GETileAtlas`.

Takže sim už produkuje stav a `Draw()` ho jen vykresluje — ten šev tam de facto **je**, jen zatím
není pojmenovaný. (Ověřeno: sim nemá žádnou závislost na `Easy3D::`/`GraphicsDevice`/`Camera3D`;
grafiku volá jen render vrstva a editor.)

---

## Princip: jedno rozhraní „scéna", dva renderery za ním

Klíč není „dva rendery vedle sebe s `#ifdef`", ale **jedno stabilní rozhraní mezi simulací a
rendererem**. Simulace každý snímek vyprodukuje engine-neutrální *popis scény*, a renderery jsou
zaměnitelné implementace jednoho rozhraní:

```
        Engine-agnostic core (World/Block/def)   ← už existuje, sdílené
                     │
        Simulace (Blupi, Interaction, WorldRuntime)   ← už bez grafiky ✔
                     │
                     ▼
          SceneFrame  (kamera, dlaždice+animFrame, billboardy+ikony,
                       Blupi stav, HUD prvky, skyRegion/pozadí)   ← NOVÉ, to je ten šev
                     │
         ┌───────────┴────────────┐
         ▼                        ▼
   GERendererLite            GERendererHi ("2026")
   Easy3D billboardy+kostky  3D modely, PBR, stíny/GI,
   2D HUD, běží i na webu    částice, bloom/SSAO/tonemap
```

Obě implementace berou **stejný `SceneFrame`**. Hra se nemění — mění se jen ten, kdo scénu kreslí.

---

## Konkrétní kroky vzhledem k dnešnímu kódu

1. **Vytáhnout `SceneFrame` ze `Draw()`.** `Draw()` dnes sahá přímo do stavu sim/světa. Prvním
   krokem je, aby `Draw()` četl **jen** `SceneFrame`:
   - kamera (pozice, orientace, FOV),
   - terén: seznam `(cell, blockType, renderMode, animFrame)`,
   - dynamické objekty: seznam `(worldPos, objectType, ikona/animState, facing)`,
   - Blupi: `(worldPos, animState, směr, vehicleMode, secretPower)`,
   - HUD: prvky v 640×480 ref-prostoru,
   - prostředí: `skyRegion`, id pozadí.

   Tohle je nejtěžší a nejcennější krok — a rozseká i ty „god" metody (RC-4 z `REMAKE-ANALYSIS.md`).
2. **Definovat `IGameRenderer`** — `BeginFrame(camera)`, `SubmitTerrain(...)`, `SubmitBillboard(...)`,
   `SubmitHud(...)`, `EndFrame()`. Dnešní kód se stane `GERendererLite : IGameRenderer`.
3. **Přidat `GERendererHi`** jako druhou implementaci, vybíranou **za běhu** (config/flag), ne přes
   `#ifdef` — to respektuje pravidlo z `CLAUDE.md`. Lite zůstává default pro web/WASM a slabé
   stroje, Hi je pro desktop.

---

## Co dělá „2026" renderer jinak

- Místo plochých billboardů → **skutečné 3D modely** dlaždic/objektů (normal-mapy, PBR materiály),
  real-time **stíny + GI**, částicové systémy (výbuchy, voda, kouř), post-processing stack (bloom,
  SSAO, tonemapping), lepší kamera/hloubka.
- **Zajímavý vedlejší efekt:** hi-fi cesta vlastně **řeší RC-5 i „billboard walk-cycle" problém**
  z analýzy — jakmile jsou postavy/dlaždice reálné 3D modely, zmizí hádání „jak zobrazit 2D sprite
  ve 3D" a nepřátelé „nechodí bokem" při pohledu pod úhlem. Ale za cenu, že potřebuje **nové 3D
  assety, které dnes neexistují** (to je i ten dlouhodobý blocker „žádný viditelný 3D Blupi model").

---

## Rozhodnutí, která jsou na vlastníkovi (a mají tvrdý dopad)

Tohle je změna směru projektu, takže tato rozhodnutí musí padnout shora — nezačínat s implementací,
dokud nejsou potvrzena:

1. **Direction lock.** Dnes je v `CLAUDE.md`/`NEXT.md` zamčeno „Direct CNA + Easy3D je jediný cíl".
   Dvourenderer to rozšiřuje — to je vědomá změna locku.
2. **Faithful-remake čára.** Musí zůstat: **mění se jen prezentace, ne mechaniky.** Obě verze hrají
   identicky (stejná fyzika, timing, objekty). „Úžasná grafika" je prezentační volba, ne nová herní
   mechanika — to je v pořádku, ale gameplay se nesmí hnout.
3. **Assety.** Hi-fi renderer potřebuje sadu 3D modelů + PBR textur navíc (volitelnou). Lite dál
   používá mobile-eggbert PNG sprity. Kdo/jak ty modely dodá?
4. **Engine pro hi-fi** — dvě cesty:
   - **A) Stejný CNA/Easy3D základ, „nabušené" shadery/materiály** — méně práce, sdílí kód, ale
     strop kvality dá CNA.
   - **B) Separátní moderní engine backend** za stejným `IGameRenderer` — vyšší strop „2026
     úžasně", ale větší práce a druhý závislostní strom.

---

## Bonus: synergie s předchozí analýzou

Ten `SceneFrame` je **přesně ten artefakt**, který potřebuje P0 parity/golden harness z
`REMAKE-ANALYSIS.md` — deterministický popis snímku, který jde diffovat. Takže tenhle refaktor a
oprava „hromad problémů" táhnou za stejný provaz: čistý šev sim↔render zároveň umožní dva renderery
*i* automatické ověřování správnosti.

---

## Doporučená sekvence

Nejdřív **krok 1–2 (`SceneFrame` + `IGameRenderer`)** s dnešním rendererem jako `GERendererLite`.
To má hodnotu samo o sobě (rozbije god-objekty, umožní golden harness) a **nezavazuje k ničemu
ohledně hi-fi**. Hi-fi renderer je pak čistě přídavek za hotovým rozhraním.

Otevřené k rozhodnutí: bod 4 — **A (stejný CNA základ)** vs **B (separátní moderní engine)** — to
zásadně mění rozsah a plán hi-fi cesty.
