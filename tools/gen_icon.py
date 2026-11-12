#!/usr/bin/env python3
"""Draws the 48x48 home-menu icons (launcher/icon.png, mockgame/icon.png): souldew's linked-rings mark."""
import os
from PIL import Image, ImageDraw
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def icon(path, ring=(127, 157, 219), bg=(42, 41, 49), edge=(57, 56, 66), s=4):
    W = 48 * s
    im = Image.new('RGBA', (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([2 * s, 2 * s, 46 * s - 1, 46 * s - 1], radius=10 * s, fill=edge)
    d.rounded_rectangle([3 * s, 3 * s, 45 * s - 1, 45 * s - 1], radius=9 * s, fill=bg)
    r, w = 9 * s, 3 * s
    for cx in (19 * s, 29 * s):
        d.ellipse([cx - r, 24 * s - r, cx + r, 24 * s + r], outline=ring, width=w)
    im.resize((48, 48), Image.LANCZOS).save(path)

icon(os.path.join(ROOT, 'launcher', 'icon.png'))
icon(os.path.join(ROOT, 'mockgame', 'icon.png'), ring=(225, 176, 90))
print('ok')
