#!/usr/bin/env python3
"""Adds the letters the game's languages need to text.png (owner Q-023, 2026-11-12).

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
from PIL import Image, ImageChops, ImageDraw

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
    # 171 on (2026-11-12): Slovak, Spanish, Italian, Polish, Portuguese and the degree sign.
    ('ĺ', 'l', 'acute'), ('ľ', 'l', 'apostrophe'), ('ŕ', 'r', 'acute'), ('Ĺ', 'L', 'acute'),
    ('Ľ', 'L', 'apostrophe'), ('Ŕ', 'R', 'acute'),
    ('ñ', 'n', 'tilde'), ('Ñ', 'N', 'tilde'), ('¿', '?', 'rotate'), ('¡', '!', 'rotate'),
    ('ì', 'ı', 'grave'), ('ò', 'o', 'grave'), ('Ì', 'I', 'grave'), ('Ò', 'O', 'grave'),
    ('ą', 'a', 'ogonek'), ('ć', 'c', 'acute'), ('ę', 'e', 'ogonek'), ('ł', 'l', 'stroke'),
    ('ń', 'n', 'acute'), ('ś', 's', 'acute'), ('ź', 'z', 'acute'), ('ż', 'z', 'dot'),
    ('Ą', 'A', 'ogonek'), ('Ć', 'C', 'acute'), ('Ę', 'E', 'ogonek'), ('Ł', 'L', 'stroke'),
    ('Ń', 'N', 'acute'), ('Ś', 'S', 'acute'), ('Ź', 'Z', 'acute'), ('Ż', 'Z', 'dot'),
    ('ã', 'a', 'tilde'), ('õ', 'o', 'tilde'), ('Ã', 'A', 'tilde'), ('Õ', 'O', 'tilde'),
    ('°', ' ', 'degree'),
    # 206 on (2026-11-12): Hungarian, Romanian, Swedish; then Russian's and Ukrainian's letters that
    # are not Latin ones (а е ё о р с у х і ї and А В Е Ё К М Н О Р С Т Х І Ї are drawn by the Latin
    # glyphs, TextMetrics::GlyphOf), rendered from Comic Relief ('@': see render()).
    ('ő', 'o', 'dacute'), ('ű', 'u', 'dacute'), ('Ő', 'O', 'dacute'), ('Ű', 'U', 'dacute'),
    ('ă', 'a', 'breve'), ('Ă', 'A', 'breve'), ('ș', 's', 'commabelow'), ('ț', 't', 'commabelow'),
    ('Ș', 'S', 'commabelow'), ('Ț', 'T', 'commabelow'), ('å', 'a', 'ring'), ('Å', 'A', 'ring'),
] + [(ch, '@' + ch, 'plain') for ch in 'бвгджзийклмнптфцчшщъыьэюяєґ'] + \
    [(ch, '@' + ch, 'plain') for ch in 'БГДЖЗИЛПУФЦЧШЩЪЫЬЭЮЯЄҐ'] + [('Й', '@И', 'breve')] + \
    [
    # 268 on (2026-11-12): Danish and Norwegian's æ ø (rendered: the sheet has no such strokes),
    # Turkish's letters composed (ı the dotless i, İ the I with the i's dot), then Greek's letters that
    # look like no Latin one (Α Β Ε Ζ Η Ι Κ Μ Ν Ο Ρ Τ Υ Χ ο, ό ί ϊ are drawn by Latin glyphs,
    # TextMetrics::GlyphOf), rendered from Comic Relief.
    ('æ', '@æ', 'plain'), ('Æ', '@Æ', 'plain'), ('ø', '@ø', 'plain'), ('Ø', '@Ø', 'plain'),
    ('ğ', 'g', 'breve'), ('Ğ', 'G', 'breve'), ('ı', 'ı', 'plain'), ('İ', 'I', 'dot'),
    ('ş', 's', 'cedilla'), ('Ş', 'S', 'cedilla'),
] + [(ch, '@' + ch, 'plain') for ch in 'αβγδεζηθικλμνξπρσςτυφχψωάέήύώϋ'] + \
    [(ch, '@' + ch, 'plain') for ch in 'ΓΔΘΛΞΠΣΦΨΩΆΈΉΊΌΎΏ']
# The marks drawn above the letter (a capital, or an l, made shorter for them).
ABOVE = {'acute', 'grave', 'circumflex', 'caron', 'diaeresis', 'ring', 'tilde', 'dot', 'dacute', 'breve'}
SHEET_ROWS = 24  # the whole sheet: the original's 8 rows and Galaxy's 16

# The Cyrillic letters text.png has no likeness of are drawn from Comic Relief (SIL Open Font
# License; Comic Sans's metrics, the shapes text.png's glyphs have), Regular: its stroke inks as much
# as the sheet's (H: 676 against 655 at 2x). Not kept in the repository: fetched once into
# ~/deps/fonts/ (COMIC_RELIEF overrides) from github.com/google/fonts, ofl/comicrelief/.
import os
FONT = os.environ.get('COMIC_RELIEF', os.path.expanduser('~/deps/fonts/ComicRelief-Regular.ttf'))
CAP_SIZE, LOWER_SIZE = 61, 56  # at 2x: H's height 46 as the sheet's capitals, x 30 as its x-height
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

    def render(ch):
        """@p ch from Comic Relief: drawn at 2x on the sheet's baseline, its ink from the sheet's left
        bearing, scaled down for the 1x sheet."""
        from PIL import ImageFont
        font = ImageFont.truetype(FONT, CAP_SIZE if ch.isupper() else LOWER_SIZE)
        big = Image.new('L', (64, 64), 0)
        ImageDraw.Draw(big).text((0, 0), ch, font=font, fill=255)
        box = big.getbbox()
        ImageDraw.Draw(big).rectangle((0, 0, 63, 63), fill=0)
        top_of_h = ImageFont.truetype(FONT, CAP_SIZE if ch.isupper() else LOWER_SIZE).getbbox('H')
        ImageDraw.Draw(big).text((4 - box[0], BASELINE * 2 - top_of_h[3]), ch, font=font, fill=255)
        return big if s == 2 else big.resize((32, 32), Image.LANCZOS)

    def glyph(ch):
        if ch.startswith('@'):
            return render(ch[1:])
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
        if name in ('cedilla', 'ogonek'):
            c = glyph('ç')
            box = ink(c, BASELINE * s)
            m = c.crop((box[0], BASELINE * s, box[2], box[3]))
            # The ogonek is the cedilla's hook turned the other way, at the letter's right.
            return (m.transpose(Image.FLIP_LEFT_RIGHT) if name == 'ogonek' else m), None
        if name == 'dacute':
            # Hungarian's double acute: two acutes, smaller, side by side.
            g = glyph('é')
            box = ink(g, 0, (X_HEIGHT - 1) * s)
            one = g.crop(box)
            one = one.resize((max(1, round(one.width * 0.75)), max(1, round(one.height * 0.85))), Image.LANCZOS)
            gap = max(1, round(one.width * 0.55))
            pair = Image.new('L', (one.width + gap, one.height), 0)
            pair = ImageChops.lighter(pair, Image.new('L', pair.size, 0))
            pair.paste(one, (0, 0))
            pair = ImageChops.lighter(pair, Image.new('L', pair.size, 0))
            second = Image.new('L', pair.size, 0)
            second.paste(one, (gap, 0))
            return ImageChops.lighter(pair, second), box[3]
        if name == 'breve':
            # Romanian's and Russian's breve: Comic Relief's, from its ă above the x-height.
            g = render('ă')
            box = ink(g, 0, (X_HEIGHT - 1) * s)
            return g.crop(box), box[3]
        if name == 'commabelow':
            c = glyph(',')
            box = ink(c)
            m = c.crop(box)
            return m.resize((max(1, round(m.width * 0.85)), max(1, round(m.height * 0.85))), Image.LANCZOS), None
        if name == 'tilde':
            t = glyph('~')
            m = t.crop(ink(t))
            h = min(m.height, 5 * s)
            return m.resize((m.width, h), Image.LANCZOS) if h != m.height else m, (X_HEIGHT - 2) * s
        if name == 'dot':
            i = glyph('i')
            box = ink(i, 0, (X_HEIGHT - 2) * s)
            box = (box[0], box[1], min(box[2], box[0] + 6 * s), box[3])  # the dot, not the sheet's stray right edge
            return i.crop(box), box[3]
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

    out = Image.new('L', (16 * cell, (SHEET_ROWS - 8) * cell), 0)
    widths = []
    for k, (ch, base, name) in enumerate(GLYPHS):
        at = ((k % 16) * cell, (k // 16) * cell)
        g = glyph(base)
        if base.startswith('@'):
            drawn = ink(g)  # a rendered letter's width from its ink, as the sheet's (right edge - 2)
            width = max(1, (drawn[2] if drawn else 2 * s) // s - 2) * s
        else:
            width = WIDTH[index_of(base)] * s
        capital = base.isupper()
        if name == 'rotate':
            # ¿ and ¡: ? and ! turned over, hanging from the x-height down below the baseline.
            box = ink(g)
            piece = g.crop(box).rotate(180)
            turned = Image.new('L', (cell, cell), 0)
            turned.paste(piece, (box[0], min((X_HEIGHT - 1) * s, cell - piece.height)))
            widths.append(width)
            out.paste(turned, at)
            continue
        if name == 'plain':
            widths.append(width)
            out.paste(g, at)
            continue
        if name == 'commabelow':
            body = ink(g, X_HEIGHT * s, BASELINE * s) if not capital else ink(g, CAP_TOP * s, BASELINE * s)
            m, _ = mark(name)
            g = union(g, m, (max(0, (body[0] + body[2]) // 2 - m.width // 2), BASELINE * s + s))
            widths.append(width)
            out.paste(g, at)
            continue
        if name == 'degree':
            m, _ = mark('ring')
            g = union(Image.new('L', (cell, cell), 0), m, (2 * s, 1 * s))
            widths.append(m.width + 2 * s)
            out.paste(g, at)
            continue
        if name == 'stroke':
            # ł and Ł: a bar across the stem, as thick as the stem, rising to the right.
            top = (CAP_TOP if capital else 0) * s
            stem = ink(g, top, (X_HEIGHT + 4) * s)
            thick = max(2 * s, stem[2] - stem[0])
            cx = (stem[0] + stem[2]) / 2.0
            cy = (12 if capital else 13) * s
            k4 = 4
            big = Image.new('L', (cell * k4, cell * k4), 0)
            ImageDraw.Draw(big).line([((cx - 4.5 * s) * k4, (cy + 2.5 * s) * k4), ((cx + 4.5 * s) * k4, (cy - 2.5 * s) * k4)],
                                     fill=255, width=int(thick * 0.8 * k4))
            bar = big.resize((cell, cell), Image.LANCZOS)
            g = ImageChops.lighter(g, bar)
            widths.append(max(width, int(cx + 4.5 * s + 1) - 2 * s))
            out.paste(g, at)
            continue
        tall = name in ABOVE and (capital or base == 'l')
        body = ink(g, (CAP_TOP if capital else X_HEIGHT) * s, BASELINE * s)
        m, bottom = mark(name)
        if tall:
            # Shorter, so that the mark fits above it: the rows from the letter's top to the baseline
            # squeezed into CAP_SHORT_TOP..BASELINE (the width kept), the mark made as small as the
            # room left.
            top, base_row = (CAP_TOP * s if capital else ink(g)[1]), BASELINE * s
            letter = g.crop((0, top, cell, base_row)).resize((cell, base_row - CAP_SHORT_TOP * s), Image.LANCZOS)
            squeezed = Image.new('L', (cell, cell), 0)
            squeezed.paste(g.crop((0, base_row, cell, cell)), (0, base_row))  # anything below the baseline
            squeezed.paste(letter, (0, CAP_SHORT_TOP * s))
            g = squeezed
            if base == 'l':
                body = ink(g, CAP_SHORT_TOP * s, BASELINE * s)
            room = CAP_SHORT_TOP * s - s  # rows 0..CAP_SHORT_TOP-2, one row clear above the letter
            if m.height > room:
                f = room / m.height
                m = m.resize((max(1, round(m.width * f)), room), Image.LANCZOS)
            y = CAP_SHORT_TOP * s - s - m.height
        elif name == 'cedilla':
            y = BASELINE * s
        elif name == 'ogonek':
            letter = ink(g)
            g = union(g, m, (letter[2] - m.width - s, BASELINE * s - 2 * s))
            widths.append(width)
            out.paste(g, at)
            continue
        elif name == 'apostrophe':
            stem = ink(g, 0, (X_HEIGHT - 2) * s)  # the ascender above the body
            x = stem[2] + s
            # Beside the stem's top, and above a t's bar (it starts at the x-height): one row clear.
            y = max(0, min(stem[1], (X_HEIGHT - 2) * s - m.height - s))
            g = union(g, m, (x, y))
            widths.append(max(width, (x + m.width) - 2 * s))
            out.paste(g, at)
            continue
        else:
            y = bottom - m.height if bottom is not None else 0
        x = max(0, (body[0] + body[2]) // 2 - m.width // 2)
        g = union(g, m, (x, y))
        widths.append(width)
        out.paste(g, at)
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
        whole = Image.new('RGBA', (512 * s, 32 * SHEET_ROWS * s), (0, 0, 0, 0))
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
