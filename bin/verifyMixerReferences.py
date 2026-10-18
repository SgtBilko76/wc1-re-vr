#!/usr/bin/env python3
"""Verify the mixer's named global operands through binary-comp.

Address normalization can hide an exchange of two global operands, even in
an instruction-exact function. Check their identities, multiplicity, and
offsets within each mixer routine. This is not an instruction similarity
score or a claim that absolute executable layout matches.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from dataclasses import replace
from itertools import zip_longest
from pathlib import Path
import sys

from binary_comp.analyzers.data import relocated_pointer_match
from binary_comp.config import load_project_target
from binary_comp.core.ghidra import function_starts_from_export_dir
from binary_comp.core.mapfile import function_starts_from_map, parse_msvc_map_symbols
from binary_comp.core.pe import PEImage
from binary_comp.core.symbols import normalize_compiled
from binary_comp.source.functions import load_source_groups, map_source_groups
from binary_comp.source.globals import parse_globals_source


FUNCTIONS = ('ix_mixer_thread_proc', 'ix_mixer_service')
GLOBALS = (
    'nMasterVolume', 'csMixer', 'hAudioWakeEvent', 'dwMixerWriteOffset',
    'pDspDevice', 'dwSoundBufferSize', 'dwDspFlags', 'pMixerBuffer',
    'dwDspTick', 'pDspPrimaryBuffer',
)


def function_end(starts: list[int], start: int) -> int:
    index = bisect_right(starts, start)
    if start not in starts or index == len(starts):
        raise ValueError(f'missing function boundary for 0x{start:08X}')
    return starts[index]


def references(image: PEImage, start: int, end: int, names: dict[int, str]):
    result = []
    for slot in sorted(image.relocations()):
        if not start <= slot < end:
            continue
        data = image.read(slot, 4)
        if data is None or len(data) != 4:
            raise ValueError(f'{image.path}: unreadable relocation at 0x{slot:08X}')
        if int.from_bytes(data, 'little') in names:
            result.append((slot - start, data))
    return result


def describe(reference, names: dict[int, str]) -> str:
    if reference is None:
        return '(missing)'
    offset, data = reference
    return f'+0x{offset:X}: {names[int.from_bytes(data, "little")]}'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', default='config/binary-comp.json')
    parser.add_argument('--target', default='full')
    parser.add_argument('--original')
    parser.add_argument('--rebuilt')
    parser.add_argument('--map', dest='map_path')
    args = parser.parse_args()
    _, target = load_project_target(args.config, args.target)
    target = replace(target, **{key: value for key, value in (
        ('original_exe', args.original), ('rebuilt_exe', args.rebuilt),
        ('map_path', args.map_path),
    ) if value is not None})

    sources = load_source_groups(target.source_dirs, target.map_skip,
                                 target.source_excludes)
    groups, missing, entries = map_source_groups(sources, target.map_path)
    if missing:
        raise ValueError('unmapped source functions: ' +
                         ', '.join(group.name for _, group in missing))
    selected = {group.name: group for group in groups if group.name in FUNCTIONS}
    if set(selected) != set(FUNCTIONS):
        raise ValueError('missing mixer function metadata')

    root = Path(__file__).resolve().parent.parent
    declarations = [decl for filename in ('dsp.cpp', 'dspv.cpp')
                    for decl in parse_globals_source(str(root / 'src/ix' / filename))]
    symbols = parse_msvc_map_symbols(str(target.map_path))
    address_map = {}
    original_names = {}
    rebuilt_names = {}
    for name in GLOBALS:
        original = [decl.address for decl in declarations if decl.name == name]
        rebuilt = [entry.va for entry in symbols
                   if normalize_compiled(entry.symbol) == name]
        if len(original) != 1 or len(rebuilt) != 1:
            raise ValueError(f'missing or ambiguous global metadata: {name}')
        address_map[original[0]] = rebuilt[0]
        original_names[original[0]] = name
        rebuilt_names[rebuilt[0]] = name
    if len(original_names) != len(GLOBALS) or len(rebuilt_names) != len(GLOBALS):
        raise ValueError('mixer globals must have distinct addresses')

    original_image = PEImage(target.original_exe)
    rebuilt_image = PEImage(target.rebuilt_exe)
    original_starts = function_starts_from_export_dir(target.code_dir)
    rebuilt_starts = function_starts_from_map(entries)
    failures = 0
    count = 0
    for name in FUNCTIONS:
        group = selected[name]
        if len(group.original_addrs) != 1:
            raise ValueError(f'ambiguous original entry: {name}')
        original_start = group.original_addrs[0]
        original = references(original_image, original_start,
                              function_end(original_starts, original_start),
                              original_names)
        rebuilt = references(rebuilt_image, group.rebuilt_addr,
                             function_end(rebuilt_starts, group.rebuilt_addr),
                             rebuilt_names)
        if not original:
            raise ValueError(f'no original global references found: {name}')
        count += len(original)
        offsets_match = [offset for offset, _ in original] == [offset for offset, _ in rebuilt]
        # Check each pointer separately: every operand must resolve through
        # its named mapping, even if a wrong address equals an original value.
        targets_match = len(original) == len(rebuilt) and all(
            relocated_pointer_match(expected[1], actual[1], address_map, ())
            for expected, actual in zip(original, rebuilt))
        if offsets_match and targets_match:
            continue
        failures += 1
        print(f'{name}: global-reference targets or instruction offsets differ')
        for expected, actual in zip_longest(original, rebuilt):
            if (expected is None or actual is None or expected[0] != actual[0]
                    or not relocated_pointer_match(expected[1], actual[1], address_map, ())):
                print(f'  original {describe(expected, original_names)}; '
                      f'rebuilt {describe(actual, rebuilt_names)}')
    print(f'Mixer global references: {count} operands in {len(FUNCTIONS)} functions; '
          f'{failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'mixer-reference verification failed: {error}', file=sys.stderr)
        sys.exit(1)
