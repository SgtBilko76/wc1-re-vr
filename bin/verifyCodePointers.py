#!/usr/bin/env python3
"""Verify embedded function-pointer targets through binary-comp.

Instruction comparison normalizes address operands, which can hide a pointer
to the wrong function. Check HIGHLOW-relocated pointers to function entries
in each reconstructed routine, preserving their order and multiplicity.
This checks reference identity; it does not score instructions or layout.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from dataclasses import replace
import sys

from binary_comp.analyzers.data import relocated_pointer_match
from binary_comp.config import load_project_target
from binary_comp.core.ghidra import function_starts_from_export_dir
from binary_comp.core.mapfile import function_starts_from_map
from binary_comp.core.pe import EXECUTABLE_FLAG, PEImage
from binary_comp.source.functions import load_source_groups, map_source_groups


def references(image: PEImage, starts: list[int], owners: dict[int, int]):
    entries = set(starts)
    result: dict[int, list[bytes]] = {}
    for slot in sorted(image.relocations()):
        section = image.section_for_va(slot)
        if section is None or not section.flags & EXECUTABLE_FLAG:
            continue
        index = bisect_right(starts, slot) - 1
        if index < 0 or starts[index] not in owners:
            continue
        data = image.read(slot, 4)
        if data is None or len(data) != 4:
            raise ValueError(f'{image.path}: unreadable relocation at 0x{slot:08X}')
        if int.from_bytes(data, 'little') in entries:
            result.setdefault(owners[starts[index]], []).append(data)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', default='config/binary-comp.json')
    parser.add_argument('--target', default='full')
    parser.add_argument('--original')
    parser.add_argument('--rebuilt')
    parser.add_argument('--map', dest='map_path')
    args = parser.parse_args()
    _, target = load_project_target(args.config, args.target)
    overrides = {key: value for key, value in (
        ('original_exe', args.original), ('rebuilt_exe', args.rebuilt),
        ('map_path', args.map_path),
    ) if value is not None}
    target = replace(target, **overrides)

    sources = load_source_groups(target.source_dirs, target.map_skip,
                                 target.source_excludes)
    groups, missing, entries = map_source_groups(sources, target.map_path)
    if missing:
        raise ValueError('unmapped source functions: ' +
                         ', '.join(group.name for _, group in missing))
    if not groups:
        raise ValueError('no reconstructed functions found')

    original_owners = {address: group.original_addrs[0]
                       for group in groups for address in group.original_addrs}
    rebuilt_owners = {group.rebuilt_addr: group.original_addrs[0]
                      for group in groups}
    address_map = {group.original_addrs[0]: group.rebuilt_addr for group in groups}
    names = {group.original_addrs[0]: group.name for group in groups}
    rebuilt_names = {group.rebuilt_addr: group.name for group in groups}
    original = references(PEImage(target.original_exe),
                          function_starts_from_export_dir(target.code_dir),
                          original_owners)
    rebuilt = references(PEImage(target.rebuilt_exe),
                         function_starts_from_map(entries), rebuilt_owners)
    if not original:
        raise ValueError('no original embedded function pointers found')

    def describe(pointers, labels):
        addresses = [int.from_bytes(data, 'little') for data in pointers]
        return ', '.join(labels.get(address, f'0x{address:08X}')
                         for address in addresses) or '(none)'

    failures = 0
    for owner in sorted(original.keys() | rebuilt.keys()):
        expected = original.get(owner, [])
        actual = rebuilt.get(owner, [])
        # Map only identified function entries. Do not allow broad code-range
        # shifts or the string-content fallback accepted by other data checks.
        if not relocated_pointer_match(b''.join(expected), b''.join(actual),
                                       address_map, ()):
            failures += 1
            print(f'{names[owner]}: function-pointer targets differ\n'
                  f'  original: {describe(expected, names)}\n'
                  f'  rebuilt:  {describe(actual, rebuilt_names)}')
    count = sum(len(pointers) for pointers in original.values())
    print(f'Code pointers: {count} references in {len(original)} functions; '
          f'{failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'code-pointer verification failed: {error}', file=sys.stderr)
        sys.exit(1)
