#!/usr/bin/env python3
"""Draws the .cia's HOME menu banner and its sound (launcher/cia/banner.png, launcher/cia/banner.wav).

- banner.png (256x128): souldew's linked-rings pixel mark (the favicon, as in common/ui.cpp drawLogoMark), the
  wordmark in Pixelify Sans and "Nuzlocke tracker" in Silkscreen, on the launcher's dark "handheld at night" shell.
- banner.wav: a soft two-note chime (1.3 s, mono 16-bit, quiet: peak about -18 dBFS), synthesised here from sine
  waves, so there is no third-party audio in it. bannertool turns both into the banner (.bnr).

Needs Pillow. Run from anywhere: python3 tools/gen_cia_assets.py
"""
import math, os, struct, wave
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'launcher', 'cia')
FONTS = os.path.join(ROOT, 'assets', 'fonts')

SHELL_TOP, SHELL_BOTTOM = (46, 44, 56), (15, 14, 19)
INK, INK3 = (235, 230, 218), (147, 143, 135)
P1, P2, LINK = (140, 176, 255), (255, 143, 220), (242, 255, 220)

# the favicon's pixel art (cells): ring 1, ring 2, the link bar between them – same as drawLogoMark
RING1 = [(4, 2, 6, 2), (2, 4, 2, 8), (10, 4, 2, 2), (10, 10, 2, 2), (4, 12, 6, 2)]
RING2 = [(22, 2, 6, 2), (28, 4, 2, 8), (20, 4, 2, 2), (20, 10, 2, 2), (22, 12, 6, 2)]
BAR = (12, 6, 8, 4)


def banner(path):
    W, H = 256, 128
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    # body: vertical gradient in a rounded rect, a light rim
    grad = Image.new('RGBA', (W, H))
    gd = ImageDraw.Draw(grad)
    for y in range(H):
        t = y / (H - 1)
        gd.line([(0, y), (W, y)], fill=tuple(int(a + (b - a) * t) for a, b in zip(SHELL_TOP, SHELL_BOTTOM)) + (255,))
    mask = Image.new('L', (W, H), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, W - 1, H - 1], radius=18, fill=255)
    im.paste(grad, (0, 0), mask)
    # glows behind the rings (blue left, pink right)
    glow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    gl = ImageDraw.Draw(glow)
    s, mx, my = 3, 80, 20
    gl.ellipse([mx + 7 * s - 34, my + 8 * s - 26, mx + 7 * s + 34, my + 8 * s + 26], fill=(111, 155, 255, 90))
    gl.ellipse([mx + 25 * s - 34, my + 8 * s - 26, mx + 25 * s + 34, my + 8 * s + 26], fill=(255, 116, 212, 80))
    glow = glow.filter(ImageFilter.GaussianBlur(14))
    im = Image.alpha_composite(im, Image.composite(glow, Image.new('RGBA', (W, H), (0, 0, 0, 0)), mask))
    d = ImageDraw.Draw(im)
    for cells, col in ((RING1, P1), (RING2, P2), ([BAR], LINK)):
        for x, y, w, h in cells:
            d.rectangle([mx + x * s, my + y * s, mx + (x + w) * s - 1, my + (y + h) * s - 1], fill=col)
    # wordmark + line
    title = ImageFont.truetype(os.path.join(FONTS, 'PixelifySans-VF-latin.ttf'), 34)
    try:
        title.set_variation_by_name('Bold')
    except Exception:
        pass
    label = ImageFont.truetype(os.path.join(FONTS, 'Silkscreen-Regular.ttf'), 8)
    tw = d.textlength('souldew', font=title)
    d.text(((W - tw) / 2 + 2, 66 + 2), 'souldew', font=title, fill=(0, 0, 0, 110))
    d.text(((W - tw) / 2, 66), 'souldew', font=title, fill=INK)
    sub = 'NUZLOCKE TRACKER'
    lw = d.textlength(sub, font=label) + (len(sub) - 1)
    x = (W - lw) / 2
    for ch in sub:  # letter-spaced like the launcher's labels
        d.text((x, 108), ch, font=label, fill=INK3)
        x += d.textlength(ch, font=label) + 1
    ImageDraw.Draw(im).rounded_rectangle([0, 0, W - 1, H - 1], radius=18, outline=(255, 255, 255, 26), width=1)
    im.save(path)


def chime(path, rate=32000):
    # two soft bell-like notes (E5, then B5), each a sine with a quiet octave partial and an exponential decay
    notes = [(659.26, 0.00), (987.77, 0.18)]
    n = int(rate * 1.3)
    out = []
    for i in range(n):
        t = i / rate
        v = 0.0
        for f, start in notes:
            u = t - start
            if u < 0:
                continue
            env = min(1.0, u / 0.008) * math.exp(-u * 3.2)
            v += env * (math.sin(2 * math.pi * f * u) + 0.18 * math.sin(2 * math.pi * 2 * f * u))
        fade = min(1.0, (n - i) / (rate * 0.15))  # no click at the end
        out.append(v * fade)
    peak = max(abs(x) for x in out) or 1.0
    gain = 10 ** (-18 / 20) / peak
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1, min(1, x * gain)) * 32767)) for x in out))


if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    banner(os.path.join(OUT, 'banner.png'))
    chime(os.path.join(OUT, 'banner.wav'))
    print('ok')
