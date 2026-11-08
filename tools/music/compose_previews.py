#!/usr/bin/env python3
"""Compose ten original General MIDI listening previews, outside the game's Content tree.

No reference melodies, scores or samples are copied. Each piece has its own written hook,
harmonic plan, instrumentation and groove, with introduction, A/A', B/B', reprise and coda.
Uses the existing render_music.py / system GM soundfont when --render is requested.
"""
from __future__ import annotations

import argparse
import json
import random
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

PPQ = 480
MAJOR = (0, 2, 4, 5, 7, 9, 11)
MINOR = (0, 2, 3, 5, 7, 8, 11)
DORIAN = (0, 2, 3, 5, 7, 9, 10)


@dataclass(frozen=True)
class Piece:
    number: int
    title: str
    bpm: int
    beats: int
    tonic: int
    scale: tuple[int, ...]
    patches: tuple[int, ...]  # GM numbers are zero-based: lead, B solo, comp, bass, answer, pad
    groove: str
    harmony_a: tuple[int, ...]
    harmony_b: tuple[int, ...]
    hook: tuple[tuple[int, ...], ...]
    mood: str


PIECES = (
    Piece(10, "Jarní výprava", 116, 4, 60, MAJOR, (71, 73, 0, 32, 11, 48), "bounce",
          (0, 5, 1, 4, 0, 3, 4, 0), (3, 0, 1, 4, 5, 1, 4, 4),
          ((0,2,4,2,1,2,4,-1), (2,4,5,4,2,1,0,-1), (0,1,2,4,2,0,1,2),
           (4,2,1,0,1,2,1,-1), (0,2,4,7,6,4,2,1), (2,0,1,2,4,2,0,-1),
           (1,2,4,2,1,0,1,2), (4,2,1,0,0,-1,-1,-1)), "Hravá procházka; klarinet, flétna a klavír."),
    Piece(11, "Sýrový foxtrot", 108, 4, 62, DORIAN, (66, 56, 1, 33, 26, 48), "swing",
          (0, 3, 0, 4, 5, 1, 4, 0), (3, 6, 2, 5, 1, 4, 1, 4),
          ((0,-1,2,4,5,4,2,1), (2,4,2,-1,0,1,2,-1), (0,2,3,4,2,1,0,2),
           (4,-1,2,1,0,1,2,-1), (2,4,6,5,4,2,1,0), (0,1,2,-1,4,2,1,-1),
           (2,4,2,1,0,-1,1,2), (4,2,1,0,-1,0,-1,-1)), "Houpavý foxtrot se saxofonem, trubkou a kráčející basou."),
    Piece(12, "Hodinový strojek", 102, 3, 67, MAJOR, (8, 10, 45, 43, 13, 48), "clock",
          (0, 4, 5, 2, 3, 0, 4, 0), (5, 2, 3, 4, 1, 5, 4, 4),
          ((0,4,2,4,1,4), (2,5,4,2,1,0), (0,2,4,5,4,2), (4,2,1,0,2,-1),
           (2,4,7,6,4,2), (0,2,1,2,4,-1), (1,4,2,4,1,2), (4,2,0,-1,0,-1)),
          "Hračkový valčík; celesta, zvonkohra, pizzicato a dřevěné perkuse."),
    Piece(13, "Modrá zátoka", 84, 3, 62, MAJOR, (11, 73, 4, 35, 89, 48), "water",
          (0, 5, 3, 4, 2, 5, 1, 4), (3, 4, 2, 5, 1, 0, 4, 4),
          ((0,-1,2,4,2,-1), (4,5,4,2,-1,0), (2,-1,4,7,6,4), (2,1,0,-1,1,-1),
           (0,2,4,-1,5,4), (2,-1,1,0,2,-1), (1,2,4,2,1,-1), (2,1,0,-1,0,-1)),
          "Klidná podvodní kolébavka; vibrafon, flétna a elektrické piano."),
    Piece(14, "Továrna na pružiny", 126, 4, 65, MINOR, (7, 62, 2, 34, 12, 50), "machine",
          (0, 0, 3, 4, 5, 3, 4, 0), (5, 6, 3, 4, 1, 4, 5, 4),
          ((0,0,4,-1,2,4,2,-1), (0,2,4,2,0,-1,1,2), (2,-1,4,2,1,0,2,-1),
           (4,2,1,-1,0,1,2,4), (0,4,5,4,2,0,2,4), (2,1,0,-1,2,4,2,-1),
           (1,1,4,2,1,0,1,2), (4,2,0,-1,0,-1,0,-1)), "Pružný tovární rytmus; clavinet, syntezátorová žesť a basová kytara."),
    Piece(15, "Ostrov mezi mraky", 110, 4, 65, MAJOR, (73, 68, 24, 32, 9, 48), "cloud",
          (0, 3, 5, 4, 0, 2, 3, 4), (5, 2, 3, 0, 1, 5, 3, 4),
          ((0,2,4,-1,7,6,4,2), (2,4,5,4,2,-1,0,2), (0,1,2,4,5,4,2,-1),
           (4,2,1,0,-1,1,2,4), (0,4,7,6,5,4,2,-1), (2,4,2,1,0,1,2,-1),
           (1,2,4,7,6,4,2,1), (4,2,1,0,-1,0,-1,-1)), "Lehká dobrodružná melodie; flétna, hoboj, nylonová kytara a zvonky."),
    Piece(16, "Palácové schodiště", 100, 3, 60, MAJOR, (6, 68, 0, 42, 45, 48), "palace",
          (0, 4, 5, 2, 3, 1, 4, 0), (5, 2, 3, 0, 1, 5, 4, 4),
          ((0,2,4,2,1,0), (2,4,5,4,2,-1), (0,1,2,4,2,0), (4,2,1,-1,2,-1),
           (2,4,7,6,4,2), (0,2,4,2,1,-1), (1,2,4,2,1,0), (4,2,0,-1,0,-1)),
          "Komorní palácový valčík; cembalo, hoboj a smyčce."),
    Piece(17, "Jeskyně světlušek", 92, 4, 69, MINOR, (10, 71, 46, 43, 14, 89), "cave",
          (0, 5, 3, 4, 0, 3, 1, 4), (5, 3, 6, 2, 1, 4, 0, 4),
          ((0,-1,4,2,-1,1,2,-1), (2,4,5,-1,4,2,0,-1), (0,2,-1,4,6,4,2,-1),
           (4,-1,2,1,0,-1,1,-1), (0,4,7,-1,6,4,2,0), (2,-1,4,2,1,0,2,-1),
           (1,2,-1,4,2,1,0,1), (4,2,1,-1,0,-1,0,-1)), "Tajemná, ale přátelská jeskyně; zvonkohra, klarinet a harfa."),
    Piece(18, "Veselá honička", 136, 4, 67, MAJOR, (56, 65, 27, 33, 1, 61), "chase",
          (0, 3, 0, 4, 5, 1, 4, 0), (3, 0, 5, 2, 1, 5, 4, 4),
          ((0,2,4,5,4,2,0,2), (2,4,5,4,2,1,0,-1), (0,0,2,4,6,5,4,2),
           (4,2,1,0,1,2,4,-1), (0,4,7,6,5,4,2,1), (2,4,2,0,1,2,4,-1),
           (1,2,4,2,1,0,1,2), (4,2,1,0,0,-1,0,-1)), "Svižná žesťová honička; trubka, saxofon, kytara a kontrabas."),
    Piece(19, "Noční hvězdná zahrada", 98, 4, 63, MAJOR, (9, 69, 4, 35, 46, 48), "night",
          (0, 5, 2, 4, 3, 0, 1, 4), (5, 2, 3, 0, 1, 5, 4, 4),
          ((0,-1,2,4,6,4,2,-1), (2,4,5,-1,4,2,0,-1), (0,1,2,-1,4,2,1,0),
           (4,2,-1,1,0,-1,2,-1), (0,2,4,7,6,-1,4,2), (2,-1,4,2,1,0,1,-1),
           (1,2,4,-1,2,1,0,1), (4,2,1,0,-1,0,-1,-1)), "Poklidné noční finále; zvonky, anglický roh, elektrické piano a harfa."),
)


def vlq(value: int) -> bytes:
    data = [value & 127]
    while value >> 7:
        value >>= 7
        data.insert(0, (value & 127) | 128)
    return bytes(data)


def meta(kind: int, data: bytes) -> bytes:
    return bytes((255, kind)) + vlq(len(data)) + data


def write_midi(piece: Piece, path: Path) -> dict:
    rng = random.Random(71000 + piece.number)
    tracks: list[list[tuple[int, int, bytes]]] = [[] for _ in range(7)]
    notes = [0] * 7
    bars = 48
    length = bars * piece.beats
    conductor = [(0, 0, meta(3, piece.title.encode('utf-8'))),
                 (0, 1, meta(81, round(60000000 / piece.bpm).to_bytes(3, 'big'))),
                 (0, 2, meta(88, bytes((piece.beats, 2, 24, 8)))),
                 (0, 3, meta(1, b'Original Galaxy Eggbert preview; not assigned to any world.'))]
    channels = (0, 1, 2, 3, 4, 5, 9)
    names = ('Melody', 'Accompaniment', 'Bass', 'Answers', 'Strings', 'Bells', 'Percussion')
    patches = (piece.patches[0], piece.patches[2], piece.patches[3], piece.patches[4], piece.patches[5], 11, 0)
    for i, (channel, patch, name) in enumerate(zip(channels, patches, names)):
        tracks[i].append((0, 0, meta(3, name.encode())))
        for ctrl, value in ((7, (88, 64, 80, 66, 40, 42, 67)[i]), (10, (64, 42, 64, 90, 64, 76, 64)[i]),
                            (91, 35), (93, 12)):
            tracks[i].append((0, 1, bytes((176 | channel, ctrl, value))))
        tracks[i].append((0, 1, bytes((192 | channel, patch))))

    def pitch(degree: int, octave: int = 0) -> int:
        return piece.tonic + piece.scale[degree % 7] + 12 * (degree // 7 + octave)

    def note(track: int, beat: float, duration: float, key: int, velocity: int):
        start = max(0, round(beat * PPQ))
        stop = max(start + 1, round((beat + duration) * PPQ))
        velocity = max(1, min(115, velocity + rng.randrange(-3, 4)))
        channel = channels[track]
        tracks[track] += [(start, 3, bytes((144 | channel, key, velocity))),
                          (stop, 2, bytes((128 | channel, key, 0)))]
        notes[track] += 1

    def melody_pitch(degree: int, last: int) -> int:
        key = pitch(degree)
        choices = [key + shift for shift in (-24, -12, 0, 12, 24) if 64 <= key + shift <= 85]
        return min(choices, key=lambda k: abs(k-last))

    previous = 72
    for bar in range(bars):
        b_section = 20 <= bar < 36
        intro = bar < 4
        coda = bar >= 44
        pattern_bar = (bar - 4) % 8
        harmony = piece.harmony_b if b_section else piece.harmony_a
        root = harmony[pattern_bar]
        if coda:
            root = (3, 4, 0, 0)[bar - 44]
        beat = bar * piece.beats
        energy = 0.72 if intro or coda else (1.0 if b_section else 0.9)
        if bar in (20, 36):
            tracks[0].append((round(beat*PPQ), 1, bytes((192, piece.patches[1] if bar == 20 else piece.patches[0]))))
        # Bass and accompaniment have deliberate differences between dance, water and toy cues.
        bass_pattern = (0, 4, 2, 4) if piece.groove not in ('swing', 'chase') else (0, 2, 4, 5)
        for step in range(piece.beats):
            if coda and bar == 47 and step > 0:
                continue
            note(2, beat + step, 0.8 if piece.groove in ('water','night') else 0.65,
                 pitch(root + bass_pattern[step % 4], -2), round(76*energy))
        for step in range(piece.beats):
            if coda and bar == 47 and step > 0:
                continue
            if piece.groove in ('cloud', 'water', 'cave', 'night', 'clock', 'palace'):
                for half in range(2):
                    degree = root + (0, 2, 4, 2, 6, 4, 2, 4)[(step*2+half) % 8]
                    note(1, beat+step+half*.5, .42, pitch(degree, -1), round(57*energy))
            elif step % 2 or piece.groove == 'machine':
                for degree in (root, root+2, root+4):
                    note(1, beat+step+(.12 if piece.groove=='swing' else .04), .34,
                         pitch(degree, -1), round(58*energy))
        for degree in (root, root+2, root+4):
            note(4, beat+.04, piece.beats*.92, pitch(degree, -1), round(42*energy))
        if not intro and not coda:
            hook = piece.hook[(pattern_bar + (4 if b_section else 0)) % 8]
            for i, degree in enumerate(hook):
                if degree < 0:
                    continue
                # B answers A with its own reversed contour and higher chord colour.
                if b_section:
                    degree = 4 - degree if i % 3 else degree + 2
                degree += root
                key = melody_pitch(degree, previous)
                previous = key
                at = i*.5
                if piece.groove == 'swing' and i % 2:
                    at += .13
                note(0, beat + at, .37 if piece.groove not in ('water','night') else .62,
                     key, round((79 if i%2==0 else 69)*energy))
            # Sparse replies leave breathing space around the hook.
            if bar % 4 == 3 or 12 <= bar < 20 or 28 <= bar < 36:
                for j, degree in enumerate((root+4, root+2, root)):
                    note(3, beat+piece.beats-1.5+j*.5, .4, pitch(degree), 57)
        elif bar >= 2:
            cadence = (root+4, root+2, root) if bar != 47 else (0,)
            for j, degree in enumerate(cadence):
                note(0, beat+j*.75, 2.3 if bar==47 else .6, pitch(degree), 66)
        if piece.groove in ('water', 'night', 'cave'):
            if bar % 2 == 0:
                note(5, beat, 1.4, pitch(root+7), 43)
                note(6, beat+piece.beats-0.5, .1, 81, 38)
        elif not (coda and bar == 47):
            for half in range(piece.beats*2):
                at=half*.5 + (.13 if piece.groove=='swing' and half%2 else 0)
                note(6, beat+at, .08, 42 if piece.groove not in ('clock','palace') else 76,
                     round((39 if half%2 else 48)*energy))
            note(6, beat, .15, 36, round(69*energy))
            for step in range(1, piece.beats, 2):
                note(6, beat+step, .12, 38 if piece.groove not in ('clock','palace') else 37, round(62*energy))
            if bar % 8 == 7:
                note(6, beat+piece.beats-.5, .15, 45, 48)
                note(6, beat+piece.beats-.25, .12, 47, 48)

    def chunk(events):
        out=bytearray(); last=0
        for tick, order, data in sorted(events, key=lambda e: (e[0], e[1])):
            out += vlq(tick-last) + data; last=tick
        end = max(last, round((length+3)*PPQ))
        out += vlq(end-last) + meta(47, b'')
        return b'MTrk' + struct.pack('>I',len(out)) + out

    path.write_bytes(b'MThd'+struct.pack('>IHHH',6,1,8,PPQ)+chunk(conductor)+b''.join(chunk(t) for t in tracks))
    return dict(id=piece.number, title=piece.title, bpm=piece.bpm, meter=f'{piece.beats}/4',
                seconds=round((length+3)*60/piece.bpm,2), mood=piece.mood, note_counts=notes,
                midi=path.name, audio=path.with_suffix('.ogg').name)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,default=Path('music-previews/retro-010-019'))
    ap.add_argument('--render',action='store_true')
    args=ap.parse_args(); args.out.mkdir(parents=True,exist_ok=True)
    manifest=[]
    for piece in PIECES:
        manifest.append(write_midi(piece,args.out/f'music{piece.number:03}.mid'))
    (args.out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    (args.out/'playlist.m3u').write_text('#EXTM3U\n'+''.join(
        f'#EXTINF:{round(t["seconds"])},{t["id"]:03} - {t["title"]}\n{t["audio"]}\n' for t in manifest))
    lines=['GALAXY EGGBERT — DESET NOVÝCH HUDEBNÍCH NÁVRHŮ', '',
           'Původní melodie a aranže pro dobový General MIDI zvuk; nejsou to ztracené skladby Daniela Rouxe.',
           'Žádná skladba není přiřazena světu ani zapojena do hry. Otevřete playlist.m3u nebo jednotlivé OGG.',
           'MIDI soubory jsou přiloženy pro případné úpravy. Hudba neobsahuje hlas ani převzaté melodie.', '',
           'Zvuk: stejná systémová GM banka a render_music.py jako u stávajících skladeb.', '']
    lines += [f'{t["id"]:03} — {t["title"]} ({t["bpm"]} BPM, {t["meter"]})\n    {t["mood"]}' for t in manifest]
    (args.out/'README.txt').write_text('\n'.join(lines)+'\n')
    if args.render:
        subprocess.run([sys.executable,'-u',str(Path(__file__).with_name('render_music.py')),
                        *[str(args.out/t['midi']) for t in manifest],'--out',str(args.out),'--ogg'],check=True)
    print(f'Prepared {len(manifest)} original previews in {args.out}')


if __name__=='__main__':
    main()
