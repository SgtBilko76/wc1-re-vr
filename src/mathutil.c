/*
 *  Integer min/max used across the game core.
 *
 *  Address range 0x41d0c0-0x41d11f (provisional -- see docs/ORDER.md).
 *  Boundary evidence: MinShort/MaxShort pair, 94 call sites, no other content in the gap.
 */
#include "wc1.h"

/* Function start: 0x41D0C0 */
short MinShort(short a, short b)
{
    if (a < b)
        return a;
    return b;
}

/* One cockpit sizing call supplies a third argument; only the first two are read. */
/* Function start: 0x41D0E0 */
short MaxShort(short a, short b, ...)
{
    if (a > b)
        return a;
    return b;
}

/* Function start: 0x41D100 */
void FreePacketAndClear(void *slot, unsigned short releaseFlags)
{
#ifdef SDL_PORT
    void *packet;

    memcpy(&packet, slot, sizeof(packet));
    if (packet != 0) {
        ReleasePacketHandle(packet);
        packet = 0;
        memcpy(slot, &packet, sizeof(packet));
    }
#else
    void **p;

    p = (void **)slot;
    if (*p != 0) {
        ReleasePacketHandle(*p);
        *p = 0;
    }
#endif
}
