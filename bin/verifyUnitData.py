#!/usr/bin/env python3
"""Verify recovered compilation-unit data blocks through binary-comp.

Only one address shift is allowed per block. Identical literals must retain
their separate locations, and empty-string pointers must remain correct.
Pointers outside a block must target the declared globals at their original
field offsets, using the rebuilt linker symbols.
Read-only blocks must retain their COFF .rdata storage and declared size.
Uninitialized blocks must retain their COFF .bss storage and declared size.
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
    storage: str = '.data'
    owner: str | None = None
    # Set only for a verified span within an incompletely recovered section.
    section_size: int | None = None


UNITS = {
    'nav': UnitData(0x00468660, 608, (
        ('nNavMapCoordinateScaling', 0x00),
        ('nNavMapScale', 0x04),
        ('aNavMapObjectiveStyles', 0x08),
        ('pNavMapShape', 0xA8),
        ('nNavMapLabelCount', 0xAC),
        ('nNavMapReservedAreaCount', 0xB0),
        ('szCampaignSector', 0xB8),
        ('apszShipMissionTypeNames', 0xC8),
        ('bInflightComputerActive', 0xF4),
    ), frozenset((*[0x0C + row * 16 + column * 4
                    for row in range(5) for column in range(3)],
                  *range(0xC8, 0xF4, 4))), (
        ('cBlackColour', 0x0046999C, 1),
        ('cViewportClearColour', 0x004699A0, 1),
        ('cRedColour', 0x004699AC, 1),
        ('cPrimaryTextColour', 0x004699B4, 1),
        ('cMagentaColour', 0x004699C8, 1),
        ('cDefaultTextColour', 0x004699CC, 1),
    )),
    'nearheap': UnitData(0x004688C0, 12, (
        ('nNearHeapActive', 0x00),
        ('nNearHeapMaxDescriptors', 0x04),
        ('nNearHeapRelocationBytes', 0x08),
    ), frozenset()),
    'gameflow': UnitData(0x004688CC, 64, (
        ('bOfficeVisitPending', 0x00),
        ('bPromotionPending', 0x04),
        ('bPlayerEjectedThisMission', 0x08),
        ('bPlayerShipTypeChanged', 0x0C),
        ('nPreviousPlayerShipType', 0x10),
        ('bCampaignStartupMode', 0x14),
        ('nPendingMedalIndex', 0x18),
        ('nPostSeriesSequence', 0x1C),
        ('bSeriesFailed', 0x20),
        ('bCampaignActive', 0x24),
    ), frozenset()),
    'title': UnitData(0x00468910, 1522, (
        ('pszIntroOpeningText', 0x000),
        ('szMissingSecretMissionsMessage', 0x008),
        ('nIntroCreditCount', 0x120),
        ('apszIntroCredits', 0x128),
        ('aTitleMenuRegions', 0x178),
        ('aIntroResourceDescriptors', 0x1B0),
        ('bTitleMenuSceneInitialized', 0x1C8),
    ), frozenset((0x000, *range(0x128, 0x174, 4), 0x1B0, 0x1B8)), (
        ('aObjectTypeData', 0x00466458, 58 * 0x87),
    )),
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
    'fade': UnitData(0x00469640, 2, (
        ('nPaletteTransitionInitialise', 0x00),
    ), frozenset()),
    'debug': UnitData(0x00469644, 34, (
        ('nDebugOverlayConsoleCount', 0x00),
        ('dwDebugOverlayKey', 0x04),
        ('dwDebugOverlayKeyLatch', 0x08),
        ('hDebugKeyboardHook', 0x0C),
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
    'trainsim': UnitData(0x00469DA8, 511, (
        ('stTrainSimPanelViewport', 0x00),
        ('stTrainSimPanelBounds', 0x18),
        ('szTrainSimTitle', 0x20),
        ('apszBuiltInHighScoreNames', 0x38),
        ('aTrainSimMissionRegions', 0x50),
        ('nTrainSimActive', 0x84),
        ('nTrainSimMission', 0x88),
        ('nArcadeWave', 0x8C),
    ), frozenset(range(0x38, 0x50, 4))),
    'pilot': UnitData(0x004763F0, 4, (
        ('pDebugOverlay', 0x00),
    ), frozenset(), storage='.bss'),
    'sound': UnitData(0x0047654C, 8, (
        ('pSnowStaticWave', 0x00),
        ('pSnowStaticSound', 0x04),
    ), frozenset(), storage='.bss'),
    'damage': UnitData(0x0046A748, 390, (
        ('pDamageDisplayBackground', 0x00),
        ('aDamageDisplayPositions', 0x08),
        ('apszComponentNames', 0x30),
        ('abDamageDisplayFrames', 0x58),
        ('apszDamageSeverityNames', 0x68),
    ), frozenset((*range(0x30, 0x54, 4), *range(0x68, 0x7C, 4)))),
    'view': UnitData(0x0046A8D0, 289, (
        ('nScriptedViewObject', 0x00),
        ('bScriptedView', 0x04),
        ('asConstellationVelocity', 0x08),
        ('asConstellationFrame', 0x28),
        ('nConstellationDirection', 0x48),
        ('pPacketDecompressionWorkspace', 0x4C),
        ('wPacketDecompressionInputSizeOverride', 0x50),
        ('wPacketCompressionFormatFlags', 0x54),
        ('aTargetArmorClipRects', 0x58),
    ), frozenset()),
    'ending': UnitData(0x0046AD88, 457, (
        ('abTaggedAllocationPrefix', 0x00),
        ('apszCampaignVictoryText', 0x08),
        ('pszTigerClawEscapeOpening', 0x18),
        ('pszTigerClawEscapeJump', 0x1C),
        ('pszTigerClawEscapeClosing', 0x20),
        ('aCampaignVictoryProjectileOrigins', 0x28),
        ('pszTheEnd', 0x40),
        ('pszForNow', 0x44),
    ), frozenset((0x08, 0x0C, 0x10, 0x18, 0x1C, 0x20, 0x40, 0x44))),
    'input': UnitData(0x0046AF54, 6, (
        ('nInputDoubleClickInterval', 0x00),
        ('nMenuPointerSpeed', 0x04),
    ), frozenset()),
    'comm': UnitData(0x0046AF60, 520, (
        ('nCommMenuChoiceCount', 0x00),
        ('nCommMenuReuseMode', 0x04),
        ('bMessageSpeed', 0x08),
        ('cPendingCommMenuAction', 0x0C),
        ('bRadioSilence', 0x10),
        ('bVideoImagesSuppressed', 0x14),
        ('bCommVideoEnabled', 0x18),
        ('apszKilrathiAceNames', 0x20),
        ('apszCommMenuText', 0x30),
        ('cCommMenuRecipient', 0x64),
        ('nCommSpeakerObject', 0x68),
        ('nCommSpeakerRating', 0x6C),
        ('nCommPortraitIndex', 0x70),
    ), frozenset(range(0x20, 0x64, 4))),
    'video': UnitData(0x0046B168, 10, (
        ('nVideoMode', 0x00),
    ), frozenset()),
    'joycal': UnitData(0x0046B178, 42, (
        ('acGraphicsModeByAnimationSpeed', 0x00),
        ('szTMDriverName', 0x08),
        ('szSuperTMDriverName', 0x10),
    ), frozenset()),
    'game-state': UnitData(0x0046C000, 112, (
        ('cViewObject', 0x0000),
        ('nEyePitchRate', 0x0004),
        ('nEyeYawRate', 0x0008),
        ('nEyeRollRate', 0x000C),
        ('DAT_0046c010', 0x0010),
        ('bCurrentKey', 0x0014),
        ('cPreviousKey', 0x0018),
        ('nCurrentWave', 0x001C),
        ('cCurrentObjective', 0x0020),
        ('bIntroSecondaryScene', 0x0024),
        ('abHazardObjects', 0x0028),
        ('nCameraViewMode', 0x003C),
        ('nExternalViewShip', 0x0040),
        ('nEjectedPilotObject', 0x0044),
        ('nClosestVisibleObject', 0x0048),
        ('nYourWingman', 0x004C),
        ('nPlayerCollisionObject', 0x0050),
        ('eSelectedGunType', 0x0054),
        ('nSelectedReleaseWeaponIndex', 0x0058),
        ('DAT_0046c05c', 0x005C),
        ('bTargetLockReadoutDirty', 0x0060),
        ('nTargetLockCountdown', 0x0064),
        ('nTargetLockMarkerAngle', 0x0068),
        ('cTargetDisplayObject', 0x006C),
    ), frozenset(), owner='game.obj', section_size=6788),
    'game-lock-state': UnitData(0x0046C074, 21, (
        ('bTargetLockAcquired', 0x0000),
        ('nTargetLockMode', 0x0004),
        ('bMissileCameraEnabled', 0x0008),
        ('bEngageAllowed', 0x000C),
        ('nAutoEngageTimer', 0x0010),
        ('cViableTargetCount', 0x0014),
    ), frozenset(), owner='game.obj', section_size=6788),
    'game-tables': UnitData(0x0046C090, 6652, (
        ('asIntroCameraSequence', 0x0000),
        ('asCannedSequence32', 0x0028),
        ('asCannedSequence33', 0x0048),
        ('asCannedSequence34', 0x0060),
        ('asCannedSequence35', 0x0078),
        ('asCampaignVictoryViewScript', 0x00D0),
        ('asCannedSequence37', 0x0100),
        ('asCannedSequence38', 0x0120),
        ('asCannedSequence39', 0x0140),
        ('asCannedSequence40', 0x0150),
        ('asCannedSequence41', 0x01A0),
        ('asTigerClawEscapeViewScript', 0x01A8),
        ('asCannedSequence42', 0x01C0),
        ('asCannedSequence43', 0x01E0),
        ('asCannedSequence44', 0x0210),
        ('asCannedSequence45', 0x0258),
        ('aMissionNavPoints', 0x0260),
        ('aMissionShips', 0x08B8),
        ('aiIntelligenceEvent', 0x12D8),
        ('aDefenseManeuversNovice', 0x1300),
        ('aDefenseManeuversVeteran', 0x1308),
        ('aDefenseManeuversElite', 0x1310),
        ('aDefenseManeuversAce', 0x1320),
        ('aDefenseManeuversBoss', 0x1330),
        ('apDefenseManeuvers', 0x1340),
        ('aRatedManeuverChoices', 0x1358),
        ('aKilrathiManeuverChoices', 0x1778),
        ('acPilotAggression', 0x1910),
        ('acPilotRecovery', 0x1928),
    ), frozenset((
        0x0FA0, 0x0FD6, 0x100C, 0x1042, 0x10AE, 0x10E4,
        0x111A, 0x1150, 0x1186, 0x11BC, 0x11F2, 0x1228,
        0x125E, 0x1340, 0x1344, 0x1348, 0x134C, 0x1350,
    )), owner='game.obj', section_size=6788),
    'screens-characters': UnitData(0x0046E188, 288, (
        ('stSavedCampaignDate', 0x000),
        ('aTalkingHeadOrigins', 0x008),
        ('abBriefingSmallCharacterAnimation', 0x060),
        ('abBriefingLargeCharacterAnimation', 0x078),
        ('aBriefingCharacters', 0x090),
    ), frozenset(range(0x09D, 0x11C, 18)),
       owner='screens.obj', section_size=1258),
    'screens-medals': UnitData(0x0046E2D0, 970, (
        ('asMedalDisplayX', 0x000),
        ('apszMedalNames', 0x010),
        ('pMedalSceneShape', 0x024),
        ('pszMedalsPilotSummary', 0x028),
        ('aBriefingPortraitOffsetX', 0x030),
        ('aBriefingPortraitOffsetY', 0x090),
        ('aBriefingPortraitScale', 0x0F0),
        ('aiBriefingLeftPanelVelocity', 0x1B0),
        ('aiBriefingPodiumVelocity', 0x1E0),
        ('aiBriefingRightPanelVelocity', 0x210),
        ('abBriefingPodiumFrames', 0x240),
        ('abDebriefingEstablishDeltas', 0x268),
        ('nDebriefingLeftX', 0x29C),
        ('nDebriefingPilotX', 0x2A0),
        ('nDebriefingRightX', 0x2A4),
        ('nDebriefingOfficerX', 0x2A8),
        ('nDebriefingPodiumX', 0x2AC),
        ('nConversationCharacter', 0x2B0),
        ('nTalkingHeadFace', 0x2B4),
        ('nConversationBackdropFrame', 0x2B8),
        ('bConversationConstellation', 0x2BC),
        ('bConversationOverlay', 0x2C0),
    ), frozenset((*range(0x010, 0x024, 4), 0x028)),
       owner='screens.obj', section_size=1258),
    'killbrd': UnitData(0x00470090, 1508, (
        ('pszBlankRoomMenuLabel', 0x000),
        ('apszPilotRankNames', 0x008),
        ('stInitialCampaignState', 0x020),
        ('aInitialPilotRecords', 0x078),
        ('abSpiritRecRoomAnimation', 0x1D0),
        ('abHunterRecRoomAnimation', 0x200),
        ('abAngelRecRoomAnimation', 0x228),
        ('abKnightRecRoomAnimation', 0x258),
        ('abIcemanRecRoomAnimation', 0x280),
        ('abManiacRecRoomAnimation', 0x2A8),
        ('abPaladinRecRoomAnimation', 0x2C0),
        ('abBossmanRecRoomAnimation', 0x2F8),
        ('abShotglassIdleAnimation', 0x328),
        ('abShotglassGlassAnimation', 0x340),
        ('abShotglassPourAnimation', 0x360),
        ('abShotglassWipeAnimation', 0x3B0),
        ('apRecRoomAnimations', 0x3C8),
        ('aRecRoomCharacterOrigins', 0x400),
        ('stRecRoomPanelOrigin', 0x40C),
        ('aRecRoomMenuRegions', 0x410),
        ('apszRecRoomBaseLabels', 0x458),
        ('apszRecRoomMenuLabels', 0x468),
        ('bPanRoomTransition', 0x480),
        ('stChalkBoardDate', 0x484),
        ('asChalkBoardPilotOrder', 0x488),
    ), frozenset((0x000, *range(0x008, 0x01C, 4),
                  *range(0x3C8, 0x3E8, 4), *range(0x3EC, 0x3FC, 4),
                  0x458, 0x45C, 0x460, 0x474, 0x478, 0x47C))),
    'gr': UnitData(0x00470678, 2030, (
        ('abPaletteTranslation', 0x000),
        ('awAbsoluteCosine', 0x100),
        ('awAbsoluteSine', 0x3D0),
    ), frozenset()),
    'gr-rdata': UnitData(0x004631C8, 56, (
        ('szGameRegistryKey', 0x00),
        ('szCheaterRegistryValue', 0x30),
    ), frozenset(), storage='.rdata', owner='gr.obj'),
    'gr-bss-prefix': UnitData(0x004875A8, 64024, (
        ('stRasterSurface', 0x00),
        ('abShapeTransformScratch', 0x18),
    ), frozenset(), storage='.bss', owner='gr.obj', section_size=64024),
    # The original owner is unresolved. This verifies the constants and
    # their relative layout in the temporary globals object's .rdata only.
    'globals-angular': UnitData(0x004630DC, 12, (
        ('DAT_004630dc', 0x00),
        ('DAT_004630e0', 0x04),
        ('DAT_004630e4', 0x08),
    ), frozenset(), storage='.rdata', owner='globals.obj'),
    'damage-bss': UnitData(0x00476554, 4, (
        ('nDisplayedComponentDamage', 0x00),
    ), frozenset(), storage='.bss', owner='damage.obj'),
    'music-bss': UnitData(0x00476558, 200, (
        ('szSfxWavePath', 0x00),
    ), frozenset(), storage='.bss', owner='music.obj'),
    'eventmgr-bss': UnitData(0x004865A8, 4096, (
        ('abCursorSaveArea', 0x00),
    ), frozenset(), storage='.bss', owner='eventmgr.obj'),
    'nav-bss': UnitData(0x00475E80, 368, (
        ('aNavMapLabels', 0x00),
        ('aNavMapExclusionRects', 0xC8),
    ), frozenset(), storage='.bss', owner='nav.obj'),
    'cockpt-bss': UnitData(0x00475FF0, 1024, (
        ('abMouseCursorBackground', 0x00),
    ), frozenset(), storage='.bss', owner='cockpt.obj'),
}


ADJACENT_UNITS = (
    ('nav', 'nearheap', 'gameflow', 'title'),
    ('barracks', 'fade', 'debug', 'disk', 'ship', 'init', 'logic'),
    ('damage', 'view'),
    ('ending', 'input', 'comm', 'video', 'joycal'),
    ('killbrd', 'gr'),
    ('sound', 'damage-bss', 'music-bss'),
    ('eventmgr-bss', 'gr-bss-prefix'),
    ('nav-bss', 'cockpt-bss', 'pilot'),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def data_section_size(path: str, storage: str) -> int:
    data = Path(path).read_bytes()
    require(len(data) >= 20, f'{path}: truncated COFF header')
    machine, sections, _, _, _, optional, _ = struct.unpack_from('<HHIIIHH', data)
    require(machine == 0x14C, f'{path}: expected an i386 COFF object')
    require(len(data) >= 20 + optional + sections * 40,
            f'{path}: truncated COFF section table')
    sizes = []
    for index in range(sections):
        offset = 20 + optional + index * 40
        if data[offset:offset + 8].rstrip(b'\0').decode('ascii') == storage:
            size, raw_pointer, reloc_pointer = struct.unpack_from('<III', data, offset + 16)
            reloc_count = struct.unpack_from('<H', data, offset + 32)[0]
            flags = struct.unpack_from('<I', data, offset + 36)[0]
            if storage == '.bss':
                require(flags & 0xC0 == 0x80 and raw_pointer == 0 and
                        reloc_pointer == 0 and reloc_count == 0,
                        f'{path}: .bss must contain uninitialized storage only')
            else:
                require(flags & 0xC0 == 0x40,
                        f'{path}: {storage} must contain initialized storage')
                if storage == '.rdata':
                    require(flags & 0xC0000000 == 0x40000000,
                            f'{path}: .rdata must be readable and not writable')
            sizes.append(size)
    require(len(sizes) == 1, f'{path}: expected one {storage} section')
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

    starts = {}
    for unit in args.unit or UNITS:
        block = UNITS[unit]
        owner = block.owner or unit + '.obj'
        start = address(owner, block.globals[0][0])
        starts[unit] = start
        section_size = block.size if block.section_size is None else block.section_size
        require(section_size >= block.size,
                f'{unit}: verified span exceeds its containing section')
        require(data_section_size(str(args.objects_root / owner), block.storage) == section_size,
                f'{owner}: {block.storage} must contain exactly {section_size} bytes')
        for name, offset in block.globals:
            require(address(owner, name) == start + offset,
                    f'{name}: incorrect offset within {unit} data')

        for image, base in ((original, block.original_start), (rebuilt, start)):
            section = image.section_for_va(base)
            pe_storage = '.data' if block.storage == '.bss' else block.storage
            require(section is not None and section.name == pe_storage and
                    base + block.size <= section.start + section.virtual_size,
                    f'{image.path}: {unit} block must occupy the {pe_storage} section')
            if block.storage != '.bss':
                require(base + block.size <= section.start + section.rawsize,
                        f'{image.path}: {unit} block must occupy initialized {pe_storage}')
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
        scope = ' (partial section)' if block.section_size is not None else ''
        print(f'{owner}{scope}: {block.size} {block.storage} bytes match with one address shift; '
              f'{len(block.globals)} globals and {len(block.pointers)} '
              'pointer relocations verified')
    for group in ADJACENT_UNITS:
        if not all(unit in starts for unit in group):
            continue
        first, last = UNITS[group[0]], UNITS[group[-1]]
        original_start = first.original_start
        rebuilt_start = starts[group[0]]
        size = last.original_start + last.size - original_start
        for unit in group:
            require(starts[unit] - rebuilt_start ==
                    UNITS[unit].original_start - original_start,
                    f'{unit}: incorrect position in adjacent data run')
        pointers = {UNITS[unit].original_start - original_start + offset
                    for unit in group for offset in UNITS[unit].pointers}
        for image, base in ((original, original_start), (rebuilt, rebuilt_start)):
            offsets = {va - base for va in image.relocations()
                       if base <= va < base + size}
            require(offsets == pointers,
                    f'{image.path}: incorrect relocations in adjacent data run')
        ranges = (RelocatedAddressRange(original_start, original_start + size,
                                       rebuilt_start),)
        ranges += tuple(RelocatedAddressRange(base, base + length, address(None, name))
                        for unit in group
                        for name, base, length in UNITS[unit].external_globals)
        original_bytes = original.read(original_start, size)
        rebuilt_bytes = rebuilt.read(rebuilt_start, size)
        require(original_bytes is not None and rebuilt_bytes is not None,
                'cannot read adjacent data run')
        if pointers:
            matches = relocated_pointer_match(original_bytes, rebuilt_bytes, {}, ranges)
        else:
            matches = AddressComparison(original_start, size, rebuilt_start,
                                        original_bytes, rebuilt_bytes).matches
        require(matches,
                'adjacent data run differs, including inter-object alignment bytes')
        padding = size - sum(UNITS[unit].size for unit in group)
        print(f'{group[0]} through {group[-1]}: {size} adjacent data bytes match; '
              f'{len(group)} unit positions and {padding} alignment bytes verified')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f'unit data verification failed: {error}', file=sys.stderr)
        sys.exit(1)
