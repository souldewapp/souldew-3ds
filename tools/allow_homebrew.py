#!/usr/bin/env python3
"""Sets the 3GX header's "allow homebrew load" flag (bit 9 of the info flags at 0x30) – test build only: the mock game
is built with libctru's homebrew start code, and plugin loaders refuse plugins in homebrew unless this bit is set.
3gxtool has no option for it. Real Pokémon games don't need it; the release souldew.3gx keeps it off."""
import struct, sys
p = sys.argv[1]
b = bytearray(open(p, 'rb').read())
assert b[:8] == b'3GX$0002', 'not a 3GX file'
flags = struct.unpack_from('<I', b, 0x30)[0] | (1 << 9)
struct.pack_into('<I', b, 0x30, flags)
open(p, 'wb').write(bytes(b))
print('%s: flags 0x%x' % (p, flags))
