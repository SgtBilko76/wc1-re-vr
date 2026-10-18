/*
 *  Scripted views, constellation effects, and cockpit target rendering.
 *
 *  Address range 42cdb0-42e31f (see docs/ORDER.md).
 *  Packet and viewport helpers remain in this provisional contiguous unit.
 */
#include "wc1.h"

short nScriptedViewObject = -1; /* 0x0046a8d0 */
int bScriptedView = 0; /* 0x0046a8d4 */
static short asConstellationVelocity[16] = { /* 0x0046a8d8 */
    8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1
};
static short asConstellationFrame[16] = { /* 0x0046a8f8 */
    0, 16, 16, 0, 4, 4, 20, 20, 24, 8, 8, 24, 28, 12, 12, 28
};
static short nConstellationDirection = -1; /* 0x0046a918 */
static unsigned char *pPacketDecompressionWorkspace = 0; /* 0x0046a91c */
static unsigned short wPacketDecompressionInputSizeOverride = 0; /* 0x0046a920 */
static unsigned short wPacketCompressionFormatFlags = 0; /* 0x0046a924 */
static ShortRect aTargetArmorClipRects[4] = { /* 0x0046a928 */
    { 12, -20, 29, 20 },
    { -11, 1, 11, 20 },
    { -11, -20, 11, 0 },
    { -29, -20, -12, 20 }
};

/* Function start: 0x42CDB0 */
unsigned int parse_view_script(void)
{
    FixedVector vector;
    short obj;
    short command;

    if (*pViewScript == 13)
        return 0;
    while (*pViewScript != 14) {
        command = *pViewScript++;
        if (command == -1) {
            bScriptedView = 0;
            nScriptedViewObject = -1;
            return 0;
        }
        switch (command) {
        case 0:
#ifdef SDL_PORT
            aShipPosition[61].x =
                (int)*pViewScript++ * 0x100;
            aShipPosition[61].y =
                (int)*pViewScript++ * 0x100;
            aShipPosition[61].z =
                (int)*pViewScript++ * 0x100;
#else
            aShipPosition[61].x =
                (int)*pViewScript++ << 8;
            aShipPosition[61].y =
                (int)*pViewScript++ << 8;
            aShipPosition[61].z =
                (int)*pViewScript++ << 8;
#endif
            break;
        case 1:
            alter_yaw(*pViewScript++, 61);
            alter_pitch(*pViewScript++, 61);
            alter_roll(*pViewScript++, 61);
            break;
        case 2:
#ifdef SDL_PORT
            ScaleFixedVector(&aShipForwardVector[61],
                             (int)*pViewScript++ * 0x100,
                             &aShipVelocity[61]);
#else
            ScaleFixedVector(&aShipForwardVector[61],
                             (int)*pViewScript++ << 8,
                             &aShipVelocity[61]);
#endif
            break;
        case 3:
            force_view(*pViewScript++,
                       nScriptedViewObject);
            break;
        case 4:
            nEyePitchGoal =
                (short)-*pViewScript++;
            nEyePitchRate = *pViewScript++;
            break;
        case 5:
            nEyePitchGoal = *pViewScript++;
            nEyePitchRate = *pViewScript++;
            break;
        case 6:
            nEyeYawGoal = *pViewScript++;
            nEyeYawRate = *pViewScript++;
            break;
        case 7:
            nEyeYawGoal =
                (short)-*pViewScript++;
            nEyeYawRate = *pViewScript++;
            break;
        case 8:
            nEyeRollGoal = *pViewScript++;
            nEyeRollRate = *pViewScript++;
            break;
        case 9:
            copy_frame(61, 63);
            alter_yaw(*pViewScript++, 63);
            alter_pitch(*pViewScript++, 63);
            alter_roll(*pViewScript++, 63);
#ifdef SDL_PORT
            ScaleFixedVector(&aShipForwardVector[63],
                             (int)*pViewScript++ * 0x100,
                             &vector);
#else
            ScaleFixedVector(&aShipForwardVector[63],
                             (int)*pViewScript++ << 8,
                             &vector);
#endif
            AddFixedVectors(&aShipVelocity[61], &vector,
                            &aShipVelocity[61]);
            break;
        case 10:
            aShipVelocity[61] =
                aShipVelocity[nScriptedViewObject];
            break;
        case 11:
            copy_frame(nScriptedViewObject, 61);
            break;
        case 12:
            aShipPosition[61] =
                aShipPosition[nScriptedViewObject];
            break;
        case 15:
            ComputeVectorDelta(&aShipPosition[61],
                &aShipPosition[nScriptedViewObject],
                &vector);
            aShipForwardVector[61] = vector;
            fix_objects_ijk(61);
            break;
        case 16:
            obj = 0;
            while (obj < 10) {
                if (nShipMissionIndices[obj] ==
                    *pViewScript)
                    break;
                obj++;
            }
            if (obj < 10)
                nScriptedViewObject = obj;
            pViewScript++;
            break;
        }
        if (*pViewScript == 13)
            return 0;
    }
    asObjectCounter[61] = pViewScript[1];
    return 0;
}

/* Function start: 0x42D1C0 */
unsigned int update_scripted_view(void)
{
    short counter;
    int command;

    command = *pViewScript;
    switch (command) {
    case 13:
        if ((nEyeYawGoal == nEyePitchGoal) !=
            nEyeRollGoal) {
            pViewScript++;
            parse_view_script();
        }
        break;
    case 14:
        counter = asObjectCounter[61];
        asObjectCounter[61]--;
        if (counter < 1) {
            pViewScript += 2;
            parse_view_script();
        }
        break;
    }
    return 0;
}

/* Function start: 0x42D230 */
void initialize_scripted_view(const short *script)
{
    bScriptedView = 1;
    zero_vector(&aShipVelocity[61]);
    init_ijk(61);
    pViewScript = script;
    parse_view_script();
    asObjectCollisionRadius[61] = 100;
}

/* Function start: 0x42D270 */
unsigned int InitializeFireworks(void)
{
    int empty = -1;
    short i = 0;

    do {
        aFireworks[i].frame = (short)empty;
        i = i + 1;
    } while (i < 0x1e);
    return 0;
}

/* Function start: 0x42D2A0 */
short TheEndFireWorks(Viewport *viewport, short count)
{
    short index;
    short emptyCount;

    emptyCount = 0;
    index = count;
    while (--index >= 0) {
        if (aFireworks[index].frame == -1) {
            emptyCount++;
        } else {
            DrawSpriteDefault(
                viewport, aFireworks[index].x,
                aFireworks[index].y,
                pFireworkShape,
                (short)(aFireworks[index].frame +
                        aFireworks[index].variant * 8));
            if (aFireworks[index].frame++ == 7) {
                aFireworks[index].frame = -1;
                FlushSoundEffectsAndLog(
                    aFireworks[index].soundHandle, index);
            } else if (aFireworks[index].frame == 1) {
                aFireworks[index].soundHandle =
                    ((unsigned int (__cdecl *)(
                        const unsigned char *, int, int, short, short,
                        int))SoundFxTick)(
                            aabSoundEffectDescriptors[36],
                            0, 127,
                            (signed char)(
                                127 -
                                (int)aFireworks[index].x *
                                    127 / 319),
                            index, 1);
            }
        }
    }
    return emptyCount;
}

/* Function start: 0x42D390 */
unsigned int InitializeConstellationField(Viewport *viewport,
                                          short direction,
                                          short density)
{
    short height;
    short index;
    int particleIndex;
    short randomIndex;
    short width;

    pConstellationViewport = viewport;
    nConstellationDirection = direction;
    width = (short)(viewport->right - viewport->left);
    height = (short)(viewport->bottom - viewport->top);
    nConstellationStarCount = (short)(density * 10 / 16);
    nConstellationParticleCount =
        (short)(density * 16 / 16);
    index = 0;
    while (index < nConstellationStarCount) {
        aConstellationStars[index].x =
            RandomInRange(0, width);
        aConstellationStars[index].y =
            RandomInRange(0, height);
        aConstellationStars[index].frame =
            (short)(RandomInRange(0, 5) + 32);
        index++;
    }
    index = 0;
    while (index < nConstellationParticleCount) {
        randomIndex = RandomInRange(0, 15);
        particleIndex = index;
        index++;
        aConstellationParticles[particleIndex].x =
            (short)(pConstellationViewport->left +
                    RandomInRange(0, width));
        aConstellationParticles[particleIndex].y =
            (short)(pConstellationViewport->top +
                    RandomInRange(0, height));
        aConstellationParticles[particleIndex].velocity =
            (short)(asConstellationVelocity[randomIndex] *
                    nConstellationDirection);
        aConstellationParticles[particleIndex].frame =
            (short)(asConstellationFrame[randomIndex] +
                    RandomInRange(0, 3));
    }
    return 0;
}

/* Function start: 0x42D500 */
unsigned int DrawConstellationField(void)
{
    short height;
    short index;
    int tableOffset;
    short speed;

    height = (short)(pConstellationViewport->bottom -
                     pConstellationViewport->top);
    ClearViewport(pConstellationViewport, cPrimaryViewBufferColour);
    for (index = 0;
         index < nConstellationStarCount;
         index++) {
        DrawSpriteDefault(pConstellationViewport,
                          aConstellationStars[index].x,
                          aConstellationStars[index].y,
                          pConstellationShape,
                          aConstellationStars[index].frame);
    }
    for (index = 0;
         index < nConstellationParticleCount;
         index++) {
        DrawSpriteDefault(pConstellationViewport,
                          aConstellationParticles[index].x,
                          aConstellationParticles[index].y,
                          pConstellationShape,
                          aConstellationParticles[index].frame);
        aConstellationParticles[index].x = (short)(
            aConstellationParticles[index].x +
            aConstellationParticles[index].velocity);
        aConstellationParticles[index].frame = (short)(
            (aConstellationParticles[index].frame & 0xfc) +
            (aConstellationParticles[index].frame + 1) % 4);
        if (nConstellationDirection < 0) {
            if (aConstellationParticles[index].x >=
                pConstellationViewport->left)
                continue;
            tableOffset = (int)RandomInRange(0, 15) * sizeof(short);
            speed = *(short *)((unsigned char *)asConstellationVelocity +
                              tableOffset);
            aConstellationParticles[index].x = (short)(
                pConstellationViewport->right -
                RandomInRange(0, speed));
            aConstellationParticles[index].y = (short)(
                pConstellationViewport->top +
                RandomInRange(0, height));
            aConstellationParticles[index].velocity = (short)-speed;
        } else {
            if (aConstellationParticles[index].x <=
                pConstellationViewport->right)
                continue;
            tableOffset = (int)RandomInRange(0, 15) * sizeof(short);
            speed = *(short *)((unsigned char *)asConstellationVelocity +
                              tableOffset);
            aConstellationParticles[index].velocity = speed;
            aConstellationParticles[index].x = (short)(
                pConstellationViewport->left +
                RandomInRange(0, speed));
            aConstellationParticles[index].y = (short)(
                pConstellationViewport->top +
                RandomInRange(0, height));
        }
        aConstellationParticles[index].frame = (short)(
            *(short *)((unsigned char *)asConstellationFrame +
                       tableOffset) +
            RandomInRange(0, 3));
    }
    return 0;
}

/* Function start: 0x42D730 */
short __stdcall OpenPacketSection(const char *filename, short section,
                                  PacketSectionHandle *handle)
{
    unsigned int sectionEntry;
    unsigned int fileSize;
    unsigned int sectionEnd;
    int sectionIndex;
    int directoryOffset;
    unsigned int directorySize;
    short compression;
    short sectionCount;
    short file;

    file = OpenDataFileOrDie(filename);
    if (file == -1)
        goto failed;
    if (ReadDataFileAtOffset(file, 0, 4, &fileSize) == 0)
        goto failed;
    if (ReadDataFileAtOffset(file, 4, 4, &directorySize) == 0)
        goto failed;
    sectionCount = (short)(directorySize >> 2) - 1;
    if (section >= sectionCount) {
        nPacketError = 3;
        goto failed;
    }
    sectionIndex = (int)section;
    directoryOffset = sectionIndex * 4 + 4;
    if (ReadDataFileAtOffset(file, directoryOffset, 4,
                             &sectionEntry) == 0)
        goto failed;
    handle->finalSection = 0;
    compression = (short)(sectionEntry >> 24) & 0xff;
    sectionEntry &= 0x00ffffff;
    if (sectionCount - sectionIndex == 1) {
        handle->finalSection = 1;
        sectionEnd = fileSize;
    } else {
        directoryOffset += 4;
        if (ReadDataFileAtOffset(file, directoryOffset, 4,
                                 &sectionEnd) == 0)
            goto failed;
        /* Discard the next directory entry's compression byte. */
        ((unsigned char *)&sectionEnd)[3] = 0;
    }
    fileSize = sectionEnd - sectionEntry;
    if (SeekDataFile(file, sectionEntry, 0) != 0) {
        handle->file = file;
        handle->dataOffset = sectionEntry;
        handle->sectionCount = sectionCount;
        handle->compression = compression;
        handle->dataSize = fileSize;
        handle->position = 0;
        return 1;
    }
failed:
    CloseDataFile((unsigned short)file);
    return 0;
}

/* Function start: 0x42D870 */
void __stdcall CloseDataFileByHandle(unsigned short *handle)
{
    CloseDataFile(*handle);
}

/* Function start: 0x42D880 */
void * __stdcall DecompressPacketSection(
    PacketSectionHandle *handle, void *destination, unsigned short flags,
    void *decompressionWorkspace)
{
    volatile unsigned int allocationSize;
    void *packet;
    void *workspace;
    void *largeScratch;
    void *smallScratch;
    unsigned char fallbackAllocations;

    fallbackAllocations = 0;
    packet = 0;
    pLastPacketAllocation = 0;
    if (wPacketCompressionFormatFlags != 0) {
        if ((handle->compression & 0xc0) != 0) {
            nPacketError = 6;
            return 0;
        }
    } else if (handle->compression != 1) {
        nPacketError = 6;
        return 0;
    }

    if (decompressionWorkspace == 0) {
        if (pPacketDecompressionWorkspace != 0) {
            wPacketDecompressionInputSizeOverride = 0;
            decompressionWorkspace =
                pPacketDecompressionWorkspace;
        }
        if (decompressionWorkspace == 0) {
            largeScratch = AllocateTaggedMemory(0x3020, 0);
            smallScratch = AllocateTaggedMemory(0x410, 0);
            if (largeScratch == 0 || smallScratch == 0) {
                if (largeScratch != 0)
                    ReleasePacketHandle(largeScratch);
                if (smallScratch != 0)
                    ReleasePacketHandle(smallScratch);
                fallbackAllocations = 1;
                largeScratch = AllocateTaggedMemory(0x3000, 0x22);
                if (largeScratch == 0) {
                    nPacketError = 1;
                    return 0;
                }
                smallScratch = AllocateTaggedMemory(0x400, 0x22);
                if (smallScratch == 0) {
                    ReleasePacketHandle(largeScratch);
                    nPacketError = 2;
                    return 0;
                }
            }
            workspace = IdentityHandle(largeScratch);
            pPacketDecompressInput = IdentityHandle(smallScratch);
            wPacketDecompressInputSize = 0x400;
            goto initializeDecompressor;
        }
    }

    workspace = IdentityHandle(decompressionWorkspace);
    wPacketDecompressInputSize = 0x400;
    allocationSize = 0x3020;
    pPacketDecompressInput =
        ((PacketDecompressionWorkspace *)decompressionWorkspace)->input;
    pPacketDecompressInput =
        IdentityHandle(pPacketDecompressInput);
    if (wPacketDecompressionInputSizeOverride != 0)
        wPacketDecompressInputSize =
            wPacketDecompressionInputSizeOverride;
    largeScratch = (void *)allocationSize;
    smallScratch = (void *)allocationSize;

initializeDecompressor:
    nPacketDecompressSourceFile = handle->file;
    nPacketDecompressInputPosition = 0;
    nPacketDecompressPending = 0;
    nPacketDecompressWorkspaceSegment =
        (short)(unsigned int)workspace;
    if (workspace != 0)
        nPacketDecompressWorkspaceSegment++;

    if (SeekPacketSection(handle, 0, 0) == -1) {
        nPacketError = 5;
        return 0;
    }
    if (destination == 0) {
        if (ReadPacketSectionData(handle, (void *)&allocationSize, 4) != 0) {
            pLastPacketAllocation = AllocateTaggedMemory(
                allocationSize, flags);
            if (pLastPacketAllocation == 0)
                nPacketError = 4;
        }
    } else {
        if (SeekPacketSection(handle, 4, 0) == -1) {
            nPacketError = 5;
            return 0;
        }
        pLastPacketAllocation = destination;
    }

    if (nPacketError == 0) {
        packet = pLastPacketAllocation;
        if (ReadPacketSectionData(
                handle, pPacketDecompressInput,
                wPacketDecompressInputSize) == 0)
            packet = 0;
        nPacketDecompressResult = GetPacketDecompressionResult(packet);
        if (packet != 0)
            FinishPacketDecompressionHook();
    }
    if (decompressionWorkspace == 0) {
        ReleasePacketHandle(smallScratch);
        if (fallbackAllocations != 0)
            ReleasePacketHandle(largeScratch);
        else
            ReleasePacketHandle(largeScratch);
    }
    return packet;
}

/* Function start: 0x42DB70 */
short GetTargetColourIndex(void)
{
    short v = (short)nVideoMode;

    if ((short)nVideoMode == -1)
        v = 0x13;
    return v;
}

/* Function start: 0x42DB90 */
void show_target_disp(void)
{
    short target;
    int targetIndex;
    ObjectTypeData *typeData;
    enum ObjectType objectType;
    signed char rating;
    short x;
    short y;
    short frame;
    short *maximumArmor;
    short armor;
    Viewport targetViewport;

    DrawTextAt(&stRightVduTextContext,
               stRightVdu.left, stRightVdu.top,
               "", 2);
    if (nTargetLockMode != 0) {
        DrawFormattedText("%F%s%F",
                          (unsigned int)cRedColour,
                          "   LOCKED TARGET",
                          (unsigned int)cDefaultTextColour);
    } else {
        DrawFormattedText("%F%s",
                          (unsigned int)cDefaultTextColour,
                          "  AUTO TARGETTING");
    }
    target = acShipTarget[0];
    if (target != -1 &&
        (aeObjectClass[target] < OBJECT_CLASS_SHIP ||
         aeSpecialManeuver[target] ==
             SPECIAL_MANEUVER_UNKNOWN_9)) {
        target = -1;
        acShipTarget[0] = -1;
    }
    cTargetDisplayObject = (signed char)target;
    DrawFormattedText("\nTarget:");
    if (target == -1) {
        DrawFormattedText(" None");
        return;
    }
    targetIndex = (int)target;
    objectType = aeObjectType[targetIndex];
    typeData = &aObjectTypeData[objectType];
    rating = acShipRating[targetIndex];
    if (rating >= 0 && rating <= 7) {
        DrawFormattedText(
            " %s",
            apWingmanPilots[(int)rating]->callsign);
    } else if (rating >= 9 && rating <= 12) {
        DrawFormattedText(
            " %s",
            apszKilrathiAceNames[(int)rating - 9]);
    } else {
        DrawFormattedText(" %s",
                          typeData->displayName);
    }
    DrawFormattedText("\nRange : ");
    InitializeCockpitReadout(1, &stRightVduTextContext);
    if (asObjectScreenX[targetIndex] == (short)0x8001) {
        cTargetDisplayObject = -1;
        return;
    }

    x = (short)(stRightVdu.left + 0x25);
    y = (short)(stRightVdu.top + 0x26);
    frame = (short)((3 - MinShort(
        (short)((aasShipShield[targetIndex][1] * 6) /
                typeData->shieldAft), 3)) * 2);
    if (frame < 6)
        DrawSpriteDefault(&stRightVdu, x, y,
                          pCockpitIndicatorShape, frame);

    targetViewport = stRightVdu;
    maximumArmor = &typeData->armorFront;
    for (armor = 0; armor < 4; armor++) {
        targetViewport.left =
            (short)(aTargetArmorClipRects[armor].left + x);
        targetViewport.top =
            (short)(aTargetArmorClipRects[armor].top + y);
        targetViewport.right =
            (short)(aTargetArmorClipRects[armor].right + x);
        targetViewport.bottom =
            (short)(aTargetArmorClipRects[armor].bottom + y);
        if (aasShipArmor[targetIndex][armor] >
            (short)(maximumArmor[armor] >> 1)) {
            DrawSpriteDefault(&targetViewport, x, y,
                              typeData->shape, 0);
        } else {
            DrawSpriteDefault(&targetViewport, x, y,
                              typeData->shape, 1);
        }
    }

    DrawSpriteDefault(&stRightVdu, x, y, typeData->shape, 2);
    frame = (short)((3 - MinShort(
        (short)((aasShipShield[targetIndex][0] * 6) /
                typeData->shieldFore), 3)) * 2);
    if (frame < 6)
        DrawSpriteDefault(&stRightVdu, x, y,
                          pCockpitIndicatorShape,
                          (short)(frame + 1));
}

/* Function start: 0x42DEA0 */
void DrawTargetRangeReadout(void)
{
    short target;

    target = acShipTarget[0];
#ifdef SDL_PORT
    /* The original tests the table before the -1 sentinel.  At target -1 it
       reads the zero-filled gap at 0x0059c3bc, so the comparison is false. */
    if (target != -1 &&
#else
    if (
#endif
        aeSpecialManeuver[target] ==
        SPECIAL_MANEUVER_UNKNOWN_9) {
        acShipTarget[0] = -1;
        InvalidateVduMode(1);
        return;
    }
    if (target != -1 &&
        aeObjectClass[target] < OBJECT_CLASS_SHIP) {
        acShipTarget[0] = -1;
        target = -1;
    }
    if (cTargetDisplayObject != target ||
        (short)(nRenderedSpaceFrame % 8) == 0) {
        set_new_vdu(1);
        show_target_disp();
    }
    if (target == -1)
        return;

    if (asObjectScreenX[target] != (short)0x8001) {
        if ((unsigned short)asObjectDistance[target] > 30000) {
            strcpy(szTextScratchBuffer, "TOO FAR");
        } else {
            strcpy(szTextScratchBuffer + strlen(
                itoa((unsigned short)asObjectDistance[target],
                     szTextScratchBuffer, 10)), " m");
        }
    } else {
        strcpy(szTextScratchBuffer, "----- m");
    }

    DrawCockpitReadout(1, szTextScratchBuffer);
    if (nTargetLockCountdown == 0) {
        if (bTargetLockAcquired == 1) {
            bTargetLockAcquired = 0;
            return;
        }
    } else if (bTargetLockReadoutDirty != 0) {
        EraseCockpitReadoutRegion(&stRightVdu,
                                  stRightVdu.left,
                                  (short)(stRightVdu.bottom - 6),
                                  stRightVdu.right,
                                  stRightVdu.bottom,
                                  (short)cBlackColour);
        bTargetLockReadoutDirty = 0;
    }
}

/* Function start: 0x42E020 */
void LogDisplayMode(const char *mode)
{
    SystemDebugPrintf("display_mode == '%s'\n", mode);
    ClearDebugPauseFlags();
    PumpMessagesDuringWait();
    exit(1);
}

/* Function start: 0x42E050 */
short __stdcall CalcRectangleArea(const Viewport *viewport)
{
    short height;
    short width;

    if (nVideoMode != 0x13)
        LogDisplayMode("not MCGA");
    height = (short)(viewport->bottom - viewport->top + 1);
    width = (short)(viewport->right - viewport->left + 1);
    return (short)(height * width);
}

/* Function start: 0x42E090 */
unsigned short __stdcall AllocateViewport(Viewport *viewport,
                                          short clearColour, short flags)
{
    unsigned short *rowOffsets;
    unsigned short top;
    short left;
    unsigned short width;
    unsigned short height;
    unsigned short row;
    short offset;

    top = (unsigned short)viewport->top;
    height = (unsigned short)(viewport->bottom - top + 1);
    width = (unsigned short)(viewport->right - viewport->left + 1);
    left = viewport->left;
    nAllocateViewportCalls++;
    if (nVideoMode != 0x13)
        LogDisplayMode("not MCGA");
    viewport->allocation = AllocateTaggedMemory(
        (unsigned int)width * height, (unsigned short)(flags + 2));
    apViewportAllocations[
        nViewportAllocationCount++] = viewport->allocation;
    if (viewport->allocation == 0)
        return 0;
    viewport->pixels = viewport->allocation;

    rowOffsets = AllocateTaggedMemory(
        (top + (unsigned int)height) * sizeof(unsigned short) + 4,
        0);
    viewport->rowOffsets = rowOffsets;
    if (rowOffsets == 0) {
        if (nVideoMode != 0x13)
            ReleasePacketHandle(viewport->allocation);
        return 0;
    }

    row = 0;
    offset = 0;
    do {
        rowOffsets[top + row] = (unsigned short)(offset - left);
        row++;
        offset = (short)(offset + width);
    } while ((unsigned int)row < (unsigned int)height + 2);
    if (clearColour != -1)
        ClearViewport(viewport, clearColour);
    return 1;
}

/* Function start: 0x42E1D0 */
void __stdcall AlignSpriteFrameToRectCorner(
    const ShortRect *rectangle, ShortPoint *position, short corner,
    unsigned char *shape, short frame)
{
    ShortRect frameBounds;
    short x;
    short y;

    switch (corner) {
    case 0:
        x = rectangle->left;
        y = rectangle->top;
        break;
    case 1:
        x = rectangle->right;
        y = rectangle->top;
        break;
    case 2:
        x = rectangle->left;
        y = rectangle->bottom;
        break;
    case 3:
        x = rectangle->right;
        y = rectangle->bottom;
        break;
    }

    GetShapeFrameBounds(&frameBounds.left, x, y, shape, frame);
    position->x = x;
    position->y = y;
    switch (corner) {
    case 0:
        position->x = (short)(x * 2 - frameBounds.left);
        position->y = (short)(y * 2 - frameBounds.top);
        return;
    case 1:
        position->x = (short)(x * 2 - frameBounds.right);
        position->y = (short)(y * 2 - frameBounds.top);
        return;
    case 2:
        position->x = (short)(x * 2 - frameBounds.left);
        position->y = (short)(y * 2 - frameBounds.bottom);
        return;
    case 3:
        position->x = (short)(x * 2 - frameBounds.right);
        position->y = (short)(y * 2 - frameBounds.bottom);
        return;
    }
}
