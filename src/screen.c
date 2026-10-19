/*
 *  Mission requests, viewport setup, and packet-memory management.
 *
 *  Address range 42efc0-42fa8f (see docs/ORDER.md).
 */
#include "wc1.h"

/* Linker commons defined here; LINK orders them by identifier hash. */
void *apPacketHandles[0x1000]; /* 0x0059e530 */
int aiPacketHandleOffsets[0x1000]; /* 0x005a2530 */
int nPacketHandleCount; /* 0x005a6530 */
Viewport *pAllocatedScreenViewport; /* 0x005a6534 */
int *pFullScreenRecord; /* 0x005a6538 */
unsigned short awScreenRowOffsets[202]; /* 0x005a6540 */
ScreenAllocation stScreenAllocation; /* 0x005a66e0 */
ScreenAllocation *pScreenAllocation; /* 0x005a66ec */

/* Function start: 0x42EFC0 */
void cleanup_objectives(void)
{
    int objectiveType;
    short objective;
    short home;
    short proximity;
    short index;
    short object;

    objective = 0;
    home = find_ship_index(stMissionHeader.homeMissionShip);
    proximity = aMissionNavPoints[
        stMissionHeader.entryNavPoint].proximityRadius;
    objectiveType = aMissionObjectives[objective].type;
    for (; objectiveType != -1;
         objective++,
         objectiveType = aMissionObjectives[objective].type) {
        if (achieved(objective) == 0) {
            index = (short)aMissionObjectives[objective].index;
            switch (objectiveType) {
            case 0:
            case 1:
            case 3:
                if (visited(objective) != 0)
                    flag_objective(objective, 2);
                break;
            case 2:
                switch (aMissionShips[index].missionType) {
                case MISSION_TYPE_GOTO_WARP:
                    if (aMissionShips[index].state == 2) {
                        flag_objective(objective, 2);
                        if (aMissionShips[index].type ==
                                OBJECT_TYPE_HORNET ||
                            aMissionShips[index].type ==
                                OBJECT_TYPE_DRAYMAN)
                            affect_mission_score(0, 5, -1);
                        else
                            affect_mission_score(0, 9, -1);
                    }
                    break;
                case MISSION_TYPE_WARP_ARRIVE:
                case MISSION_TYPE_COME_HOME:
                    object = find_ship_index(index);
                    if (object != -1 && home != -1 &&
                        distance_from_object(object, home) < proximity)
                        aMissionShips[index].state = 1;
                    if (aMissionShips[index].state == 1 &&
                        sighted(objective) != 0) {
                        flag_objective(objective, 2);
                        if (aMissionShips[index].type ==
                                OBJECT_TYPE_HORNET ||
                            aMissionShips[index].type ==
                                OBJECT_TYPE_DRAYMAN)
                            affect_mission_score(0, 5, -1);
                        else
                            affect_mission_score(0, 9, -1);
                    }
                    break;
                default:
                    if (aMissionShips[index].state == 0 &&
                        sighted(objective) != 0) {
                        flag_objective(objective, 2);
                        if (aMissionShips[index].type ==
                                OBJECT_TYPE_HORNET ||
                            aMissionShips[index].type ==
                                OBJECT_TYPE_DRAYMAN)
                            affect_mission_score(0, 5, -1);
                        else
                            affect_mission_score(0, 9, -1);
                    }
                    break;
                }
                break;
            case 4:
                if (aMissionShips[index].state == 3)
                    flag_objective(objective, 2);
                break;
            }
        }
    }
}

/* Function start: 0x42F1F0 */
int too_busy(short ship)
{
    return aeShipMissionType[ship] == MISSION_TYPE_ROUT;
}

/* Function start: 0x42F210 */
void reply(short ship, short accepted)
{
    if (accepted == 1) {
        send_message(ship, 0);
        return;
    }
    send_message(ship, 1);
}

/* Function start: 0x42F240 */
int disobey_formation(short ship)
{
    switch (aiPilotLevel[ship]) {
    case 10:
        return any_enemy_tail(0);
    case 11:
        return report_kilrathi_rout(0);
    }
    return 0;
}

/* Function start: 0x42F270 */
int bad_target(short ship, short target)
{
    if (target != ship &&
        aeShipSide[target] != aeShipSide[ship] &&
        too_busy(ship) == 0)
        return 0;
    return 1;
}

/* Function start: 0x42F2B0 */
short can_land(void)
{
    int *objectiveType;
    short result;
    short index;

    result = 0;
    if (any_enemy(0, 20000) == 0) {
        if (evaluate_damage(0) < 50 || nPlayerKillCount > 0 ||
            anShipFuel[0] < 1000)
            result = 1;
        index = 0;
        if (cMissionObjectiveCount > 0) {
            do {
                objectiveType =
                    &aMissionObjectives[index].type;
                if (*objectiveType != 1) {
                    if (achieved(index) == 0) {
                        if (visited(index) == 0 || *objectiveType == 2)
                            goto next_objective;
                    }
                    result = 1;
                }
next_objective:
                index++;
            } while (index < cMissionObjectiveCount);
        }
    }
    return result;
}

/* Function start: 0x42F350 */
short i_wanna_rout(short ship, int pilot)
{
    if (pilot <= 4)
        return 1;
    switch (pilot) {
    case 6:
        return any_enemy(ship, 5000) == 0;
    case 7:
    case 11:
        return 0;
    case 8:
        return aeShipMissionType[ship] ==
               MISSION_TYPE_CANNED_SEQUENCE;
    case 9:
        return (short)triumph(0);
    case 10:
        return any_enemy(0, 10000) == 0;
    }
    return 1;
}

/* Function start: 0x42F3F0 */
void request(short requester, short ship, short command)
{
    signed char *requesterTarget;
    short target;
    short object;
    short attacker;

    requesterTarget = &acShipTarget[requester];

    for (;;) {
        target = (short)*requesterTarget;
        switch (command) {
        case 1:
            allow_engage();
            if (bad_target(ship, target) != 0) {
                reply(ship, 0);
                return;
            }
            engage(ship, target, OBJECTIVE_ENGAGE_ENEMY);
            reply(ship, 1);
            return;
        case 2:
            allow_engage();
            attacker = -1;
            for (object = 0; object < 10; object++) {
                if (aeObjectClass[object] >=
                        OBJECT_CLASS_SHIP &&
                    aeSpecialManeuver[object] !=
                        SPECIAL_MANEUVER_UNKNOWN_9 &&
                    aeShipSide[ship] !=
                        aeShipSide[object] &&
                    acShipTarget[object] == requester) {
                    attacker = object;
                    break;
                }
            }
            if (attacker == -1) {
                command = 9;
                continue;
            }
            engage(ship, attacker, OBJECTIVE_ENGAGE_ENEMY);
            reply(ship, 1);
            return;
        case 3:
            if (i_wanna_rout(ship, aiPilotLevel[ship]) != 0 &&
                try2rout(ship) != 0) {
                bEngageAllowed = 0;
                reply(ship, 1);
                return;
            }
            reply(ship, 0);
            return;
        case 4:
        case 5:
        case 6:
            if (RandomBelow(100) < 70 ||
                ((signed char)acShipRating[ship] >= 9 &&
                 (signed char)acShipRating[ship] <= 12))
                send_message(ship, command - 2);
            if (acShipTarget[ship] != requester &&
                too_busy(ship) == 0) {
                engage(ship, requester, OBJECTIVE_ENGAGE_ENEMY);
                return;
            }
            break;
        case 7:
            allow_engage();
            if (aeShipObjective[ship] ==
                    OBJECTIVE_HOLD_FORMATION) {
                reset_objective(ship, OBJECTIVE_BREAK_FORMATION);
                reply(ship, 1);
                return;
            }
            reply(ship, 0);
            return;
        case 8:
            disallow_engage();
            if (disobey_formation(ship) != 0) {
                alter_objective(ship, OBJECTIVE_BREAK_FORMATION);
                reply(ship, 0);
                return;
            }
            nAutoEngageTimer = -150;
            reply(ship, 1);
            return;
        case 9:
            disallow_engage();
            if (disobey_formation(ship) != 0) {
                reply(ship, 0);
                return;
            }
            reset_objective(ship, OBJECTIVE_HOLD_FORMATION);
            nAutoEngageTimer = -150;
            reply(ship, 1);
            return;
        case 10:
        case 11:
            bRadioSilence = 0;
            reply(ship, 1);
            bRadioSilence = command == 10;
            return;
        case 12:
            cleanup_objectives();
            if (can_land() != 0) {
                bLandingAuthorized = 1;
                send_message(ship, 8);
                return;
            }
            send_message(ship, 9);
            return;
        }
        return;
    }
}

/* Function start: 0x42F730 */
unsigned short __stdcall ShouldSuspendCursorForRect(const ShortRect *bounds)
{
    (void)bounds;
    return 0;
}

/* Function start: 0x42F740 */
unsigned short __stdcall InitializeDIBScreenViewport(
    Viewport *viewport, unsigned short colour)
{
    short row;

    (void)colour;
    stScreenAllocation.state = 0;
    pAllocatedScreenViewport = viewport;
    stScreenAllocation.viewport = viewport;
    pScreenAllocation = &stScreenAllocation;
    viewport->right = 319;
    viewport->bottom = 199;
    viewport->left = 0;
    viewport->top = 0;
    if (nVideoMode != 0x13) {
        SystemDebugPrintf("== BAD alloc_screen == : type: '%d'\n",
                          (int)(short)nVideoMode);
        exit(1);
    }
    viewport->pixels = GetDIBPixelBuffer();
    viewport->rowOffsets = awScreenRowOffsets;
    for (row = 0; row < 202; row++)
        awScreenRowOffsets[row] = row * 320;
    return 1;
}

/* Function start: 0x42F7E0 */
/* Initialises a 320x200 viewport record (0x13F == 319, 199) then validates it. */
void __stdcall InitFullScreenViewport(int *record, short arg)
{
    Viewport *viewport = (Viewport *)record;

    pFullScreenRecord = record;
    viewport->left = 0;
    viewport->right = 319;
    viewport->bottom = 199;
    viewport->top = 0;
    AllocateViewport(viewport, arg, 0);
}

/* Function start: 0x42F810 */
unsigned int __stdcall GetPacketSize(const char *filename, short section)
{
    PacketSectionHandle handle;
    unsigned int size;

    size = (unsigned int)-1;
    if (OpenPacketSection(filename, section, &handle) != 0) {
        switch (handle.compression) {
        case 1:
            ReadPacketSectionData(&handle, &size, 4);
            if (nPacketError != 0)
                size = (unsigned int)-1;
            break;
        case 2:
            size = handle.dataSize;
            break;
        default:
            size = handle.dataSize;
            break;
        }
        CloseDataFileByHandle((unsigned short *)&handle);
    }
    return size;
}

/* Function start: 0x42F890 */
int GetFreeNearHeapBytes(void)
{
    NearHeapBlock *block;
    int descriptorAddress;
    int freeBytes;

    freeBytes = 0;
    descriptorAddress =
        nNearHeapBase + nNearHeapSize - 8;
    for (; descriptorAddress >= nNearHeapFirstDescriptor;
         descriptorAddress -= 8) {
        block = DosNearPtrToFar(descriptorAddress);
        if ((block->sizeAndFlags & 0x80000000) == 0)
            freeBytes += block->sizeAndFlags & 0xfffff;
    }
    return freeBytes;
}

/* Function start: 0x42F8E0 */
/* Nothing in the shipped executable calls this.  It is the largest-block
 * companion of GetFreeNearHeapBytes (the _memmax to its _memavl). */
int GetLargestFreeNearHeapBlock(void)
{
    NearHeapBlock *block;
    int descriptorAddress;
    int largestBytes;

    largestBytes = 0;
    descriptorAddress =
        nNearHeapBase + nNearHeapSize - 8;
    for (; descriptorAddress >= nNearHeapFirstDescriptor;
         descriptorAddress -= 8) {
        block = DosNearPtrToFar(descriptorAddress);
        if ((block->sizeAndFlags & 0x80000000) == 0 &&
            largestBytes < (int)(block->sizeAndFlags & 0xfffff))
            largestBytes = block->sizeAndFlags & 0xfffff;
    }
    return largestBytes;
}

/* Function start: 0x42F930 */
void FrameStartHook(int mode)
{
    TimerResetHook();
}

/* Function start: 0x42F940 */
unsigned short IsSoundHardwarePresent(short device)
{
    GetSoundHardwareFlag();
    return 1;
}

/* Function start: 0x42F950 */
void MessagePumpHook(int mode)
{
    TimerStopHook();
}

/* Function start: 0x42F960 */
void *PushMemoryStackFrame(void *memory, int offset)
{
    if (offset != 0) {
#if !defined(SDL_PORT) || defined(SDL_PORT_LEGACY_DEBUG_OUTPUT)
        printf("push %p by %d\n", memory, offset);
#endif
        if (nPacketHandleCount == 0x1000)
            exit_squadron("qq mem push overflow");
        aiPacketHandleOffsets[nPacketHandleCount] = offset;
        if (offset < 0)
            apPacketHandles[nPacketHandleCount++] = memory =
                (unsigned char *)memory - offset;
        else
            apPacketHandles[nPacketHandleCount++] = memory =
                (unsigned char *)memory + offset;
    }
    return memory;
}

/* Function start: 0x42F9E0 */
int IsPushedPacketHandle(void *handle)
{
    int index;

    index = 0;
    if (nPacketHandleCount > 0) {
        do {
            if (apPacketHandles[index] == handle) {
                if (aiPacketHandleOffsets[index] < 0)
                    return 1;
                return 0;
            }
            index++;
        } while (index < nPacketHandleCount);
    }
    return 0;
}

/* Function start: 0x42FA20 */
void *MapPacketHandleToBlock(void *handle)
{
    int count = nPacketHandleCount;
    int bytes = count * 4;
    int i;
    void **entry;
    int offset;

    for (;;) {
        i = 0;
        nPacketHandleCount = count;
        if (bytes <= 0)
            return handle;
        entry = apPacketHandles;
        while (*entry != handle) {
            entry = entry + 1;
            i = i + 1;
            nPacketHandleCount = count;
            if (i >= count)
                return handle;
        }
        offset = aiPacketHandleOffsets[i];
        if (offset < 0)
            handle = (unsigned char *)handle + offset;
        else
            handle = (unsigned char *)handle - offset;
#ifdef SDL_PORT
        apPacketHandles[i] =
            apPacketHandles[count - 1];
#else
        apPacketHandles[i] =
            *(void **)((unsigned char *)apPacketHandles + bytes - 4);
#endif
        bytes = bytes - 4;
        count = count - 1;
        aiPacketHandleOffsets[i] =
            *(int *)((unsigned char *)aiPacketHandleOffsets + bytes);
    }
}
