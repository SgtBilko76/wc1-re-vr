/*
 *  Incremental palette transitions and the adjacent DOS string append entry.
 *
 *  Address range 41c510-41c75f (see docs/ORDER.md). The initialization flag
 *  follows the barracks literal pool. Ownership of the string entry remains
 *  provisional within this range.
 */
#define WC1_FADE_SOURCE
#include "wc1.h"

short nPaletteTransitionInitialise = 1; /* 0x00469640 */
short *pPaletteTransitionDirection; /* 0x005a7d88 */
short *pPaletteTransitionDelta; /* 0x005a7d8c */
short nPaletteTransitionMaxDelta; /* 0x005a7d90 */
short *pPaletteTransitionAccumulator; /* 0x005a7d94 */
short nPaletteTransitionCountdown; /* 0x005a7d98 */

/* Function start: 0x41C510 */
unsigned short __stdcall StepPaletteTransition(short *current,
                                                const short *target,
                                                short componentCount)
{
    unsigned int byteCount;
    short difference;
    short index;
    short previousCountdown;

    if (nPaletteTransitionInitialise != 0) {
        byteCount = (unsigned int)(componentCount * 2);
        pPaletteTransitionAccumulator =
            AllocateTaggedMemory(byteCount, 0);
        pPaletteTransitionDelta =
            AllocateTaggedMemory(byteCount, 0);
        pPaletteTransitionDirection =
            AllocateTaggedMemory(byteCount, 0);
        if (pPaletteTransitionAccumulator == 0 ||
            pPaletteTransitionDelta == 0 ||
            pPaletteTransitionDirection == 0) {
            if (pPaletteTransitionAccumulator != 0)
                ReleasePacketHandle(pPaletteTransitionAccumulator);
            if (pPaletteTransitionDelta != 0)
                ReleasePacketHandle(pPaletteTransitionDelta);
            if (pPaletteTransitionDirection != 0)
                ReleasePacketHandle(pPaletteTransitionDirection);
            return 0;
        }

        nPaletteTransitionMaxDelta = 0;
        for (index = 0; index < componentCount; index++) {
            difference = (short)(current[index] - target[index]);
            if (difference < 0) {
                difference = (short)-difference;
                pPaletteTransitionDirection[index] = 4;
            } else {
                pPaletteTransitionDirection[index] = -4;
            }
            pPaletteTransitionDelta[index] = difference;
            if (nPaletteTransitionMaxDelta < difference)
                nPaletteTransitionMaxDelta = difference;
        }

        difference = (short)(nPaletteTransitionMaxDelta / 4);
        for (index = 0; index < componentCount; index++) {
            pPaletteTransitionAccumulator[index] = difference;
        }
        nPaletteTransitionInitialise = 0;
        nPaletteTransitionCountdown =
            (short)(nPaletteTransitionMaxDelta / 4);
    }

    previousCountdown = nPaletteTransitionCountdown;
    nPaletteTransitionCountdown--;
    if (previousCountdown == 0) {
        ReleasePacketHandle(pPaletteTransitionAccumulator);
        ReleasePacketHandle(pPaletteTransitionDelta);
        ReleasePacketHandle(pPaletteTransitionDirection);
        nPaletteTransitionInitialise = 1;
        return 0;
    }

    for (index = 0; index < componentCount; index++) {
        pPaletteTransitionAccumulator[index] =
            (short)(pPaletteTransitionAccumulator[index] +
                    pPaletteTransitionDelta[index]);
        if (pPaletteTransitionAccumulator[index] >
            nPaletteTransitionMaxDelta) {
            pPaletteTransitionAccumulator[index] =
                (short)(pPaletteTransitionAccumulator[index] -
                        nPaletteTransitionMaxDelta);
            current[index] =
                (short)(current[index] +
                        pPaletteTransitionDirection[index]);
        }
    }
    return 1;
}

/* Function start: 0x41C740 */
char *__stdcall DosStrcat(char *destination, const char *source)
{
    char *end;

    end = DosStrchr(destination, 0);
    DosStrcpy(end, source);
    return destination;
}
