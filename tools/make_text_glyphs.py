#!/usr/bin/env python3
"""Adds the letters Czech, German and French need to text.png (owner Q-023, 2026-11-12).

text.png (Content/icons/, 512x256, and its 2x sheet in Content/icons2x/) is Mobile Eggbert's font:
16 x 8 cells of 32 px, black glyphs on clear texels, with French's 15 lower-case accented letters
(glyphs 15-29). This keeps those eight rows as they are and adds eight more (the sheet becomes
512x512, 1024x1024 at 2x): glyph 128 and on, composed from the sheet's own strokes, in the order of
GLYPHS below, which TextMetrics::kExtraGlyphs repeats.

- a lower-case letter takes its mark from a letter that has it: the acute of é, the grave of è, the
  circumflex of ê, the diaeresis of ë; the caron is the circumflex turned over; the ring is the o made
  small; ď and ť have the caron as an apostrophe beside their stem, as Czech writes them;
- a capital with a mark is drawn a few pixels shorter, so that the mark fits the cell above it (the
  capitals reach its top; the owner's choice, all languages);
- í is the i without its dot.

Run from the repository: python3 tools/make_text_glyphs.py (it reads the eight original rows, so it
can run again). Prints each new glyph's width for TextMetrics::kWidth.
"""
import sys
from PIL import Image, ImageChops

# (character, base character, mark). Marks: acute, grave, circumflex, caron, diaeresis, ring, cedilla,
# apostrophe (the caron of ď and ť).
GLYPHS = [
    ('á', 'a', 'acute'), ('č', 'c', 'caron'), ('ď', 'd', 'apostrophe'), ('ě', 'e', 'caron'),
    ('í', 'ı', 'acute'), ('ň', 'n', 'caron'), ('ó', 'o', 'acute'), ('ř', 'r', 'caron'),
    ('š', 's', 'caron'), ('ť', 't', 'apostrophe'), ('ú', 'u', 'acute'), ('ů', 'u', 'ring'),
    ('ý', 'y', 'acute'), ('ž', 'z', 'caron'),
    ('Á', 'A', 'acute'), ('Č', 'C', 'caron'), ('Ď', 'D', 'caron'), ('É', 'E', 'acute'),
    ('Ě', 'E', 'caron'), ('Í', 'I', 'acute'), ('Ň', 'N', 'caron'), ('Ó', 'O', 'acute'),
    ('Ř', 'R', 'caron'), ('Š', 'S', 'caron'), ('Ť', 'T', 'caron'), ('Ú', 'U', 'acute'),
    ('Ů', 'U', 'ring'), ('Ý', 'Y', 'acute'), ('Ž', 'Z', 'caron'),
    ('Ä', 'A', 'diaeresis'), ('Ö', 'O', 'diaeresis'), ('Ü', 'U', 'diaeresis'),
    ('À', 'A', 'grave'), ('Â', 'A', 'circumflex'), ('Ç', 'C', 'cedilla'), ('È', 'E', 'grave'),
    ('Ê', 'E', 'circumflex'), ('Ë', 'E', 'diaeresis'), ('Î', 'I', 'circumflex'),
    ('Ï', 'I', 'diaeresis'), ('Ô', 'O', 'circumflex'), ('Ù', 'U', 'grave'), ('Û', 'U', 'circumflex'),
]
FIRST = 128
# The original's accented letters (TextMetrics::kAccented, glyphs 15-29).
ACCENTED = 'üàâéèëêïîôùûäöç'
X_HEIGHT = 9     # the lower-case letters' top row (1x)
CAP_TOP = 1      # the capitals' top row
BASELINE = 24    # the row under the letters
CAP_SHORT_TOP = 6  # a capital with a mark starts here


def ascii_widths():
    """TextMetrics::kWidth's first 128 entries (Mobile Eggbert's table_width): the bases' widths."""
    import re
    text = open('src/GalaxyEggbert/Game/TextMetrics.hpp', encoding='utf-8').read()
    table = text[text.index('kWidth['):]
    table = table[table.index('{') + 1:table.index('};')]
    return [int(v) for v in re.findall(r'\d+', table)][:128]


def index_of(ch):
    if ch == 'ı':
        return ord('i')
    if ch in ACCENTED:
        return 15 + ACCENTED.index(ch)
    return ord(ch)


def make(sheet, s):
    """New rows for `sheet` (a 2x sheet when s == 2); returns (image, widths)."""
    cell = 32 * s
    alpha = sheet.getchannel('A')

    def glyph(ch):
        i = index_of(ch)
        g = alpha.crop(((i % 16) * cell, (i // 16) * cell, (i % 16 + 1) * cell, (i // 16 + 1) * cell))
        if ch == 'ı':  # the i without its dot: its stem only
            g = g.copy()
            g.paste(0, (0, 0, cell, X_HEIGHT * s - s))
            g.paste(0, (int(cell * 0.5), 0, cell, cell))
        return g

    def ink(g, top=0, bottom=None):
        """The box of the glyph's ink between rows top and bottom (a faint fringe left out)."""
        bottom = cell if bottom is None else bottom
        part = g.crop((0, top, cell, bottom)).point(lambda v: 255 if v > 60 else 0)
        box = part.getbbox()
        return None if box is None else (box[0], box[1] + top, box[2], box[3] + top)

    def mark(name):
        """The mark alone (cropped to its ink) and its ink's lower edge in its source cell."""
        if name == 'ring':
            o = glyph('o')
            box = ink(o)
            small = o.crop(box)
            w = max(2, round((box[2] - box[0]) * 0.45))
            h = max(2, round((box[3] - box[1]) * 0.45))
            return small.resize((w, h), Image.LANCZOS), (X_HEIGHT - 2) * s
        if name == 'apostrophe':
            q = glyph("'")
            box = ink(q)
            m = q.crop(box)
            return m.resize((max(2, round(m.width * 0.8)), max(2, round(m.height * 0.8))), Image.LANCZOS), 0
        if name == 'cedilla':
            c = glyph('ç')
            box = ink(c, BASELINE * s)
            return c.crop((box[0], BASELINE * s, box[2], box[3])), None
        source = {'acute': 'é', 'grave': 'è', 'circumflex': 'ê', 'caron': 'ê', 'diaeresis': 'ë'}[name]
        g = glyph(source)
        box = ink(g, 0, (X_HEIGHT - 1) * s)
        m = g.crop((box[0], 0, box[2], box[3]))
        m = m.crop((0, box[1], m.width, m.height))
        if name == 'caron':
            m = m.transpose(Image.FLIP_TOP_BOTTOM)
        return m, box[3]

    def union(a, b, at):
        layer = Image.new('L', a.size, 0)
        layer.paste(b, at)
        return ImageChops.lighter(a, layer)

    out = Image.new('L', (16 * cell, 8 * cell), 0)
    widths = []
    for k, (ch, base, name) in enumerate(GLYPHS):
        g = glyph(base)
        width = WIDTH[index_of(base)] * s
        capital = base.isupper()
        body = ink(g, (CAP_TOP if capital else X_HEIGHT) * s, BASELINE * s)
        m, bottom = mark(name)
        if capital and name != 'cedilla':
            # Shorter, so that the mark fits above it: the rows from CAP_TOP to the baseline squeezed
            # into CAP_SHORT_TOP..BASELINE (the width kept), the mark made as small as the room left.
            top, base_row = CAP_TOP * s, BASELINE * s
            letter = g.crop((0, top, cell, base_row)).resize((cell, base_row - CAP_SHORT_TOP * s), Image.LANCZOS)
            squeezed = Image.new('L', (cell, cell), 0)
            squeezed.paste(g.crop((0, base_row, cell, cell)), (0, base_row))  # anything below the baseline
            squeezed.paste(letter, (0, CAP_SHORT_TOP * s))
            g = squeezed
            room = CAP_SHORT_TOP * s - s  # rows 0..CAP_SHORT_TOP-2, one row clear above the letter
            if m.height > room:
                f = room / m.height
                m = m.resize((max(1, round(m.width * f)), room), Image.LANCZOS)
            y = CAP_SHORT_TOP * s - s - m.height
        elif name == 'cedilla':
            y = BASELINE * s
        elif name == 'apostrophe':
            stem = ink(g, 0, (X_HEIGHT - 2) * s)  # the ascender above the body
            x = stem[2] + s
            # Beside the stem's top, and above a t's bar (it starts at the x-height): one row clear.
            y = max(0, min(stem[1], (X_HEIGHT - 2) * s - m.height - s))
            g = union(g, m, (x, y))
            widths.append(max(width, (x + m.width) - 2 * s))
            out.paste(g, ((k % 16) * cell, (k // 16) * cell))
            continue
        else:
            y = bottom - m.height if bottom is not None else 0
        x = (body[0] + body[2]) // 2 - m.width // 2
        g = union(g, m, (x, y))
        widths.append(width)
        out.paste(g, ((k % 16) * cell, (k // 16) * cell))
    rgba = Image.new('RGBA', out.size, (0, 0, 0, 0))
    rgba.putalpha(out)
    return rgba, widths


WIDTH = None


def main():
    global WIDTH
    WIDTH = ascii_widths()
    widths1 = None
    for path, s in (('Content/icons/text.png', 1), ('Content/icons2x/text.png', 2)):
        sheet = Image.open(path).convert('RGBA')
        original = sheet.crop((0, 0, 512 * s, 256 * s))
        rows, widths = make(original, s)
        whole = Image.new('RGBA', (512 * s, 512 * s), (0, 0, 0, 0))
        whole.paste(original, (0, 0))
        whole.paste(rows, (0, 256 * s))
        whole.save(path, optimize=True)
        if s == 1:
            widths1 = widths
    for k, ((ch, _, _), w) in enumerate(zip(GLYPHS, widths1)):
        print(f'{FIRST + k}\t{ch}\t{w}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
