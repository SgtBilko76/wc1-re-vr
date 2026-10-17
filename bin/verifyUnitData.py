#!/usr/bin/env python3
"""Verify recovered compilation-unit data blocks through binary-comp.

Only one address shift is allowed per block. Identical literals must retain
their separate locations, and empty-string pointers must remain correct.
Pointers outside a block must target the declared globals at their original
field offsets, using the rebuilt linker symbols.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct
import sys

from binary_comp.analyzers.data import (
    AddressComparison, RelocatedAddressRange, relocated_pointer_match,
)
from binary_comp.core.mapfile import parse_msvc_map_symbols
from binary_comp.core.pe import PEImage
from binary_comp.core.symbols import normalize_compiled


@dataclass(frozen=True)
class UnitData:
    original_start: int
    size: int
    globals: tuple[tuple[str, int], ...]
    pointers: frozenset[int]
    external_globals: tuple[tuple[str, int, int], ...] = ()


UNITS = {
    'barracks': UnitData(0x004693C8, 632, (
        ('aBarracksBunkOrigins', 0x00),
        ('apszSaveCampaignMenuLabels', 0x20),
        ('apszBarracksMenuLabels', 0x28),
        ('stModalTextBounds', 0x78),
        ('pModalTextPanel', 0x80),
        ('asCampaignPilotFiles', 0x88),
        ('asCampaignBriefingFiles', 0x90),
        ('asMissionDataFiles', 0x98),
    ), frozenset((0x20, 0x24, 0x68, 0x6C, 0x70, 0x74))),
    'debug': UnitData(0x00469644, 36, (
        ('nDebugOverlayConsoleCount', 0x00),
        ('dwDebugOverlayKey', 0x04),
        ('dwDebugOverlayKeyLatch', 0x08),
        ('hDebugKeyboardHook', 0x0C),
        ('szDebugOverlayFontName', 0x10),
        ('szDebugOverlaySpinner', 0x18),
        ('szDebugOverlayNewline', 0x20),
    ), frozenset()),
    'disk': UnitData(0x00469668, 523, (
        ('apszGraphicsModeNames', 0x00),
        ('pszPacketReadOperation', 0x10),
        ('pszPacketAllocationOperation', 0x14),
        ('pszPacketLoadErrorFormat', 0x18),
        ('szDiskMarkerFile', 0x20),
        ('nDiskPromptBorderColour', 0x2C),
        ('DAT_00469698', 0x30),
        ('nKeyboardPointerStep', 0x3C),
    ), frozenset((0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x30, 0x34))),
    'ship': UnitData(0x00469878, 286, (
        ('asPlayerDamageSystemTable', 0x00),
        ('aeShipHitDebrisTypes', 0xD8),
        ('acGunRefireDelay', 0xE4),
    ), frozenset()),
    'init': UnitData(0x00469998, 550, (
        ('bSlowSceneAnimation', 0x00),
        ('cBlackColour', 0x04),
        ('cViewportClearColour', 0x08),
        ('cBlueColour', 0x0C),
        ('cYellowColour', 0x10),
        ('cRedColour', 0x14),
        ('cDarkGreenColour', 0x18),
        ('cPrimaryTextColour', 0x1C),
        ('cOrangeColour', 0x20),
        ('cDarkBlueColour', 0x24),
        ('cDarkGreyColour', 0x28),
        ('cLightGreyColour', 0x2C),
        ('cMagentaColour', 0x30),
        ('cDefaultTextColour', 0x34),
        ('cBrownColour', 0x38),
        ('cAsteroidColour', 0x3C),
        ('cPrimaryViewBufferColour', 0x40),
        ('abLegacyVideoModeColours', 0x48),
        ('asConversationTextColours', 0x58),
        ('bGraphicsActive', 0x88),
        ('szInsufficientMemory', 0x90),
        ('anBaseMemoryReservationByVideoMode', 0xF8),
        ('anFullMusicMemoryReservationByVideoMode', 0x108),
        ('anExpandedMemoryReservationByVideoMode', 0x118),
    ), frozenset()),
    'logic': UnitData(0x00469BC0, 447, (
        ('aCommon3SpaceResources', 0x000),
        ('aMissionResourceDescriptors', 0x060),
        ('aCockpitResourceDescriptors', 0x088),
        ('aCockpitSecondaryResources', 0x120),
        ('aCockpitPrimaryResources', 0x148),
        ('bIntroSceneResourcesActive', 0x188),
        ('asConstellationObjectIndices', 0x190),
        ('bCockpitResourcesActive', 0x198),
        ('b3SpaceObjectsActive', 0x19C),
        ('asSceneAnimationLogicalFiles', 0x1A0),
        ('bSceneAnimationWaitCommand', 0x1B0),
    ), frozenset((*range(0x000, 0x058, 8), *range(0x060, 0x080, 8),
                  *range(0x088, 0x118, 8), *range(0x120, 0x140, 8),
                  *range(0x148, 0x180, 8))), (
        ('aObjectTypeData', 0x00466458, 58 * 0x87),
        ('pCockpitPilotShape', 0x0046905C, 4),
        ('pCockpitExplosionShape', 0x00469064, 4),
        ('pConfedCommBackground', 0x00469278, 4),
        ('pCommStaticShape', 0x0046927C, 4),
        ('pKilrathiCommBackground', 0x00469280, 4),
        ('apCommPortraitShapes', 0x0059E180, 16 * 4),
        ('pTargetLockShape', 0x005A6BF4, 4),
        ('pCockpitWeaponShape', 0x005A7564, 4),
        ('pCockpitIndicatorShape', 0x005A7658, 4),
        ('pPilotHandShape', 0x005A7684, 4),
        ('pCockpitDamageShape', 0x005A76F4, 4),
        ('apCockpitShapes', 0x005A7C80, 4 * 4),
        ('pCinematicViewBackdrop', 0x005A7C90, 4),
        ('pRearViewBackdrop', 0x005A7C94, 4),
    )),
}


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
    parser.add_argument('--objects-root', type=Path, default=Path('out'))
    parser.add_argument('--unit', choices=UNITS, action='append')
    args = parser.parse_args()

    original = PEImage(args.original)
    rebuilt = PEImage(args.rebuilt)
    symbols = parse_msvc_map_symbols(args.map_path)

    def address(owner: str | None, name: str) -> int:
        matches = [entry.va for entry in symbols
                   if normalize_compiled(entry.symbol) == name
                   and (owner is None or
                        (entry.object_file or '').split(':')[-1] == owner)]
        require(len(matches) == 1, f'expected one {owner}:{name} in map')
        return matches[0]

    for unit in args.unit or UNITS:
        block = UNITS[unit]
        owner = unit + '.obj'
        start = address(owner, block.globals[0][0])
        require(data_section_size(str(args.objects_root / owner)) == block.size,
                f'{owner}: .data must contain exactly {block.size} bytes')
        for name, offset in block.globals:
            require(address(owner, name) == start + offset,
                    f'{name}: incorrect offset within {unit} data')

        for image, base in ((original, block.original_start), (rebuilt, start)):
            section = image.section_for_va(base)
            require(section is not None and section.name == '.data' and
                    base + block.size <= section.start + section.rawsize,
                    f'{image.path}: {unit} block must occupy initialized .data')
            offsets = {va - base for va in image.relocations()
                       if base <= va < base + block.size}
            require(offsets == block.pointers,
                    f'{image.path}: incorrect HIGHLOW relocations for {unit}')

        original_bytes = original.read(block.original_start, block.size)
        rebuilt_bytes = rebuilt.read(start, block.size)
        require(original_bytes is not None and rebuilt_bytes is not None,
                f'cannot read {unit} data')
        ranges = (RelocatedAddressRange(block.original_start,
                                       block.original_start + block.size, start),)
        ranges += tuple(RelocatedAddressRange(base, base + size, address(None, name))
                        for name, base, size in block.external_globals)
        # Do not supply PE images: string-content fallback would accept a
        # pointer to another, byte-identical literal at an incorrect offset.
        if block.pointers:
            matches = relocated_pointer_match(original_bytes, rebuilt_bytes, {}, ranges)
        else:
            matches = AddressComparison(block.original_start, block.size, start,
                                        original_bytes, rebuilt_bytes).matches
        require(matches, f'{unit} data differs after its single permitted address shift')
        for offset in sorted(block.pointers):
            require(relocated_pointer_match(original_bytes[offset:offset + 4],
                                            rebuilt_bytes[offset:offset + 4], {}, ranges),
                    f'{unit} pointer at +0x{offset:02X} has an incorrect target')
        print(f'{owner}: {block.size} data bytes match with one address shift; '
              f'{len(block.globals)} globals and {len(block.pointers)} '
              'pointer relocations verified')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'unit data verification failed: {error}', file=sys.stderr)
        sys.exit(1)
