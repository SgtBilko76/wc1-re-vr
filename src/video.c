/*
 *  Video resource shutdown and palette allocation.
 *
 *  Address range 4318f0-431a0f (see docs/ORDER.md).
 */
#include "wc1.h"

unsigned short nVideoMode = 0xffff; /* 0x0046b168 */

/* Function start: 0x4318F0 */
void __stdcall ShutdownVideoHook(short mode)
{
    ReleaseVideoResourcesHook();
}

/* Function start: 0x431900 */
short __stdcall ReserveContiguousPaletteEntries(short entryCount)
{
    short entry;
    short freeEntries;
    short firstEntry;

    freeEntries = 0;
    entry = 0;
    firstEntry = 0;
    for (;;) {
        if (awPaletteEntryAllocation[entry] != 0) {
            freeEntries = 0;
            firstEntry = (short)(entry + 1);
        } else
            freeEntries++;
        if (freeEntries == entryCount)
            break;
        entry++;
        if (entry >= 256)
            return -1;
    }

    freeEntries = 0;
    while (freeEntries < entryCount) {
        awPaletteEntryAllocation[firstEntry + freeEntries] =
            entryCount;
        freeEntries++;
    }
    return firstEntry;
}

/* Function start: 0x431970 */
void __stdcall ReleaseContiguousPaletteEntries(short firstEntry)
{
    short entry;
    short entryCount;

    entry = 0;
    entryCount = awPaletteEntryAllocation[firstEntry];
    while (entry < entryCount) {
        awPaletteEntryAllocation[firstEntry + entry] = 0;
        entry++;
    }
}

/* Function start: 0x4319B0 */
void PrintPaletteAllocationMap(void)
{
    short index;
    short row;
    short column;

    index = 0;
    row = 4;
    do {
        column = 0x40;
        do {
#if !defined(SDL_PORT) || defined(SDL_PORT_LEGACY_DEBUG_OUTPUT)
            printf("%c", awPaletteEntryAllocation[index++] < 1 ?
                   '_' : '.');
#else
            index++;
#endif
            column--;
        } while (column != 0);
#if !defined(SDL_PORT) || defined(SDL_PORT_LEGACY_DEBUG_OUTPUT)
        printf("\n");
#endif
        row--;
    } while (row != 0);
}
