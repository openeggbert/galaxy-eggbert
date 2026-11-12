#!/usr/bin/env python3
"""The flags of the game's languages for the main menu's language button (the owner, 2026-11-12).

Content/icons/flags.png (and its 2x sheet in Content/icons2x/): one cell a language, in
Localization.hpp's kLanguages order: English (the Union Flag), French, German, Czech. Each cell is
48x32 (96x64 at 2x), the flag drawn from its geometry (no picture copied), eight times as large and
scaled down, with a thin dark edge. Run from the repository: python3 tools/make_flags.py
"""
import sys
from PIL import Image, ImageDraw

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


FLAGS = [union_flag, french, german, czech]


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
