"""Prevent normal hybrid firmware images from overwriting reserved credential flash."""
import pathlib
import struct
import sys
LIMIT = 0x101fb000
BASE = 0x10000000
for path in sys.argv[1:]:
    image = pathlib.Path(path).read_bytes()
    if not image or len(image) % 512:
        raise SystemExit("Invalid UF2: " + path)
    for off in range(0, len(image), 512):
        magic, magic2, flags, address, length = struct.unpack_from("<5I",image,off)
        if (magic,magic2)!=(0x0a324655,0x9e5d5157) or flags & 1:
            raise SystemExit("Unexpected UF2 block")
        if length>476 or address<BASE or address+length>LIMIT:
            raise SystemExit("Firmware overlaps reserved PS4 sector: " + hex(address))
    print("Verified firmware below PS4 credential sector:", path)
