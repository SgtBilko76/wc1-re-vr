#!/usr/bin/env python3
"""Check the compiler-generated DSP array initializers and CRT registrations.

The four $E functions have no source annotations. Extract their linked spans
and delegate exact byte comparison to binary-comp; do not score them here.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from binary_comp.core.mapfile import parse_msvc_map_symbols
from binary_comp.core.pe import PEImage
from binary_comp.core.symbols import normalize_compiled


INITIALIZERS = (
    ('dsps.obj', 0x00445190, 0x004451B5, 'ix_dsps_alloc'),
    ('dspv.obj', 0x004467A0, 0x004467C5, 'ix_dspv_set_active'),
)
ORIGINAL_CRT_START = 0x00465000


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', default='data/full/WC1.ORI.EXE')
    parser.add_argument('--rebuilt', default='WC1.EXE')
    parser.add_argument('--map', dest='map_path', default='WC1.map')
    parser.add_argument('--binary-comp', default='binary-comp')
    args = parser.parse_args()

    original = PEImage(args.original)
    rebuilt = PEImage(args.rebuilt)
    symbols = parse_msvc_map_symbols(args.map_path)

    def address(name: str, owner: str | None = None) -> int:
        matches = [entry.va for entry in symbols
                   if (entry.symbol == name or
                       normalize_compiled(entry.symbol) == name)
                   and (owner is None or
                        (entry.object_file or '').split(':')[-1] == owner)]
        require(len(matches) == 1, f'expected one {owner or ""}:{name} in map')
        return matches[0]

    def word(image: PEImage, va: int) -> int:
        data = image.read(va, 4)
        require(data is not None and len(data) == 4,
                f'cannot read CRT entry at 0x{va:08X} in {image.path}')
        return struct.unpack('<I', data)[0]

    crt_start = address('___xc_a')
    crt_end = address('___xc_z')
    require(crt_end - crt_start == 0x10,
            'CRT constructor table must contain the CRT initializer and two DSP entries')
    require(word(rebuilt, crt_start) == 0 and word(rebuilt, crt_end) == 0,
            'CRT constructor table terminators must be zero')

    with tempfile.TemporaryDirectory(prefix='wc1-array-initializers-') as temp:
        for index, (owner, original_start, original_end, first_function) in enumerate(INITIALIZERS):
            start = address('_$E2', owner)
            inner = address('_$E1', owner)
            end = address(first_function, owner)
            require(inner == start + 21, f'{owner}: $E1 must follow the 21-byte $E2')
            require(start < inner < end and end - start < 4096,
                    f'{owner}: invalid initializer boundaries')

            original_slot = ORIGINAL_CRT_START + 8 + index * 4
            rebuilt_slot = crt_start + 8 + index * 4
            require(word(original, original_slot) == original_start,
                    f'{owner}: unexpected original CRT registration')
            require(word(rebuilt, rebuilt_slot) == start,
                    f'{owner}: missing or misordered CRT registration')
            require(original_slot in original.relocations() and
                    rebuilt_slot in rebuilt.relocations(),
                    f'{owner}: CRT pointer must have a HIGHLOW relocation')

            original_bytes = original.read(original_start, original_end - original_start)
            rebuilt_bytes = rebuilt.read(start, end - start)
            require(original_bytes is not None and rebuilt_bytes is not None,
                    f'{owner}: cannot read initializer code')
            original_path = Path(temp, owner + '.original.bin')
            rebuilt_path = Path(temp, owner + '.rebuilt.bin')
            original_path.write_bytes(original_bytes)
            rebuilt_path.write_bytes(rebuilt_bytes)
            result = subprocess.run(
                [args.binary_comp, 'byte-compare', '--fail-on-difference',
                 str(original_path), str(rebuilt_path)],
                capture_output=True, text=True,
            )
            if result.returncode:
                print(result.stdout, end='')
                print(result.stderr, end='', file=sys.stderr)
                raise ValueError(f'{owner}: initializer bytes differ')
            print(f'{owner}: {len(original_bytes)} initializer bytes exact; '
                  'CRT registration and relocation verified')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'initializer verification failed: {error}', file=sys.stderr)
        sys.exit(1)
