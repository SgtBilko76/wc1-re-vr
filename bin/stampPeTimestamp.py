#!/usr/bin/env python3
"""Set a PE image's link timestamp.

LINK 4.20 writes the current time into the file header and has no option to
choose it, so the reference build stamps the original's link time afterwards:

    bin/stampPeTimestamp.py WC1.EXE 0x32480D3C

The image carries no checksum and no debug directory, so the file header's
TimeDateStamp is the only copy of the time.
"""
import struct
import sys


def main() -> int:
    path, value = sys.argv[1], int(sys.argv[2], 0)
    with open(path, 'r+b') as handle:
        data = handle.read(0x400)
        if data[:2] != b'MZ':
            raise SystemExit(f'{path}: not an MZ executable')
        header = struct.unpack_from('<I', data, 0x3C)[0]
        if data[header:header + 4] != b'PE\0\0':
            raise SystemExit(f'{path}: not a PE executable')
        handle.seek(header + 8)
        handle.write(struct.pack('<I', value))
    return 0


if __name__ == '__main__':
    sys.exit(main())
