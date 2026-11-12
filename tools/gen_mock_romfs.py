#!/usr/bin/env python3
"""Builds the mock game's RomFS (mockgame/romfs/a/0/1/2): a GARC laid out like X/Y's encounter archive, whose last
file is a zone table (56 bytes per zone, the place / met location at +0x1C, 9 bits). The plugin reads it from the
running title's RomFS exactly as it does in the real game. Zones: 0 Vaniville Town (6), 1 Aquacorde Town (10),
2 Route 2 (12), 3 Santalune Forest (14), 4 Route 3 (16), 5 Santalune City (18)."""
import os, struct
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLACES = [6, 10, 12, 14, 16, 18]

def garc(files):
    n = len(files)
    fato = b'OTAF' + struct.pack('<IHH', 0x0C + 4 * n, n, 0xFFFF) + b''.join(struct.pack('<I', i * 16) for i in range(n))
    data, entries, off = b'', b'', 0
    for f in files:
        pad = f + b'\0' * ((-len(f)) % 4)
        entries += struct.pack('<IIII', 1, off, off + len(f), len(f))
        data += pad
        off += len(pad)
    fatb = b'BTAF' + struct.pack('<II', 0x0C + len(entries), n) + entries
    fimb = b'BMIF' + struct.pack('<II', 0x0C, len(data)) + data
    head_size = 0x1C
    data_off = head_size + len(fato) + len(fatb) + 0x0C
    total = head_size + len(fato) + len(fatb) + len(fimb)
    head = b'CRAG' + struct.pack('<IHHIIII', head_size, 0xFEFF, 0x0400, 4, data_off, total, max(len(f) for f in files))
    return head + fato + fatb + fimb

zones = b''
for p in PLACES:
    z = bytearray(56)
    z[0x1C] = p & 0xFF
    z[0x1D] = (p >> 8) & 1
    zones += bytes(z)
out = os.path.join(ROOT, 'mockgame', 'romfs', 'a', '0', '1')
os.makedirs(out, exist_ok=True)
with open(os.path.join(out, '2'), 'wb') as f:
    f.write(garc([b'\x00' * 32, zones]))
print('mockgame/romfs/a/0/1/2 written')
