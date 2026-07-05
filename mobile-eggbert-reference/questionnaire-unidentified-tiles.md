# Otázky k neidentifikovaným dlaždicím (34 ikon)

**Účel tohoto souboru:** toto NENÍ trvalý katalogový dokument (nemá číslo v `00-overview.md`'s
indexu) — je to pracovní dotazník. Vyplň odpovědi přímo pod každou otázku (za `Odpověď:`), soubor
ulož, a dej mi vědět. Já pak tvoje odpovědi zpracuji zpět do `02-tiles.md`
(a případně `15-3d-render-mapping-design.md`).

Jde o 34 dlaždic, u kterých 8 agentů při vizuální kontrole (`15-3d-render-mapping-design.md` §10.6)
nedokázalo s jistotou určit, co obrázek vlastně zobrazuje — proto jsou v `02-tiles.md` označené
jako `needs identification` místo billboard/krychle/ThinMechanical.

## Co rozlišujeme (render mód)

Pro každou dlaždici rozhodujeme, jak se má v 3D vykreslit — na výběr je:

- **`UniformCube`** (krychle) — normální krychle, textura dlaždice se nalepí na všech 6 stěn.
  Výchozí varianta; hodí se pro "hromadný" materiál (hlína, kámen, tráva, obyčejná zeď).
- **`Billboard`** — plochá tabule/sprite, která se vždy otočí čelem ke kameře. Hodí se pro
  vzácné/speciální předměty jako sloupky, sochy, cedule, pilíře — věci, které na obrázku vypadají
  jako "stojící předmět", ne jako plný objemový blok.
- **`ThinMechanical`** — tenký, plochý mechanický/nebezpečný prvek (pila, mřížka, ventilátor,
  potrubí, spínač...) — není to ani plná krychle, ani billboard otočený ke kameře (např. pila leží
  v pevné rovině, mřížka je našroubovaná na zeď). Přesná 3D geometrie pro tento mód ještě není
  rozhodnutá, teprve se řeší.
- **speciální povrch** — voda, rostlina/chaluha — plochý povrch, ani krychle, ani billboard.
- **nevím / nejde poznat** — pokud ani ty z obrázku nepoznáš, co to je, klidně napiš "nevím" — je
  to lepší než hádat.

U každé dlaždice napiš, prosím:
1. **Co to podle tebe je** (vlastními slovy, co obrázek zobrazuje)
2. **Jaký render mód by měl mít** (jedna z výše uvedených pěti možností)

---

## Icon 1

![icon1](images/tile-full-001.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 10/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): plochý panel s vlnovkou + barevné tečky, možná ovládací panel/displej

**Co to je?**
Odpověď: ovladaci panel s obrazovkou a tlacitky (nebo to jsou svetla)

**Render mód?**
Odpověď: krychle ale ta texture se nalepi jenom na jednu stranu ze 4 ostatni strany maji tu modrou barvu

---

## Icon 7

![icon7](images/tile-full-007.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): plochý panel s vlnovkou + barevné tečky, možná ovládací panel/displej (stejné jako 1)

**Co to je?**
Odpověď: kus nejakeho stroje

**Render mód?**
Odpověď: krychle ale ta texture se nalepi jenom na jednu stranu ze 4 ostatni strany maji tu modrou barvu

---

## Icon 61

![icon61](images/tile-full-061.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 6/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): oranžovo-hnědý dřevěný tvar, možná prkno/trám příbuzný `Ladder` (66)

**Co to je?**
Odpověď: je to cihla

**Render mód?**
Odpověď: asi to udelame jako billboard

---

## Icon 62

![icon62](images/tile-full-062.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 6/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): oranžovo-hnědý dřevěný tvar, možná prkno/trám příbuzný `Ladder` (66)

**Co to je?**
Odpověď: cihla

**Render mód?**
Odpověď: asi to udelame jako billboard

---

## Icon 65

![icon65](images/tile-full-065.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 6/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): oranžovo-hnědý dřevěný tvar, možná prkno/trám příbuzný `Ladder` (66)

**Co to je?**
Odpověď: nekolik cihel

**Render mód?**
Odpověď: billboard

---

## Icon 67

![icon67](images/tile-full-067.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): oranžovo-hnědý dřevěný tvar, možná prkno/trám příbuzný `Ladder` (66)

**Co to je?**
Odpověď: cihly

**Render mód?**
Odpověď: billboard

---

## Icon 73

![icon73](images/tile-full-073.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 12/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): kruh malých koulí kolem zlatého středu, kruhové/rotující uspořádání

**Co to je?**
Odpověď: pruchozi blok

**Render mód?**
Odpověď: krychle - bude mit tuto texturu na vsech 4 stranach shora a zdola bude jenom modra barva

---

## Icon 78

![icon78](images/tile-full-078.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory, možná spínač/deska plošných spojů

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 79

![icon79](images/tile-full-079.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 80

![icon80](images/tile-full-080.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 81

![icon81](images/tile-full-081.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 82

![icon82](images/tile-full-082.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 83

![icon83](images/tile-full-083.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 13/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 84

![icon84](images/tile-full-084.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 14/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: krychle s texturou na vsech 6 stranach

---

## Icon 85

![icon85](images/tile-full-085.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 2/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): tmavý kovový panel se šrouby/pákami/konektory (stejná rodina jako 78)

**Co to je?**
Odpověď: nejaky block

**Render mód?**
Odpověď: na 4 stranach (2 shora 2 zboku bude tato texture), zbyvajici 2 strany budou mit sedivou barvu

---

## Icon 139

![icon139](images/tile-full-139.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 9/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): dřevěná knihovna/skříňka (zavřené/otevřené dveře), silně čelní kresba

**Co to je?**
Odpověď: knihovna

**Render mód?**
Odpověď: textura jenom na jedne strane, ostatnich 5 stran bude mit ten oranzovy dreveny odstin

---

## Icon 140

![icon140](images/tile-full-140.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 10/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): dřevěná knihovna/skříňka (stejná rodina jako 139)

**Co to je?**
Odpověď: knihovna

**Render mód?**
Odpověď: textura jenom na jedne strane, ostatnich 5 stran bude mit ten oranzovy dreveny odstin

---

## Icon 141

![icon141](images/tile-full-141.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 9/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): dřevěná knihovna/skříňka (stejná rodina jako 139)

**Co to je?**
Odpověď: knihovna

**Render mód?**
Odpověď: textura jenom na jedne strane, ostatnich 5 stran bude mit ten oranzovy dreveny odstin

---

## Icon 142

![icon142](images/tile-full-142.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 9/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): dřevěná knihovna/skříňka (stejná rodina jako 139)

**Co to je?**
Odpověď: knihovna

**Render mód?**
Odpověď: textura jenom na jedne strane, ostatnich 5 stran bude mit ten oranzovy dreveny odstin

---

## Icon 143

![icon143](images/tile-full-143.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): dřevěná knihovna/skříňka (stejná rodina jako 139)

**Co to je?**
Odpověď: knihovna

**Render mód?**
Odpověď: textura jenom na jedne strane, ostatnich 5 stran bude mit ten oranzovy dreveny odstin

---

## Icon 198

![icon198](images/tile-full-198.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 10/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): bílý zaoblený oblouk/kopule, možná tunel nebo iglú

**Co to je?**
Odpověď: pruchozi blok

**Render mód?**
Odpověď: krychle tato textura bude na dvou bocnich stranach proti sobe, ostatni strany budou mit tu bezovou barvu

---

## Icon 200

![icon200](images/tile-full-200.png)

- Aktuální jméno v katalogu: `Platform`
- Použití: 27/78 souborů — "floating platform", passable: ano
- Můj dřívější odhad (nejistý): ZPOCHYBNĚNO: název říká plovoucí plošina, ale ořez ukazuje dvě tenké svislé nožičky, ne plochou desku

**Co to je?**
Odpověď: pruchozi blok - mrize

**Render mód?**
Odpověď: krychle textura na 4 bocnich stranach, ze zhora z zdola nic (pruhledno)

---

## Icon 202

![icon202](images/tile-full-202.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 11/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): skoro celé průhledné, jen jedna tenká vodorovná čára

**Co to je?**
Odpověď: toto je tyc, blupi na ni muze skocit a preleze nebezpecnou prekazku dole take

**Render mód?**
Odpověď: toto je specialni pripad, nejak to budes renderovat v 3d prostoru, ale ne billboard, asi tu texturu preneses na kvadrovy model tyce na ty 4 dlouhe strany 2 bocni strany (maly prostor, ctverec) budou mit modrou barvu

---

## Icon 246

![icon246](images/tile-full-246.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): souvislý reliéfní bublinkový/pěnový vzor přes celou dlaždici

**Co to je?**
Odpověď: syr

**Render mód?**
Odpověď: krychle textura na vsech stranach

---

## Icon 247

![icon247](images/tile-full-247.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): souvislý reliéfní bublinkový/pěnový vzor (stejná rodina jako 246)

**Co to je?**
Odpověď: syr

**Render mód?**
Odpověď: krychle textura na vsech stranach

---

## Icon 248

![icon248](images/tile-full-248.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): souvislý reliéfní bublinkový/pěnový vzor (stejná rodina jako 246)

**Co to je?**
Odpověď: syr

**Render mód?**
Odpověď: krychle textura na vsech stranach

---

## Icon 249

![icon249](images/tile-full-249.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 8/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): souvislý reliéfní bublinkový/pěnový vzor (stejná rodina jako 246)

**Co to je?**
Odpověď: syr

**Render mód?**
Odpověď: krychle textura na vsech stranach

---

## Icon 386

![icon386](images/tile-full-386.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 6/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tenký svislý tvar (deska/vlajka na tyči, válcový sloupek, kotouč na podstavci), ale označen jako plný/neprůchozí

**Co to je?**
Odpověď: bock palace

**Render mód?**
Odpověď: krychle, textura na jedne strana, na ostatnich stranach sediva barva

---

## Icon 387

![icon387](images/tile-full-387.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 6/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tenký svislý tvar (stejná rodina jako 386)

**Co to je?**
Odpověď: bock palace

**Render mód?**
Odpověď: krychle, textura na jedne strane, na ostatnich stranach sediva barva

---

## Icon 388

![icon388](images/tile-full-388.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 4/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tenký svislý tvar (stejná rodina jako 386)

**Co to je?**
Odpověď: bock palace

**Render mód?**
Odpověď: krychle, textura na jedne strane, na ostatnich stranach sediva barva

---

## Icon 389

![icon389](images/tile-full-389.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 5/78 souborů, passable: ne
- Můj dřívější odhad (nejistý): tenký svislý tvar (stejná rodina jako 386)

**Co to je?**
Odpověď: asi fontana

**Render mód?**
Odpověď: krychle, textura na jedne strane, na ostatnich stranach sediva barva

---

## Icon 401

![icon401](images/tile-full-401.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 16/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): velmi slabá šedá větvící se/vyzařující kresba čar, sotva viditelná — nejpoužívanější neidentifikovaná ikona

**Co to je?**
Odpověď: pavucina

**Render mód?**
Odpověď: bude to krychle, textura na jedne strane, na ostatnich stranach pruhledna barva, natoceni krychle (4 strany) bude ulozeno v metadatech bloku

---

## Icon 402

![icon402](images/tile-full-402.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 15/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): velmi slabá šedá větvící se/vyzařující kresba čar (stejná rodina jako 401)

**Co to je?**
Odpověď: pavucina

**Render mód?**
Odpověď: bude to krychle, textura na jedne strane, na ostatnich stranach pruhledna barva, natoceni krychle (4 strany) bude ulozeno v metadatech bloku

---

## Icon 403

![icon403](images/tile-full-403.png)

- Aktuální jméno v katalogu: (unnamed)
- Použití: 15/78 souborů, passable: ano
- Můj dřívější odhad (nejistý): velmi slabá šedá větvící se/vyzařující kresba čar (stejná rodina jako 401)

**Co to je?**
Odpověď: pavucina

**Render mód?**
Odpověď: bude to krychle, textura na jedne strane, na ostatnich stranach pruhledna barva, natoceni krychle (4 strany) bude ulozeno v metadatech bloku

---
