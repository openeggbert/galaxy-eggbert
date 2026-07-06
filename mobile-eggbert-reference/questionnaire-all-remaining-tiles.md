# Otázky ke zbývajícím dlaždicím (druhé kolo, ~280 ikon)

**Účel:** stejné jako `questionnaire-unidentified-tiles.md` (první kolo, 34 ikon, hotovo — viz
`15-3d-render-mapping-design.md` §11), ale tentokrát pro úplně všechny zbývající pojmenované
dlaždice — včetně těch, které jsem už dřív (8 agentů, první průchod, 2026-07-06) odhadl jako
`Billboard`/`ThinMechanical`/`special-surface`/`architectural-kit`, i těch, co jsem nechal jako
výchozí krychli (`UniformCube`) bez poznámky, protože jsem u nich neviděl žádný problém.

Vyplň odpovědi za `Odpověď:` u každé dlaždice, kterou poznáš — klidně přeskoč ty, u kterých si
nejsi jistý. Až to uložíš, dej mi vědět a zpracuji to zpět do `02-tiles.md`.

## Co rozlišujeme (render mód)

- **`UniformCube`** (krychle) — textura na všech 6 stranách. Výchozí varianta pro hromadný materiál
  (hlína, kámen, tráva, obyčejná zeď).
- **`Billboard`** — plochá tabule/sprite, vždy otočená čelem ke kameře. Sloupky, sochy, cedule,
  pilíře — "stojící předmět", ne plný objemový blok.
- **`ThinMechanical`** — tenký, plochý mechanický/nebezpečný prvek (pila, mřížka, ventilátor,
  potrubí, spínač...). Přesná geometrie ještě není rozhodnutá.
- **`DirectionalCube`** — krychle, ale textura je jen na některých stranách (řekni na kolika a
  kterých — 1 čelo? 2 protilehlé? 4 boční? shora/zdola?), zbylé strany mají jednu plochou barvu
  (řekni jakou) nebo jsou úplně průhledné/otevřené (řekni to taky).
- **speciální povrch** — voda, rostlina/chaluha — plochý povrch, ani krychle, ani billboard.
- **jiná/nová geometrie** — pokud tvar neodpovídá ničemu výše (jako dřívější "tyč" u ikony 202),
  popiš ho vlastními slovy.
- **nevím / nejde poznat** — klidně napiš "nevím", je to lepší než hádat.

U každé dlaždice napiš, prosím:
1. **Co to podle tebe je** (vlastními slovy, co obrázek zobrazuje)
2. **Jaký render mód by měl mít** (a pokud `DirectionalCube`, které strany mají texturu a jakou
   barvu/průhlednost mají ty ostatní)

Níže uvedená "moje dřívější poznámka" je buď z prvního 8-agentního průchodu (nezávisle
neověřeno), nebo prázdná/"unnamed variant" pokud jsem dlaždici nechal jako výchozí krychli.

---

## Icon 2

![icon2](images/tile-full-002.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: kus nějakého stroje

**Render mód?**
Odpověď: DirectionalCube — textura na 4 bočních stranách, shora a zdola plná barva (modrý odstín, stejný jako pozadí ikony)

---

## Icon 3

![icon3](images/tile-full-003.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 20/78 files

**Co to je?**
Odpověď: kus stroje s červeným světlem, které něco signalizuje

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 4

![icon4](images/tile-full-004.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 5

![icon5](images/tile-full-005.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 6

![icon6](images/tile-full-006.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 8

![icon8](images/tile-full-008.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 9

![icon9](images/tile-full-009.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 10

![icon10](images/tile-full-010.png)

- Aktuální jméno v katalogu: `Ground`
- Moje dřívější poznámka (kategorie): ground
- Animace: no, passable: no
- Použití: 30/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky (katalogové jméno `Ground` je podle uživatele nesprávné)

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 11

![icon11](images/tile-full-011.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 12

![icon12](images/tile-full-012.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: kus stroje s 3 barevnými světly signalizujícími něco

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 13

![icon13](images/tile-full-013.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: kus stroje s nějakými výstupky

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 14

![icon14](images/tile-full-014.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: kus stroje s 2 barevnými světly signalizujícími něco

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, zbylých 5 stran plná barva (modrý odstín); natočení textury (ze 4 možných stran) je uložené v metadatech daného bloku

---

## Icon 15

![icon15](images/tile-full-015.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 16/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura na 2 protilehlých bočních stranách (směr nastavitelný v metadatech bloku), shora plná barva (modrý odstín), zdola průhledné, ze zbylých 2 bočních stran jedna průhledná a druhá plná barva (modrý odstín)

---

## Icon 16

![icon16](images/tile-full-016.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 21/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura na 2 protilehlých bočních stranách (směr nastavitelný v metadatech bloku), shora plná barva (modrý odstín), zdola průhledné, ze zbylých 2 bočních stran jedna průhledná a druhá plná barva (modrý odstín)

---

## Icon 17

![icon17](images/tile-full-017.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 16/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura na 2 protilehlých bočních stranách (směr nastavitelný v metadatech bloku), shora plná barva (modrý odstín), zdola průhledné, ze zbylých 2 bočních stran jedna průhledná a druhá plná barva (modrý odstín)

---

## Icon 18

![icon18](images/tile-full-018.png)

- Aktuální jméno v katalogu: `StoneA`
- Moje dřívější poznámka (kategorie): ground/decoration
- Animace: no, passable: no
- Použití: 21/78 files

**Co to je?**
Odpověď: kus stroje (katalogové jméno `StoneA` je podle uživatele nesprávné)

**Render mód?**
Odpověď: DirectionalCube — textura na 2 protilehlých bočních stranách (směr nastavitelný v metadatech bloku), shora plná barva (modrý odstín), zdola průhledné, ze zbylých 2 bočních stran jedna průhledná a druhá plná barva (modrý odstín)

---

## Icon 19

![icon19](images/tile-full-019.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné ze 4 bočních stran, shora plná barva (modrý odstín), zbylé 4 strany (3 boční + zdola) průhledné

---

## Icon 20

![icon20](images/tile-full-020.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné ze 4 bočních stran, shora plná barva (modrý odstín), zbylé 4 strany (3 boční + zdola) průhledné

---

## Icon 21

![icon21](images/tile-full-021.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné ze 4 bočních stran, shora plná barva (modrý odstín), zbylé 4 strany (3 boční + zdola) průhledné

---

## Icon 22

![icon22](images/tile-full-022.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 17/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 23

![icon23](images/tile-full-023.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 34/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 24

![icon24](images/tile-full-024.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 35/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 25

![icon25](images/tile-full-025.png)

- Aktuální jméno v katalogu: `StoneB`
- Moje dřívější poznámka (kategorie): ground/decoration
- Animace: no, passable: no
- Použití: 42/78 files

**Co to je?**
Odpověď: kus stroje (katalogové jméno `StoneB` je podle uživatele nesprávné)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 26

![icon26](images/tile-full-026.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 27

![icon27](images/tile-full-027.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 15/78 files

**Co to je?**
Odpověď: kus stroje, 2 trubky, jedna z trubek má na sobě koule

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 28

![icon28](images/tile-full-028.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — textura jen na jedné boční straně, shora plná barva (modrý odstín), zbylé 4 strany (3 boční + zdola) průhledné

---

## Icon 29

![icon29](images/tile-full-029.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 30

![icon30](images/tile-full-030.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): yellow sign with painted numeral "1"
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: NENÍ Billboard (dřívější poznámka je špatně). Startovní pozice pohyblivého (moveable) objektu — editorová ikona, pozůstatek z desktopové Speedy Blupi (C++98/DirectX3). Ikony 30 a 31 se v Galaxy Eggbert použijí jen ve 3D editoru světa, ne ve hře samotné.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách (textura má i průhlednost), shora i zdola plná barva (žlutá)

---

## Icon 31

![icon31](images/tile-full-031.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): yellow sign with painted numeral "2"
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: NENÍ Billboard (dřívější poznámka je špatně). Koncová pozice pohyblivého (moveable) objektu, dvojice k ikoně 30 — editorová ikona, pozůstatek z desktopové Speedy Blupi (C++98/DirectX3). Použije se jen ve 3D editoru světa, ne ve hře samotné.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na všech 4 bočních stranách (textura má i průhlednost), shora i zdola plná barva (žlutá)

---

## Icon 35

![icon35](images/tile-full-035.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 36

![icon36](images/tile-full-036.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 18/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 37

![icon37](images/tile-full-037.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 18/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 38

![icon38](images/tile-full-038.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 39

![icon39](images/tile-full-039.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 40

![icon40](images/tile-full-040.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 41

![icon41](images/tile-full-041.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 5/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 42

![icon42](images/tile-full-042.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 3/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 43

![icon43](images/tile-full-043.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 44

![icon44](images/tile-full-044.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné

---

## Icon 45

![icon45](images/tile-full-045.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné

---

## Icon 46

![icon46](images/tile-full-046.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné

---

## Icon 47

![icon47](images/tile-full-047.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 3/78 files

**Co to je?**
Odpověď: dřevěná stěna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné

---

## Icon 48

![icon48](images/tile-full-048.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): yellow warning-triangle sign (name candidate: `WarningSign`)
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: NENÍ Billboard (dřívější poznámka je špatně). Kus stroje — textura je nálepka výstražného trojúhelníku.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné straně, zbylých 5 stran plná barva (modrý odstín)

---

## Icon 49

![icon49](images/tile-full-049.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 10/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 2 bočních stranách + shora + zdola (správně natočené textury), zbylé 2 boční strany plná barva (modrý odstín)

---

## Icon 50

![icon50](images/tile-full-050.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 14/78 files

**Co to je?**
Odpověď: kus stroje

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 51

![icon51](images/tile-full-051.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 14/78 files

**Co to je?**
Odpověď: kus stroje s tyčemi

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 52

![icon52](images/tile-full-052.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 16/78 files

**Co to je?**
Odpověď: tyče, co něco podpírají

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora i zdola plná barva (modrý odstín)

---

## Icon 53

![icon53](images/tile-full-053.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 23/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 54

![icon54](images/tile-full-054.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 25/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 55

![icon55](images/tile-full-055.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 26/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 56

![icon56](images/tile-full-056.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 29/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 57

![icon57](images/tile-full-057.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 18/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 58

![icon58](images/tile-full-058.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 17/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 59

![icon59](images/tile-full-059.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 24/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 60

![icon60](images/tile-full-060.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 24/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 63

![icon63](images/tile-full-063.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 10/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 64

![icon64](images/tile-full-064.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 9/78 files

**Co to je?**
Odpověď: šroub

**Render mód?**
Odpověď: nová geometrie — `TripleCrossBillboard`: stejná textura vykreslená 3× uprostřed bloku, každá rovina pootočená o 60° vůči ostatním, v půdorysu tvoří trojúhelník (obdoba "cross" billboardu u rostlin, ale se 3 rovinami místo 2)

---

## Icon 66

![icon66](images/tile-full-066.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): vertical rung-segmented column, reads as a ladder, passable
- Animace: no, passable: yes
- Použití: 15/78 files

**Co to je?**
Odpověď: NENÍ ThinMechanical/žebřík (dřívější poznámka je špatně). Několik cihel.

**Render mód?**
Odpověď: DirectionalCube — krychle, průchozí, textura jen na jedné ze 4 bočních stran (směr v metadatech bloku), zbylých 5 stran průhledných

---

## Icon 68

![icon68](images/tile-full-068.png)

- Aktuální jméno v katalogu: `Lava (base)`
- Moje dřívější poznámka (kategorie): hazard
- Animace: yes, 8 frames (68-72), passable: no
- Použití: 41/78 files — kills Blupi on contact

**Co to je?**
Odpověď: láva

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách (zachovává stávající animovaný render, žádná nová geometrie)

---

## Icon 69

![icon69](images/tile-full-069.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 9/78 files

**Co to je?**
Odpověď: láva (animační snímek stejné sekvence jako ikona 68)

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách (zachovává stávající animovaný render, žádná nová geometrie)

---

## Icon 74

![icon74](images/tile-full-074.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: hromada kamení

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, shora plná barva (šedivá), zdola průhledné

---

## Icon 75

![icon75](images/tile-full-075.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: hromada kamení

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, shora plná barva (šedivá), zdola průhledné

---

## Icon 76

![icon76](images/tile-full-076.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): stone pedestal/column
- Animace: no, passable: yes
- Užití: 22/78 files

**Co to je?**
Odpověď: kamenný podstavec/sloup, průchozí

**Render mód?**
Odpověď: NENÍ Billboard. Nová geometrie — `InnerPillarBox`: vnější krychle bloku je celá (všech 6 stran) průhledná; uvnitř bloku je menší kvádr (sloup), který má texturu na svých 4 bočních stranách

---

## Icon 77

![icon77](images/tile-full-077.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): yellow "Y" signpost/antenna
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: dřevěný podstavec/sloupek, průchozí

**Render mód?**
Odpověď: NENÍ Billboard. Nová geometrie — `InnerFlatPlate` (varianta `InnerPillarBox` z ikony 76, ale místo kvádru je uvnitř tenká deska): vnější krychle bloku celá průhledná; uvnitř je plochá deska s texturou na obou stranách

---

## Icon 86

![icon86](images/tile-full-086.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained balls)
- Animace: no, passable: yes
- Použití: 5/78 files

**Co to je?**
Odpověď: brána/mříž s protizávažím (mechanismus s kovovými tyčemi a závěsnými koulemi)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na bočních stranách, shora i zdola plná barva (šedivý odstín)

---

## Icon 87

![icon87](images/tile-full-087.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained balls)
- Animace: no, passable: no
- Použití: 18/78 files

**Co to je?**
Odpověď: brána/mříž s protizávažím (mechanismus s kovovými tyčemi a závěsnými koulemi)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na bočních stranách, shora plná barva (šedivý odstín), zdola průhledné

---

## Icon 88

![icon88](images/tile-full-088.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained balls)
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: brána/mříž s protizávažím (mechanismus s kovovými tyčemi a závěsnými koulemi)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na bočních stranách, shora plná barva (šedivý odstín), zdola průhledné

---

## Icon 89

![icon89](images/tile-full-089.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained balls)
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: brána/mříž s protizávažím (mechanismus s kovovými tyčemi a závěsnými koulemi)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na bočních stranách, shora plná barva (šedivý odstín), zdola průhledné

---

## Icon 90

![icon90](images/tile-full-090.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained balls)
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: brána/mříž s protizávažím (mechanismus s kovovými tyčemi a závěsnými koulemi)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na bočních stranách, shora plná barva (šedivý odstín), zdola průhledné

---

## Icon 91

![icon91](images/tile-full-091.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): `Water2` sub-frame (per `table_decor_eau2`, not `Water1`) — **special-surface** (§10.4): liquid surface sub-frame, not bulk material
- Animace: yes, part of `Water2`'s 6-frame cycle, passable: no
- Použití: 25/78 files

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 92

![icon92](images/tile-full-092.png)

- Aktuální jméno v katalogu: `Water1 (base)`
- Moje dřívější poznámka (kategorie): decorative/swimmable — **special-surface** (§10.4): flat liquid surface with wavy top-edge silhouette, not bulk material
- Animace: yes, 6 frames (92-95), passable: no
- Použití: 38/78 files — Blupi swims when grounded

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 96

![icon96](images/tile-full-096.png)

- Aktuální jméno v katalogu: `Water2 (base)`
- Moje dřívější poznámka (kategorie): decorative/swimmable — **special-surface** (§10.4): flat liquid surface with wavy top-edge silhouette, not bulk material
- Animace: yes, 6 frames (91, 96-98), passable: no
- Použití: not found in scanned files — Blupi swims when grounded

**Co to je?**
Odpověď: voda

**Render mód?**
Odpověď: OTEVŘENÁ OTÁZKA — uživatel zatím neví, jak vodu v 3D renderovat, rozhodne se později (viz task "Decide water tile render mode")

---

## Icon 107

![icon107](images/tile-full-107.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: DirectionalCube — krychle, textura (stávající dlaždice) na bočních stranách, zdola hnědá plná barva, shora samostatná textura trávy (viz task "Source/generate a grass-top texture" — potřeba sehnat licenčně vhodný asset nebo vygenerovat)

---

## Icon 108

![icon108](images/tile-full-108.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 3/78 files

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: DirectionalCube — krychle, 2 boční strany vlastní textura ikony 108, 1 boční strana textura ikony 107, 1 boční strana průhledná, zdola hnědá plná barva, shora samostatná textura trávy (viz task "Source/generate a grass-top texture")

---

## Icon 109

![icon109](images/tile-full-109.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: tráva

**Render mód?**
Odpověď: DirectionalCube — krychle, 2 boční strany vlastní textura ikony 109, 1 boční strana textura ikony 107, 1 boční strana průhledná, zdola hnědá plná barva, shora samostatná textura trávy (viz task "Source/generate a grass-top texture")

---

## Icon 110

![icon110](images/tile-full-110.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): thin dashed wire/spark/cable line segment, exact identity unclear
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: grafické znázornění větru od větráku, který Blupiho na daném místě táhne směrem pryč od větráku

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` — deska uprostřed krychle, textura na obou stranách

---

## Icon 114

![icon114](images/tile-full-114.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): thin dashed wire/spark/cable line segment, exact identity unclear
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: grafické znázornění větru od větráku, který Blupiho na daném místě táhne směrem pryč od větráku

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` — deska uprostřed krychle, textura na obou stranách

---

## Icon 118

![icon118](images/tile-full-118.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): thin dashed wire/spark/cable line segment, exact identity unclear
- Animace: no, passable: yes
- Použití: 4/78 files

**Co to je?**
Odpověď: grafické znázornění větru od větráku, který Blupiho na daném místě táhne směrem pryč od větráku

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` — deska uprostřed krychle, textura na obou stranách

---

## Icon 122

![icon122](images/tile-full-122.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): thin dashed wire/spark/cable line segment, exact identity unclear
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: grafické znázornění větru od větráku, který Blupiho na daném místě táhne směrem pryč od větráku

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` — deska uprostřed krychle, textura na obou stranách

---

## Icon 126

![icon126](images/tile-full-126.png)

- Aktuální jméno v katalogu: `FanLeft (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): ventilator with thin propeller blades on a hub, wall panel
- Animace: yes, 3 frames (126-128), passable: no
- Použití: 8/78 files — kills unless shielded

**Co to je?**
Odpověď: větrák (fan)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, 5. strana (základna větráku) plná barva (modrý odstín), 6. strana průhledná

---

## Icon 129

![icon129](images/tile-full-129.png)

- Aktuální jméno v katalogu: `FanRight (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): ventilator with thin propeller blades on a hub, wall panel
- Animace: yes, 3 frames (129-131), passable: no
- Použití: 7/78 files — kills unless shielded

**Co to je?**
Odpověď: větrák (fan)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, 5. strana (základna větráku) plná barva (modrý odstín), 6. strana průhledná

---

## Icon 132

![icon132](images/tile-full-132.png)

- Aktuální jméno v katalogu: `FanUp (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): ventilator with thin propeller blades on a hub, wall panel
- Animace: yes, 3 frames (132-134), passable: no
- Použití: 4/78 files — kills unless shielded

**Co to je?**
Odpověď: větrák (fan)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, 5. strana (základna větráku) plná barva (modrý odstín), 6. strana průhledná

---

## Icon 135

![icon135](images/tile-full-135.png)

- Aktuální jméno v katalogu: `FanDown (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): ventilator with thin propeller blades on a hub, wall panel
- Animace: yes, 3 frames (135-137), passable: no
- Použití: 1/78 files — kills unless shielded

**Co to je?**
Odpověď: větrák (fan)

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, 5. strana (základna větráku) plná barva (modrý odstín), 6. strana průhledná

---

## Icon 138

![icon138](images/tile-full-138.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): single blue pipe segment with flanged joints
- Animace: no, passable: yes
- Použití: 19/78 files

**Co to je?**
Odpověď: trubka, přichycená nahoře k nějakému dalšímu bloku

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` — deska uprostřed krychle

---

## Icon 144

![icon144](images/tile-full-144.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 145

![icon145](images/tile-full-145.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 146

![icon146](images/tile-full-146.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 147

![icon147](images/tile-full-147.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 148

![icon148](images/tile-full-148.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 149

![icon149](images/tile-full-149.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 150

![icon150](images/tile-full-150.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 151

![icon151](images/tile-full-151.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 152

![icon152](images/tile-full-152.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 153

![icon153](images/tile-full-153.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 154

![icon154](images/tile-full-154.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivá), zdola průhledné

---

## Icon 155

![icon155](images/tile-full-155.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivá), zdola průhledné

---

## Icon 156

![icon156](images/tile-full-156.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: hromada kamenů

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 158

![icon158](images/tile-full-158.png)

- Aktuální jméno v katalogu: `Sp0`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 0
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 159

![icon159](images/tile-full-159.png)

- Aktuální jméno v katalogu: `Sp1`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 1
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 160

![icon160](images/tile-full-160.png)

- Aktuální jméno v katalogu: `Sp2`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 2
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 161

![icon161](images/tile-full-161.png)

- Aktuální jméno v katalogu: `Sp3`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 3
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 162

![icon162](images/tile-full-162.png)

- Aktuální jméno v katalogu: `Sp4`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 4
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 163

![icon163](images/tile-full-163.png)

- Aktuální jméno v katalogu: `Sp5`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 5
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 164

![icon164](images/tile-full-164.png)

- Aktuální jméno v katalogu: `Sp6`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 6
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 165

![icon165](images/tile-full-165.png)

- Aktuální jméno v katalogu: `Sp7`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold pedestal, likely `SecretPower` value 7
- Animace: no, passable: yes
- Použití: 1/78 files — named but no behavior attached yet

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 174

![icon174](images/tile-full-174.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 175

![icon175](images/tile-full-175.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 176

![icon176](images/tile-full-176.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 177

![icon177](images/tile-full-177.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 178

![icon178](images/tile-full-178.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 9/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 179

![icon179](images/tile-full-179.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 4/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 180

![icon180](images/tile-full-180.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 2/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 181

![icon181](images/tile-full-181.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): red disc + digit on a post, numbered-marker family 174-181; exact purpose not yet identified
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: stanoviště pro přenesení do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 182

![icon182](images/tile-full-182.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): plain post/pillar; per `06-doors.md`'s `SearchDoor` this is the real door-trigger tile
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: obyčejný sloup/pilíř — skutečná spouštěcí dlaždice dveří (`SearchDoor`, viz `06-doors.md`)

**Render mód?**
Odpověď: Billboard

---

## Icon 183

![icon183](images/tile-full-183.png)

- Aktuální jméno v katalogu: `GoldPillar` (renamed from `Wall`, 2026-07-06 — the old name/description was wrong)
- Moje dřívější poznámka (kategorie): decorative/gate-adjacent
- Animace: no, passable: no
- Použití: 1/78 files (`world001.txt`, 12 cells forming two parallel 6-7-tile-tall columns — a gate/portal-frame shape, not tiled wall material) — **correction:** the crop is visibly a golden pillar/post, not brick-wall texture; sits immediately after icon 182 (the real door tile, see `06-doors.md`'s `SearchDoor`), consistent with a door/gate-adjacent decorative element. Separately, `Decor::AdaptDoors` gives icon 183 a special meaning on mobile-eggbert's hub/world-select screen (`m_mission==1`): it marks an uncollected world's gold, removed via a rising open-door-style animation once collected (see `06-doors.md`) — whether that specific hub context is `world001.txt` itself wasn't confirmed, but it shows this icon carries real symbolic weight beyond ordinary terrain.

**Co to je?**
Odpověď: sloup, nepruchozí

**Render mód?**
Odpověď: Billboard — uživatel se domnívá, že se sloup zvedá/animuje při otevírání dveří (potvrdit proti `Decor::AdaptDoors`/`06-doors.md` při implementaci)

---

## Icon 185

![icon185](images/tile-full-185.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: hromada kamenů, průchozí

**Render mód?**
Odpověď: UniformCube — textura na všech 6 stranách

---

## Icon 186

![icon186](images/tile-full-186.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: okno na straně domu

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 187

![icon187](images/tile-full-187.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): green ventilation-grille bars
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: NENÍ ventilační mřížka (dřívější poznámka je špatně). Okno na straně domu.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 188

![icon188](images/tile-full-188.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): green ventilation-grille bars
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: NENÍ ventilační mřížka (dřívější poznámka je špatně). Okno na straně domu.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 189

![icon189](images/tile-full-189.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): green ventilation-grille bars
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: NENÍ ventilační mřížka (dřívější poznámka je špatně). Okno na straně domu.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 190

![icon190](images/tile-full-190.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: okno na straně domu

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 191

![icon191](images/tile-full-191.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): thin red post
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: NENÍ billboard/tenký červený sloup (dřívější poznámka je špatně). Okno na straně domu.

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 192

![icon192](images/tile-full-192.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): painted door-with-hinges graphic
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: dveře na straně domu (identita souhlasí s dřívější poznámkou, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura jen na jedné boční straně (směr v metadatech bloku), zbylých 5 stran plná barva (béžový odstín)

---

## Icon 193

![icon193](images/tile-full-193.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: střecha domu s okny

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivý odstín), zdola plná barva (béžový odstín)

---

## Icon 194

![icon194](images/tile-full-194.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: střecha domu s okny

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivý odstín), zdola plná barva (béžový odstín)

---

## Icon 195

![icon195](images/tile-full-195.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: střecha domu s okny

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivý odstín), zdola plná barva (béžový odstín)

---

## Icon 196

![icon196](images/tile-full-196.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: střecha domu, bez okna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivý odstín), zdola plná barva (béžový odstín)

---

## Icon 197

![icon197](images/tile-full-197.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: střecha domu, bez okna

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (šedivý odstín), zdola plná barva (béžový odstín)

---

## Icon 199

![icon199](images/tile-full-199.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): yellow crossed-bar A-frame/brace, passable
- Animace: no, passable: yes
- Použití: 9/78 files

**Co to je?**
Odpověď: stejné jako ikona 77 (podstavec), jen jiný tvar podstavce

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerFlatPlate` (stejně jako ikona 77) — deska uprostřed krychle, textura na obou stranách

---

## Icon 201

![icon201](images/tile-full-201.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): metal cross-braced grate, passable — most common flagged icon in this pass (36/78 files)
- Animace: no, passable: yes
- Použití: 36/78 files

**Co to je?**
Odpověď: železné mříže (anglický popis "metal cross-braced grate" je přesnější než ThinMechanical klasifikace)

**Render mód?**
Odpověď: NENÍ ThinMechanical. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 203

![icon203](images/tile-full-203.png)

- Aktuální jméno v katalogu: `Marine (base)`
- Moje dřívější poznámka (kategorie): decorative/animated water — **special-surface** (§10.4): green seaweed/kelp frond — thin plant, not bulk; foliage-style billboard/cross-plane treatment recommended
- Animace: yes, 11 frames (203-208), passable: yes
- Použití: 18/78 files

**Co to je?**
Odpověď: Marine (mořská řasa)

**Render mód?**
Odpověď: Billboard

---

## Icon 211

![icon211](images/tile-full-211.png)

- Aktuální jméno v katalogu: `Spring`
- Moje dřívější poznámka (kategorie): interactive — **ThinMechanical** (§10.3): coiled spring, direct `Saw`-precedent parallel
- Animace: no, passable: no
- Použití: not found in scanned files — auto-launches Blupi upward

**Co to je?**
Odpověď: Spring (pružina)

**Render mód?**
Odpověď: NENÍ ThinMechanical. Billboard

---

## Icon 214

![icon214](images/tile-full-214.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): dashed red/yellow boundary-marker outline
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: (dřívější poznámka — přerušovaný červený/žlutý ohraničující obrys)

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 215

![icon215](images/tile-full-215.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): plain sphere/orb
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 216

![icon216](images/tile-full-216.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): plain sphere/orb
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 217

![icon217](images/tile-full-217.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): plain sphere/orb
- Animace: no, passable: no
- Použití: 14/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 218

![icon218](images/tile-full-218.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): colored balls/marbles on candy-striped poles
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 219

![icon219](images/tile-full-219.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): colored balls/marbles on candy-striped poles
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 220

![icon220](images/tile-full-220.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): colored balls/marbles on candy-striped poles
- Animace: no, passable: no
- Použití: 5/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 221

![icon221](images/tile-full-221.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): colored balls/marbles on candy-striped poles
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 222

![icon222](images/tile-full-222.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): colored balls/marbles on candy-striped poles
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 230

![icon230](images/tile-full-230.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): star badge
- Animace: no, passable: no
- Použití: 6/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 231

![icon231](images/tile-full-231.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): star badge
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 233

![icon233](images/tile-full-233.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): mushroom/tree silhouette
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 234

![icon234](images/tile-full-234.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): mushroom/tree silhouette
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: krychle s dětským motivem

**Render mód?**
Odpověď: NENÍ Billboard. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 235

![icon235](images/tile-full-235.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): candy-striped pole (same art family as 218-222), passable
- Animace: no, passable: yes
- Použití: 15/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 236

![icon236](images/tile-full-236.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): candy-striped pole (same art family as 218-222), passable
- Animace: no, passable: yes
- Použití: 14/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 245

![icon245](images/tile-full-245.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): arched window/doorway pair
- Animace: no, passable: yes
- Použití: 3/78 files

**Co to je?**
Odpověď: okna paláce

**Render mód?**
Odpověď: NENÍ Billboard. DirectionalCube — krychle, shora i zdola plná barva (šedivý odstín), textura jen na jedné boční straně, zbylé 3 boční strany průhledné

---

## Icon 250

![icon250](images/tile-full-250.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 251

![icon251](images/tile-full-251.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 252

![icon252](images/tile-full-252.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 253

![icon253](images/tile-full-253.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 8/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 254

![icon254](images/tile-full-254.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 3/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 255

![icon255](images/tile-full-255.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 3/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 256

![icon256](images/tile-full-256.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 257

![icon257](images/tile-full-257.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 258

![icon258](images/tile-full-258.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 259

![icon259](images/tile-full-259.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 260

![icon260](images/tile-full-260.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): blue pipe/valve/gauge-fitting family (straight run/elbow/T-junction/valve; exact sub-type per icon not resolved)
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: modrá trubka/ventil/měřicí přípojka (identita souhlasí, render mód ne)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na bočních stranách, shora plná barva (modrý odstín), zdola průhledné

---

## Icon 261

![icon261](images/tile-full-261.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zeď z cihel

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 262

![icon262](images/tile-full-262.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zeď z cihel

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 263

![icon263](images/tile-full-263.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zeď z cihel

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 264

![icon264](images/tile-full-264.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 265

![icon265](images/tile-full-265.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 266

![icon266](images/tile-full-266.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 267

![icon267](images/tile-full-267.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 268

![icon268](images/tile-full-268.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 269

![icon269](images/tile-full-269.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 6/78 files

**Co to je?**
Odpověď: okraje sýra (nejistota u uživatele)

**Render mód?**
Odpověď: NEJISTÉ — uživatel neví jistě, návrh: `InnerFlatPlate` (textura na desce uprostřed krychle)

---

## Icon 270

![icon270](images/tile-full-270.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 271

![icon271](images/tile-full-271.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 272

![icon272](images/tile-full-272.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 273

![icon273](images/tile-full-273.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **ThinMechanical** (§10.3): sparse yellow drip/goo/liquid overlay; marked `animated:no` but shape suggests an undetected animation cycle
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 274

![icon274](images/tile-full-274.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 275

![icon275](images/tile-full-275.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 276

![icon276](images/tile-full-276.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 6/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 277

![icon277](images/tile-full-277.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 4/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 278

![icon278](images/tile-full-278.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 4/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 279

![icon279](images/tile-full-279.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 280

![icon280](images/tile-full-280.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 281

![icon281](images/tile-full-281.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 282

![icon282](images/tile-full-282.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: okraje sýra

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 283

![icon283](images/tile-full-283.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: zeď z cihel

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 4 bočních stranách, shora i zdola plná barva (oranžový odstín)

---

## Icon 284

![icon284](images/tile-full-284.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: hroudy země

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 285

![icon285](images/tile-full-285.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 286

![icon286](images/tile-full-286.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 287

![icon287](images/tile-full-287.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 288

![icon288](images/tile-full-288.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 289

![icon289](images/tile-full-289.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 290

![icon290](images/tile-full-290.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 291

![icon291](images/tile-full-291.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 292

![icon292](images/tile-full-292.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 293

![icon293](images/tile-full-293.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 294

![icon294](images/tile-full-294.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 295

![icon295](images/tile-full-295.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 296

![icon296](images/tile-full-296.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 297

![icon297](images/tile-full-297.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 298

![icon298](images/tile-full-298.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 10/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 299

![icon299](images/tile-full-299.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 9/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 300

![icon300](images/tile-full-300.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 301

![icon301](images/tile-full-301.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: hroudy hlíny

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 302

![icon302](images/tile-full-302.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 11/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 303

![icon303](images/tile-full-303.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 12/78 files

**Co to je?**
Odpověď: okraje hroudy hlíny

**Render mód?**
Odpověď: `InnerFlatPlate` — textura na desce uprostřed krychle

---

## Icon 304

![icon304](images/tile-full-304.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball on a red spring/screw bumper
- Animace: no, passable: no
- Použití: 15/78 files

**Co to je?**
Odpověď: generátor blesku

**Render mód?**
Odpověď: Billboard

---

## Icon 305

![icon305](images/tile-full-305.png)

- Aktuální jméno v katalogu: `Blitz`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): thin jagged lightning-bolt/arc line, not bulk
- Animace: tick-gated, passable: no
- Použití: 15/78 files — kills 25% of ticks

**Co to je?**
Odpověď: blesk (identita souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. Billboard

---

## Icon 309

![icon309](images/tile-full-309.png)

- Aktuální jméno v katalogu: `Marker`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): small gold trophy/cup on a pedestal
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: malý zlatý pohár na podstavci (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 317

![icon317](images/tile-full-317.png)

- Aktuální jméno v katalogu: `Crusher (base)`
- Moje dřívější poznámka (kategorie): hazard — **Billboard** (§10.2): teal segmented piston shaft on a small base, thin mechanical rod not a solid block
- Animace: yes, 10 frames (317-323), passable: no
- Použití: 3/78 files — kills Blupi on contact

**Co to je?**
Odpověď: Crusher (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 324

![icon324](images/tile-full-324.png)

- Aktuální jméno v katalogu: `Temp (base)`
- Moje dřívější poznámka (kategorie): hazard/passable-timing — **ThinMechanical** (§10.3): stepped triangular/pyramid wedge, non-cuboid silhouette
- Animace: yes, 20 frames, 2 blank (324-329), passable: no
- Použití: 5/78 files — vanishes 2/20 frames

**Co to je?**
Odpověď: Temp (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. Billboard

---

## Icon 330

![icon330](images/tile-full-330.png)

- Aktuální jméno v katalogu: `Teleport1`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): cone/pyramid beacon post with colored indicator dots + emblem letter
- Animace: no, passable: no
- Použití: 11/78 files — solid pillar

**Co to je?**
Odpověď: Teleport1 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 331

![icon331](images/tile-full-331.png)

- Aktuální jméno v katalogu: `Teleport2`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): cone/pyramid beacon post with colored indicator dots + emblem letter
- Animace: no, passable: no
- Použití: 6/78 files — solid pillar

**Co to je?**
Odpověď: Teleport2 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 332

![icon332](images/tile-full-332.png)

- Aktuální jméno v katalogu: `Teleport3`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): cone/pyramid beacon post with colored indicator dots + emblem letter
- Animace: no, passable: no
- Použití: 4/78 files — solid pillar

**Co to je?**
Odpověď: Teleport3 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 333

![icon333](images/tile-full-333.png)

- Aktuální jméno v katalogu: `Teleport4`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): cone/pyramid beacon post with colored indicator dots + emblem letter
- Animace: no, passable: no
- Použití: 3/78 files — solid pillar

**Co to je?**
Odpověď: Teleport4 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 334

![icon334](images/tile-full-334.png)

- Aktuální jméno v katalogu: `Door1`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): red pillar/bollard shape, not a door panel; supersedes the earlier UniformCube call
- Animace: no, passable: no
- Použití: 12/78 files — key-gated, see 06-doors.md

**Co to je?**
Odpověď: Door1 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 335

![icon335](images/tile-full-335.png)

- Aktuální jméno v katalogu: `Door2`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): same red pillar/bollard family as 334, not a door panel
- Animace: no, passable: no
- Použití: 10/78 files — key-gated, see 06-doors.md

**Co to je?**
Odpověď: Door2 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 336

![icon336](images/tile-full-336.png)

- Aktuální jméno v katalogu: `Door3`
- Moje dřívější poznámka (kategorie): interactive — **Billboard** (§10.2): same red pillar/bollard family as 334, not a door panel
- Animace: no, passable: no
- Použití: 11/78 files — key-gated, see 06-doors.md

**Co to je?**
Odpověď: Door3 (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard

---

## Icon 337

![icon337](images/tile-full-337.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 8/78 files

**Co to je?**
Odpověď: (souhlas s dřívější poznámkou)

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 339

![icon339](images/tile-full-339.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: yes
- Použití: 4/78 files

**Co to je?**
Odpověď: sýr

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 341

![icon341](images/tile-full-341.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 342

![icon342](images/tile-full-342.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 343

![icon343](images/tile-full-343.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 344

![icon344](images/tile-full-344.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 345

![icon345](images/tile-full-345.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 346

![icon346](images/tile-full-346.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 347

![icon347](images/tile-full-347.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 348

![icon348](images/tile-full-348.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 349

![icon349](images/tile-full-349.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 350

![icon350](images/tile-full-350.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 351

![icon351](images/tile-full-351.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 352

![icon352](images/tile-full-352.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 353

![icon353](images/tile-full-353.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 354

![icon354](images/tile-full-354.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 355

![icon355](images/tile-full-355.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 356

![icon356](images/tile-full-356.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 357

![icon357](images/tile-full-357.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 358

![icon358](images/tile-full-358.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 11/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 359

![icon359](images/tile-full-359.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 360

![icon360](images/tile-full-360.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 361

![icon361](images/tile-full-361.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 5/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 362

![icon362](images/tile-full-362.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 13/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 363

![icon363](images/tile-full-363.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 12/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: UniformCube — krychle, textura na všech 6 stranách

---

## Icon 364

![icon364](images/tile-full-364.png)

- Aktuální jméno v katalogu: `Bridge`
- Moje dřívější poznámka (kategorie): interactive — **ThinMechanical** (§10.3): thin walkway/log structure — only the top 16px band is solid per `table_decor_quart`
- Animace: no, passable: no
- Použití: 9/78 files — mostly solid in 2D too (only its top quarter is solid per `table_decor_quart`, `IsPassIcon(364)` returns false); solid in galaxy-eggbert. **Correction (2026-07-05, DOC-306):** an earlier version of this row claimed 364 was fully "passable in 2D" — false, see `14-crates-lifts-bridges-effects.md` for the full construction-sequence behavior (the cell does become fully hollow for most of the 157-tick build animation, but the finished/idle tile itself is not simply "passable").

**Co to je?**
Odpověď: Bridge (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. DirectionalCube — krychle, textura na 4 bočních stranách, shora plná barva (hnědý odstín), zdola průhledné

---

## Icon 373

![icon373](images/tile-full-373.png)

- Aktuální jméno v katalogu: `Spike (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): lower confidence: may just be the glossy wall-texture family, not an obviously pointed spike; flagged for a closer look
- Animace: yes, 16 frames (347,373,374), passable: no
- Použití: 5/78 files — kills Blupi on contact

**Co to je?**
Odpověď: NENÍ Spike/hrot (dřívější poznámka je špatně) — zelený sliz

**Render mód?**
Odpověď: NENÍ ThinMechanical. UniformCube — krychle, textura na všech 6 stranách

---

## Icon 375

![icon375](images/tile-full-375.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): twisted rope/banded decorative post
- Animace: no, passable: yes
- Použití: 3/78 files

**Co to je?**
Odpověď: podstavec/sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 376

![icon376](images/tile-full-376.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): twisted rope/banded decorative post
- Animace: no, passable: yes
- Použití: 5/78 files

**Co to je?**
Odpověď: podstavec/sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 377

![icon377](images/tile-full-377.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): twisted rope/banded decorative post
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: podstavec/sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 378

![icon378](images/tile-full-378.png)

- Aktuální jméno v katalogu: `Saw (base)`
- Moje dřívější poznámka (kategorie): hazard — **ThinMechanical** (§10.3): circular saw blade — thin, flat, not bulk material
- Animace: yes, 6 frames (378-383), passable: no
- Použití: 15/78 files — kills Blupi on contact; stoppable by Switch

**Co to je?**
Odpověď: Saw (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. Billboard

---

## Icon 379

![icon379](images/tile-full-379.png)

- Aktuální jméno v katalogu: `SawStopped`
- Moje dřívější poznámka (kategorie): interactive/hazard-adjacent — **ThinMechanical** (§10.3): same circular-blade shape as 378, stopped state
- Animace: no, passable: no
- Použití: not found in scanned files — the Saw's toggled-off, safe static state; `GETerrainRenderer` swaps a saw tile between icon 378 (spinning) and 379 (stopped) when a linked `Switch` is toggled

**Co to je?**
Odpověď: SawStopped (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. Billboard

---

## Icon 384

![icon384](images/tile-full-384.png)

- Aktuální jméno v katalogu: `Switch`
- Moje dřívější poznámka (kategorie): interactive — **ThinMechanical** (§10.3): flat wall-mounted control box with ON/OFF lights
- Animace: no, passable: no
- Použití: 8/78 files — toggles linked Saw tiles

**Co to je?**
Odpověď: Switch (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerPillarBox` (jako ikona 76) — vnější krychle průhledná, uvnitř kvádr s texturou na jedné straně, zbylé strany kvádru plná barva (modrý odstín)

---

## Icon 385

![icon385](images/tile-full-385.png)

- Aktuální jméno v katalogu: `SwitchOff`
- Moje dřívější poznámka (kategorie): interactive — **ThinMechanical** (§10.3): flat wall-mounted control box with ON/OFF lights
- Animace: no, passable: no
- Použití: not found in scanned files — toggles linked Saw tiles

**Co to je?**
Odpověď: SwitchOff (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: NENÍ ThinMechanical. `InnerPillarBox` (jako ikona 76) — vnější krychle průhledná, uvnitř kvádr s texturou na jedné straně, zbylé strany kvádru plná barva (modrý odstín)

---

## Icon 390

![icon390](images/tile-full-390.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 5/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 391

![icon391](images/tile-full-391.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): twin arch/window niches — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 392

![icon392](images/tile-full-392.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): left door-jamb edge piece — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 393

![icon393](images/tile-full-393.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): right door-jamb edge piece — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 394

![icon394](images/tile-full-394.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): lintel/corner-post fragment — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 395

![icon395](images/tile-full-395.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): lintel/corner-post fragment — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 396

![icon396](images/tile-full-396.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 397

![icon397](images/tile-full-397.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): lintel/corner-post fragment — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: stavební blok paláce

**Render mód?**
Odpověď: DirectionalCube — krychle, textura jen na jedné boční straně, zbylé strany plná barva (šedivý odstín)

---

## Icon 398

![icon398](images/tile-full-398.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): fence rail on two posts
- Animace: no, passable: yes
- Použití: 7/78 files

**Co to je?**
Odpověď: zábradlí (souhlasí s dřívější poznámkou), ale je statické — neotáčí se dle pohledu hráče

**Render mód?**
Odpověď: NENÍ klasický (kamerou otočný) Billboard. `InnerFlatPlate` — deska uprostřed průhledné krychle, textura na obou stranách desky

---

## Icon 399

![icon399](images/tile-full-399.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): fluted classical column
- Animace: no, passable: yes
- Použití: 5/78 files

**Co to je?**
Odpověď: sloupec (souhlasí s dřívější poznámkou)

**Render mód?**
Odpověď: Billboard — klasický, otáčí se dle pohledu hráče

---

## Icon 400

![icon400](images/tile-full-400.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **architectural-kit** (§10.5): the archway opening itself — likely a fragment of a modular door/archway sprite kit
- Animace: no, passable: yes
- Použití: 5/78 files

**Co to je?**
Odpověď: brána/vchod

**Render mód?**
Odpověď: DirectionalCube — krychle, textura na 2 protilehlých bočních stranách, zbylé strany plná barva (šedivý odstín); průchozí je to jen skrz tuto texturovanou stranu (otvor)

---

## Icon 404

![icon404](images/tile-full-404.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): green vase/bulb on a neck
- Animace: no, passable: yes
- Použití: 3/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: Billboard

---

## Icon 410

![icon410](images/tile-full-410.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): small green knob/dome, mostly cropped
- Animace: no, passable: yes
- Použití: 5/78 files

**Co to je?**
Odpověď: zelený sliz

**Render mód?**
Odpověď: Billboard

---

## Icon 411

![icon411](images/tile-full-411.png)

- Aktuální jméno v katalogu: `Tile411`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold picture-frame-on-pedestal display stand
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: budka do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 412

![icon412](images/tile-full-412.png)

- Aktuální jméno v katalogu: `Tile412`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold picture-frame-on-pedestal display stand
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: budka do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 413

![icon413](images/tile-full-413.png)

- Aktuální jméno v katalogu: `Tile413`
- Moje dřívější poznámka (kategorie): unclassified — **Billboard** (§10.2): gold picture-frame-on-pedestal display stand
- Animace: no, passable: yes
- Použití: 1/78 files

**Co to je?**
Odpověď: budka do jiného světa

**Render mód?**
Odpověď: Billboard

---

## Icon 421

![icon421](images/tile-full-421.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 10/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 422

![icon422](images/tile-full-422.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 15/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 423

![icon423](images/tile-full-423.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 424

![icon424](images/tile-full-424.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 9/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 425

![icon425](images/tile-full-425.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 4/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 426

![icon426](images/tile-full-426.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 427

![icon427](images/tile-full-427.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 7/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 428

![icon428](images/tile-full-428.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 429

![icon429](images/tile-full-429.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 430

![icon430](images/tile-full-430.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 431

![icon431](images/tile-full-431.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 432

![icon432](images/tile-full-432.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 2/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 433

![icon433](images/tile-full-433.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 434

![icon434](images/tile-full-434.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 435

![icon435](images/tile-full-435.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---

## Icon 437

![icon437](images/tile-full-437.png)

- Aktuální jméno v katalogu: (unnamed)
- Moje dřívější poznámka (kategorie): unnamed variant — **Billboard** (§10.2): gold ball + digit on a post, numbered-marker family 421-437; exact purpose not yet identified
- Animace: no, passable: no
- Použití: 1/78 files

**Co to je?**
Odpověď: sloup

**Render mód?**
Odpověď: Billboard

---
