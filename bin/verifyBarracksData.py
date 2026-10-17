#!/usr/bin/env python3
"""Verify the barracks initialized-data block through binary-comp.

Only one address shift is allowed for the entire block. In particular, the
two identical save-menu literals must retain their separate locations.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct
import sys

from binary_comp.analyzers.data import RelocatedAddressRange, relocated_pointer_match
from binary_comp.core.mapfile import parse_msvc_map_symbols
from binary_comp.core.pe import PEImage
from binary_comp.core.symbols import normalize_compiled


ORIGINAL_START = 0x004693C8
ORIGINAL_END = 0x00469640
BLOCK_SIZE = ORIGINAL_END - ORIGINAL_START
GLOBALS = (
    ('aBarracksBunkOrigins', 0x00),
    ('apszSaveCampaignMenuLabels', 0x20),
    ('apszBarracksMenuLabels', 0x28),
    ('stModalTextBounds', 0x78),
    ('pModalTextPanel', 0x80),
    ('asCampaignPilotFiles', 0x88),
    ('asCampaignBriefingFiles', 0x90),
    ('asMissionDataFiles', 0x98),
)
POINTER_OFFSETS = frozenset((0x20, 0x24, 0x68, 0x6C, 0x70, 0x74))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def data_section_size(path: str) -> int:
    data = Path(path).read_bytes()
    require(len(data) >= 20, f'{path}: truncated COFF header')
    machine, sections, _, _, _, optional, _ = struct.unpack_from('<HHIIIHH', data)
    require(machine == 0x14C, f'{path}: expected an i386 COFF object')
    require(len(data) >= 20 + optional + sections * 40,
            f'{path}: truncated COFF section table')
    sizes = []
    for index in range(sections):
        offset = 20 + optional + index * 40
        if data[offset:offset + 8].rstrip(b'\0') == b'.data':
            sizes.append(struct.unpack_from('<I', data, offset + 16)[0])
    require(len(sizes) == 1, f'{path}: expected one .data section')
    return sizes[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', default='data/full/WC1.ORI.EXE')
    parser.add_argument('--rebuilt', default='WC1.EXE')
    parser.add_argument('--map', dest='map_path', default='WC1.map')
    parser.add_argument('--object', dest='object_path', default='out/barracks.obj')
    args = parser.parse_args()

    original = PEImage(args.original)
    rebuilt = PEImage(args.rebuilt)
    symbols = parse_msvc_map_symbols(args.map_path)

    def address(name: str) -> int:
        matches = [entry.va for entry in symbols
                   if normalize_compiled(entry.symbol) == name
                   and (entry.object_file or '').split(':')[-1] == 'barracks.obj']
        require(len(matches) == 1, f'expected one barracks.obj:{name} in map')
        return matches[0]

    start = address(GLOBALS[0][0])
    require(data_section_size(args.object_path) == BLOCK_SIZE,
            f'barracks.obj: .data must contain exactly {BLOCK_SIZE} bytes')
    for name, offset in GLOBALS:
        require(address(name) == start + offset,
                f'{name}: incorrect offset within barracks data')

    for image, base in ((original, ORIGINAL_START), (rebuilt, start)):
        section = image.section_for_va(base)
        require(section is not None and section.name == '.data' and
                base + BLOCK_SIZE <= section.start + section.rawsize,
                f'{image.path}: barracks block must occupy initialized .data')
        offsets = {va - base for va in image.relocations()
                   if base <= va < base + BLOCK_SIZE}
        require(offsets == POINTER_OFFSETS,
                f'{image.path}: expected the six menu-pointer HIGHLOW relocations')

    original_bytes = original.read(ORIGINAL_START, BLOCK_SIZE)
    rebuilt_bytes = rebuilt.read(start, BLOCK_SIZE)
    require(original_bytes is not None and rebuilt_bytes is not None,
            'cannot read barracks data')
    ranges = (RelocatedAddressRange(ORIGINAL_START, ORIGINAL_END, start),)
    # Do not supply PE images: the string-content fallback would accept a
    # pointer to the other, byte-identical save-menu literal.
    require(relocated_pointer_match(original_bytes, rebuilt_bytes, {}, ranges),
            'barracks data differs after its single permitted address shift')
    for offset in sorted(POINTER_OFFSETS):
        require(relocated_pointer_match(original_bytes[offset:offset + 4],
                                        rebuilt_bytes[offset:offset + 4], {}, ranges),
                f'menu pointer at +0x{offset:02X} has an incorrect target')
    print(f'barracks.obj: {BLOCK_SIZE} data bytes match with one address shift; '
          'eight globals and six pointer relocations verified')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'barracks data verification failed: {error}', file=sys.stderr)
        sys.exit(1)
