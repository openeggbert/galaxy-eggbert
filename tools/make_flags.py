#!/usr/bin/env python3
"""The flags of the game's languages for the main menu's language button (the owner, 2026-11-12).

Content/icons/flags.png (and its 2x sheet in Content/icons2x/): one cell a language, in
Localization.hpp's kLanguages order: English (the Union Flag), French, German, Czech, Slovak, Spanish,
Italian, Polish, Portuguese (Brazil's flag: the game's Portuguese is Brazil's), Dutch, Hungarian,
Romanian, Swedish, Russian, Ukrainian, Finnish, Danish, Norwegian, Turkish, Greek. Each cell is
48x32 (96x64 at 2x), the flag drawn from its geometry (no picture copied), eight times as large and
scaled down, with a thin dark edge. Run from the repository: python3 tools/make_flags.py
"""
import sys
from PIL import Image, ImageChops, ImageDraw

CELL_W, CELL_H = 48, 32
SUPER = 8

BLUE_UK, RED_UK = (1, 33, 105), (200, 16, 46)
BLUE_FR, RED_FR = (0, 85, 164), (239, 65, 53)
BLACK_DE, RED_DE, GOLD_DE = (0, 0, 0), (221, 0, 0), (255, 206, 0)
BLUE_CZ, RED_CZ = (17, 69, 126), (215, 20, 26)
WHITE = (255, 255, 255)


def union_flag(d, w, h):
    """The Union Flag on a w x h field: its 60 x 30 geometry spread over the field, the red saltire
    counterchanged (its 4-unit stroke clipped to four triangles, a 2-unit band each side in turn)."""
    import math
    sx, sy = w / 60.0, h / 30.0

    def stripe(draw, x0, y0, x1, y1, width, colour):
        dx, dy = (x1 - x0), (y1 - y0)
        length = math.hypot(dx, dy)
        nx, ny = -dy / length * width / 2, dx / length * width / 2
        pts = [(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)]
        draw.polygon([(px * sx, py * sy) for px, py in pts], fill=colour)

    d.rectangle((0, 0, w, h), fill=BLUE_UK)
    stripe(d, 0, 0, 60, 30, 6, WHITE)
    stripe(d, 60, 0, 0, 30, 6, WHITE)
    red = Image.new('L', (int(w), int(h)), 0)
    rd = ImageDraw.Draw(red)
    stripe(rd, 0, 0, 60, 30, 4, 255)
    stripe(rd, 60, 0, 0, 30, 4, 255)
    clip = Image.new('L', (int(w), int(h)), 0)
    cd = ImageDraw.Draw(clip)
    for tri in [((30, 15), (60, 15), (60, 30)), ((30, 15), (30, 30), (0, 30)),
                ((30, 15), (0, 15), (0, 0)), ((30, 15), (30, 0), (60, 0))]:
        cd.polygon([(px * sx, py * sy) for px, py in tri], fill=255)
    from PIL import ImageChops
    d._image.paste(RED_UK, (0, 0), ImageChops.multiply(red, clip))
    d.rectangle((25 * sx, 0, 35 * sx, h), fill=WHITE)
    d.rectangle((0, 10 * sy, w, 20 * sy), fill=WHITE)
    d.rectangle((27 * sx, 0, 33 * sx, h), fill=RED_UK)
    d.rectangle((0, 12 * sy, w, 18 * sy), fill=RED_UK)


def french(d, w, h):
    for i, colour in enumerate((BLUE_FR, WHITE, RED_FR)):
        d.rectangle((i * w / 3.0, 0, (i + 1) * w / 3.0, h), fill=colour)


def german(d, w, h):
    for i, colour in enumerate((BLACK_DE, RED_DE, GOLD_DE)):
        d.rectangle((0, i * h / 3.0, w, (i + 1) * h / 3.0), fill=colour)


def czech(d, w, h):
    d.rectangle((0, 0, w, h / 2.0), fill=WHITE)
    d.rectangle((0, h / 2.0, w, h), fill=RED_CZ)
    d.polygon([(0, 0), (w / 2.0, h / 2.0), (0, h)], fill=BLUE_CZ)


BLUE_SK, RED_SK = (11, 78, 162), (238, 28, 37)
RED_ES, YELLOW_ES = (170, 21, 27), (241, 191, 0)
GREEN_IT, RED_IT = (0, 146, 70), (206, 43, 55)
RED_PL = (220, 20, 60)
GREEN_BR, YELLOW_BR, BLUE_BR = (0, 156, 59), (255, 223, 0), (0, 39, 118)


def slovak(d, w, h):
    """White, blue, red; the arms (a white double cross on three blue hills, a red shield edged white)
    towards the hoist, as small as the flag allows."""
    for i, colour in enumerate((WHITE, BLUE_SK, RED_SK)):
        d.rectangle((0, i * h / 3.0, w, (i + 1) * h / 3.0), fill=colour)
    sx, sy = w / 9.0, h / 6.0  # the flag's 9 x 6 units
    x0, x1, y0, y1 = 1.4 * sx, 3.8 * sx, 1.3 * sy, 4.9 * sy

    def shield_points(pad):
        cx = (x0 + x1) / 2.0
        return [(x0 - pad, y0 - pad), (x1 + pad, y0 - pad), (x1 + pad, y0 + (y1 - y0) * 0.55),
                (cx, y1 + pad * 1.4), (x0 - pad, y0 + (y1 - y0) * 0.55)]
    d.polygon(shield_points(0.18 * sx), fill=WHITE)
    d.polygon(shield_points(0), fill=RED_SK)
    cx = (x0 + x1) / 2.0
    # The three hills, inside the shield.
    hills = Image.new('L', (int(w), int(h)), 0)
    hd = ImageDraw.Draw(hills)
    for hx, hy, r in ((cx - 0.62 * sx, y1 - 0.55 * sy, 0.5 * sx), (cx + 0.62 * sx, y1 - 0.55 * sy, 0.5 * sx),
                      (cx, y1 - 0.8 * sy, 0.55 * sx)):
        hd.ellipse((hx - r, hy - r * 0.75, hx + r, hy + r * 1.6), fill=255)
    inside = Image.new('L', (int(w), int(h)), 0)
    ImageDraw.Draw(inside).polygon(shield_points(0), fill=255)
    d._image.paste(BLUE_SK, (0, 0), ImageChops.multiply(hills, inside))
    # The double cross.
    t = 0.16 * sx
    d.rectangle((cx - t, y0 + 0.35 * sy, cx + t, y1 - 1.3 * sy), fill=WHITE)
    d.rectangle((cx - 0.55 * sx, y0 + 0.75 * sy, cx + 0.55 * sx, y0 + 0.75 * sy + 2 * t), fill=WHITE)
    d.rectangle((cx - 0.8 * sx, y0 + 1.45 * sy, cx + 0.8 * sx, y0 + 1.45 * sy + 2 * t), fill=WHITE)


def spanish(d, w, h):
    """Red, yellow (twice as wide), red: the civil flag, without the arms."""
    d.rectangle((0, 0, w, h), fill=RED_ES)
    d.rectangle((0, h / 4.0, w, h * 3 / 4.0), fill=YELLOW_ES)


def italian(d, w, h):
    for i, colour in enumerate((GREEN_IT, WHITE, RED_IT)):
        d.rectangle((i * w / 3.0, 0, (i + 1) * w / 3.0, h), fill=colour)


def polish(d, w, h):
    d.rectangle((0, 0, w, h / 2.0), fill=WHITE)
    d.rectangle((0, h / 2.0, w, h), fill=RED_PL)


def brazilian(d, w, h):
    """Green, the yellow rhombus, the blue globe with its white band (the stars and the motto left out
    at this size)."""
    d.rectangle((0, 0, w, h), fill=GREEN_BR)
    mx, my = w * 0.085, h * 0.085 * 1.5
    d.polygon([(mx, h / 2.0), (w / 2.0, my), (w - mx, h / 2.0), (w / 2.0, h - my)], fill=YELLOW_BR)
    r = h * 0.25
    d.ellipse((w / 2.0 - r, h / 2.0 - r, w / 2.0 + r, h / 2.0 + r), fill=BLUE_BR)
    # The band: an arc across the globe, from its left low to its right high.
    band = Image.new('L', (int(w), int(h)), 0)
    bd = ImageDraw.Draw(band)
    big = r * 2.2
    cx, cy = w / 2.0 - r * 0.2, h / 2.0 + r * 1.85
    bd.ellipse((cx - big, cy - big, cx + big, cy + big), outline=255, width=int(max(1, r * 0.16)))
    globe = Image.new('L', (int(w), int(h)), 0)
    ImageDraw.Draw(globe).ellipse((w / 2.0 - r, h / 2.0 - r, w / 2.0 + r, h / 2.0 + r), fill=255)
    d._image.paste(WHITE, (0, 0), ImageChops.multiply(band, globe))


def stripes(colours, vertical=False):
    def draw(d, w, h):
        n = len(colours)
        for i, colour in enumerate(colours):
            if vertical:
                d.rectangle((i * w / n, 0, (i + 1) * w / n, h), fill=colour)
            else:
                d.rectangle((0, i * h / n, w, (i + 1) * h / n), fill=colour)
    return draw


def swedish(d, w, h):
    """Blue, the yellow Nordic cross: 5:2:9 across, 4:2:4 down."""
    d.rectangle((0, 0, w, h), fill=(0, 106, 167))
    x0, x1 = w * 5 / 16.0, w * 7 / 16.0
    y0, y1 = h * 4 / 10.0, h * 6 / 10.0
    d.rectangle((x0, 0, x1, h), fill=(254, 204, 0))
    d.rectangle((0, y0, w, y1), fill=(254, 204, 0))


def nordic(field, cross, across, down, inner=None, inner_across=None, inner_down=None):
    """A Nordic cross: `across` and `down` the field's units (left, cross, right; top, cross, bottom);
    an `inner` cross (Norway's) drawn inside it on its own units."""
    def draw(d, w, h):
        d.rectangle((0, 0, w, h), fill=field)

        def cross_of(colour, a, b):
            x0 = w * a[0] / float(sum(a))
            x1 = w * (a[0] + a[1]) / float(sum(a))
            y0 = h * b[0] / float(sum(b))
            y1 = h * (b[0] + b[1]) / float(sum(b))
            d.rectangle((x0, 0, x1, h), fill=colour)
            d.rectangle((0, y0, w, y1), fill=colour)
        cross_of(cross, across, down)
        if inner is not None:
            cross_of(inner, inner_across, inner_down)
    return draw


def turkish(d, w, h):
    """Red, the white crescent and star (the flag law's proportions, G the height: the outer circle at
    G/2 from the hoist, G/2 across; the inner one G/16 farther, 2G/5 across; the star's circle G/4
    across, G/3 beyond the inner circle's centre), one point of the star towards the crescent."""
    import math
    g = float(h)
    d.rectangle((0, 0, w, h), fill=(227, 10, 23))
    cx, cy = g / 2, g / 2
    d.ellipse((cx - g / 4, cy - g / 4, cx + g / 4, cy + g / 4), fill=WHITE)
    ix = cx + g / 16
    d.ellipse((ix - g / 5, cy - g / 5, ix + g / 5, cy + g / 5), fill=(227, 10, 23))
    r = g / 8
    sx = ix + g / 3 + r
    pts = []
    for k in range(10):
        a = math.pi + k * math.pi / 5  # the first point towards the hoist
        rad = r if k % 2 == 0 else r * 0.382
        pts.append((sx + rad * math.cos(a), cy + rad * math.sin(a)))
    d.polygon(pts, fill=WHITE)


def greek(d, w, h):
    """Nine stripes, blue first; the blue canton five stripes high (as wide) with the white cross one
    stripe wide."""
    blue = (13, 94, 175)
    s = h / 9.0
    for i in range(9):
        d.rectangle((0, i * s, w, (i + 1) * s), fill=blue if i % 2 == 0 else WHITE)
    c = 5 * s
    d.rectangle((0, 0, c, c), fill=blue)
    d.rectangle((2 * s, 0, 3 * s, c), fill=WHITE)
    d.rectangle((0, 2 * s, c, 3 * s), fill=WHITE)


FLAGS = [union_flag, french, german, czech, slovak, spanish, italian, polish, brazilian,
         stripes([(174, 28, 40), WHITE, (33, 70, 139)]),           # Netherlands
         stripes([(206, 41, 57), WHITE, (71, 112, 80)]),           # Hungary
         stripes([(0, 43, 127), (252, 209, 22), (206, 17, 38)], vertical=True),  # Romania
         swedish,
         stripes([WHITE, (0, 57, 166), (213, 43, 30)]),            # Russia
         stripes([(0, 87, 183), (255, 215, 0)]),                  # Ukraine
         nordic(WHITE, (0, 47, 108), (5, 3, 10), (4, 3, 4)),       # Finland
         nordic((200, 16, 46), WHITE, (12, 4, 21), (12, 4, 12)),   # Denmark
         nordic((186, 12, 47), WHITE, (6, 4, 12), (6, 4, 6), (0, 32, 91), (7, 2, 13), (7, 2, 7)),  # Norway
         turkish,
         greek]


def sheet(scale):
    cw, ch = CELL_W * scale, CELL_H * scale
    out = Image.new('RGBA', (cw * len(FLAGS), ch), (0, 0, 0, 0))
    for i, draw in enumerate(FLAGS):
        big = Image.new('RGB', (cw * SUPER, ch * SUPER), WHITE)
        draw(ImageDraw.Draw(big), cw * SUPER, ch * SUPER)
        small = big.resize((cw, ch), Image.LANCZOS).convert('RGBA')
        edge = ImageDraw.Draw(small)
        edge.rectangle((0, 0, cw - 1, ch - 1), outline=(40, 40, 40, 255), width=scale)
        out.paste(small, (i * cw, 0))
    return out


def main():
    sheet(1).save('Content/icons/flags.png', optimize=True)
    sheet(2).save('Content/icons2x/flags.png', optimize=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
