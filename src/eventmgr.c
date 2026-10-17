/*
 *  Event manager, mouse pointer, and frame timer.
 *
 *  The provisional address range is recorded in docs/ORDER.md.
 *  The FM Towns executable preserves the original source name
 *  "source\\eventmgr.c" for this unit.
 */
#include "wc1.h"

/* The SDL renderer also reads the saved cursor background. */
#ifndef SDL_PORT
static
#endif
unsigned char abCursorSaveArea[0x1000]; /* 0x004865a8 */

/* Function start: 0x4355F0 */
void TranslatePolledInputEvent(unsigned short type, unsigned int value)
{
    InputDeviceSample *samples;
    unsigned int state;

    switch (type) {
    case 2:
        state = (value >> 2) |
                (bHostSecondaryMouseButton << 1) |
                bHostPrimaryMouseButton;
        QueueInputEvent(type, nHostMouseMessageX, nHostMouseMessageY, 0,
                        bHostPrimaryMouseButton, bHostSecondaryMouseButton,
                        state);
        return;
    case 6:
        samples = aInputDeviceSamples;
        QueueInputEvent(type, samples[nActiveInputDevice].x,
                        samples[nActiveInputDevice].y, 0, 0, 0, 0);
        return;
    case 13:
        QueueInputEvent(type, nHostMouseMessageX, nHostMouseMessageY,
                        0, 0, 0, 0);
        return;
    }
}

/* Function start: 0x4356A0 */
void QueueInputEventAtCursor(unsigned short type, short primaryButton,
                             short secondaryButton)
{
    int eventType = type;

    QueueInputEvent(eventType, stMouseCursorState.x, stMouseCursorState.y, 0,
                    primaryButton, secondaryButton, 0);
}

/* Function start: 0x4356E0 */
InputEvent *AllocateInputEvent(void)
{
    int *used;
    int index;

    if (bInputEventPoolInitialized != 0) {
        index = 0;
        used = aiInputEventSlotUsed;
        do {
            if (*used == 0) {
                aiInputEventSlotUsed[index] = 1;
                return &aInputEventPool[index];
            }
            used++;
            index++;
        } while (used < &aiInputEventSlotUsed[0x100]);
        return 0;
    }
    memset(aInputEventPool, 0,
           sizeof(aInputEventPool));
    memset(aiInputEventSlotUsed, 0,
           sizeof(aiInputEventSlotUsed));
    bInputEventPoolInitialized = 1;
    aiInputEventSlotUsed[0] = 1;
    return &aInputEventPool[0];
}

/* Function start: 0x435760 */
void ReleaseInputEvent(InputEvent *event)
{
    int slot;

    for (slot = 0; slot < 0x100; slot++) {
        if (&aInputEventPool[slot] == event)
            aiInputEventSlotUsed[slot] = 0;
    }
}

/* Function start: 0x435790 */
void QueueInputEvent(int type, int x, int y, int value,
                     int primaryButton, int secondaryButton,
                     unsigned int timestamp)
{
    unsigned int modifiers;
    InputEvent *event;

    modifiers = 0;
    if (GetShiftKeyState() != 0)
        modifiers = 0xe0;
    if (GetControlKeyState() != 0)
        modifiers |= 0x2000;
    if (GetKeyboardModifiers() != 0)
        modifiers |= 0x700;
    if (primaryButton != 0)
        modifiers |= 2;
    if (secondaryButton != 0)
        modifiers |= 4;

    if (pInputEventHead == 0) {
        pInputEventHead = AllocateInputEvent();
        if (pInputEventHead == 0) {
            ReleaseInputEventQueue();
            return;
        }
        pInputEventTail = pInputEventHead;
        pInputEventHead->next = 0;
        pInputEventTail->previous = 0;
    } else {
        event = AllocateInputEvent();
        pInputEventTail->next = event;
        if (pInputEventTail->next == 0) {
            ReleaseInputEventQueue();
            return;
        }
        pInputEventTail->next->previous =
            pInputEventTail;
        pInputEventTail = pInputEventTail->next;
        pInputEventTail->next = 0;
    }
    pInputEventTail->type = type;
    pInputEventTail->modifiers = modifiers;
    pInputEventTail->x = x;
    pInputEventTail->y = y;
    pInputEventTail->value = value;
    pInputEventTail->primaryButton = (short)primaryButton;
    pInputEventTail->secondaryButton = (short)secondaryButton;
}

/* Function start: 0x4358B0 */
void ReleaseInputEventQueue(void)
{
    InputEvent *event = pInputEventHead;

    while (event != 0) {
        InputEvent *next = event->next;

        ReleaseInputEvent(event);
        event = next;
    }
    pInputEventTail = 0;
    pInputEventHead = 0;
}

/* Function start: 0x4358E0 */
void RetainInputEventsOfType(int type)
{
    InputEvent *event = pInputEventHead;

    while (event != 0) {
        InputEvent *next = event->next;

        if (event->type != type) {
            if (event->previous != 0)
                event->previous->next = next;
            else
                pInputEventHead = next;
            if (event->next != 0)
                event->next->previous = event->previous;
            else
                pInputEventTail = event->previous;
            ReleaseInputEvent(event);
        }
        event = next;
    }
}

/* Function start: 0x435940 */
void RemoveInputEvent(InputEvent *event)
{
    if (event->previous != 0) {
        if (event->next != 0) {
            event->previous->next = event->next;
            event->next->previous = event->previous;
            ReleaseInputEvent(event);
            return;
        }
        event->previous->next = 0;
        pInputEventTail = event->previous;
        ReleaseInputEvent(event);
        return;
    }
    if (event->next != 0) {
        pInputEventHead = event->next;
        event->next->previous = 0;
        ReleaseInputEvent(event);
        return;
    }
    pInputEventTail = 0;
    pInputEventHead = 0;
    ReleaseInputEvent(event);
}

/* Function start: 0x4359C0 */
short __stdcall GetNextInputEvent(InputEventState *state)
{
    int eventX;
    int eventY;
    short limit;
    int type;

    type = 0;
    if (pInputEventHead != 0) {
        eventX = pInputEventHead->x;
        eventY = pInputEventHead->y;
        limit = stMouseCursorState.viewport->left;
        if (limit > eventX)
            pInputEventHead->x = limit;
        else {
            limit = stMouseCursorState.viewport->right;
            if (limit < eventX)
                pInputEventHead->x = limit;
        }
        limit = stMouseCursorState.viewport->top;
        if (limit > eventY)
            pInputEventHead->y = limit;
        else {
            limit = stMouseCursorState.viewport->bottom;
            if (limit < eventY)
                pInputEventHead->y = limit;
        }

        state->modifiers =
            (short)pInputEventHead->modifiers;
        switch (pInputEventHead->type) {
        case 1:
            stMouseCursorState.x = pInputEventHead->x;
            stMouseCursorState.y = pInputEventHead->y;
            stMouseCursorState.primaryButton = 0;
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 1;
            break;
        case 2:
            stMouseCursorState.x = pInputEventHead->x;
            stMouseCursorState.y = pInputEventHead->y;
            stMouseCursorState.primaryButton =
                (unsigned char)pInputEventHead->primaryButton;
            stMouseCursorState.secondaryButton =
                (unsigned char)pInputEventHead->secondaryButton;
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 2;
            state->value =
                (int)pInputEventHead->secondaryButton * 2 |
                (int)pInputEventHead->primaryButton;
            break;
        case 3:
            state->value = pInputEventHead->value;
            state->x = stMouseCursorState.x;
            state->y = stMouseCursorState.y;
            type = 3;
            break;
        case 4:
            state->x = pInputEventHead->value;
            state->x = stMouseCursorState.x;
            state->y = stMouseCursorState.y;
            type = 4;
            break;
        case 5:
            state->x = pInputEventHead->value;
            type = 5;
            break;
        case 6:
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 6;
            break;
        case 7:
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 7;
            break;
        case 8:
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 8;
            break;
        case 9:
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 9;
            break;
        case 10:
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 10;
            break;
        case 13:
            stMouseCursorState.x = pInputEventHead->x;
            stMouseCursorState.y = pInputEventHead->y;
            state->x = pInputEventHead->x;
            state->y = pInputEventHead->y;
            type = 13;
            break;
        }
        RemoveInputEvent(pInputEventHead);
    }
    return type;
}

/* Function start: 0x435CC0 */
short __stdcall PollInputEvent(InputEventState *event, short filter)
{
    PumpWindowMessages();
    return GetNextInputEvent(event);
}

/* Function start: 0x435CE0 */
short __stdcall PeekInputEvent(InputEventState *state, short type)
{
    InputEvent *event;
    int eventType;
    int modifiers;

    event = pInputEventHead;
    modifiers = 0;
    while (event != 0 && event->type != type)
        event = event->next;
    if (event != 0) {
        state->type = event->type;
        state->value = event->modifiers;
        state->timestamp = event->timestamp;
        eventType = (int)event->type;
        if (eventType == 2 || eventType == 1)
            modifiers = 1;
        modifiers |= ((unsigned short)event->primaryButton < 1 ? 0 : -1) & 2;
        modifiers |= ((unsigned short)event->secondaryButton < 1 ? 0 : -1) & 4;
        state->modifiers = (short)modifiers;
        state->x = event->x;
        state->y = event->y;
        return 1;
    }
    return 0;
}

/* Function start: 0x435D80 */
short __stdcall IsInputEventQueued(short type)
{
    InputEvent *event = pInputEventHead;

    while (event != 0 && event->type != type)
        event = event->next;
    return event != 0;
}

/* Function start: 0x435DB0 */
void FlushInputEvents(void)
{
    ReleaseInputEventQueue();
}

/* Function start: 0x435DC0 */
short __stdcall ResetAllocationDepth(int x, int y)
{
    (void)x;
    (void)y;
    nMouseCursorShowCount = 0;
    return 1;
}

/* Function start: 0x435DE0 */
void CheckCursor(void)
{
}

/* Function start: 0x435DF0 */
/* Empty in the Win32 build; the hidden kill-board code at 0x43F8B0 calls it. */
void RefreshMouseCursorHook(void)
{
}

/* Function start: 0x435E20 */
void CaptureMouseCursorBackground(void)
{
    int x;
    int y;

    if (nMouseCursorShowCount == 0 ||
        stMouseCursorState.viewport == 0 ||
        stMouseCursorState.shape == 0)
        return;

    CaptureSpriteBackground(stMouseCursorState.viewport,
                            abCursorSaveArea,
                            stMouseCursorState.x,
                            stMouseCursorState.y,
                            stMouseCursorState.shape,
                            stMouseCursorState.frame);
    x = stMouseCursorState.x;
    y = stMouseCursorState.y;
    if (nMouseCursorDamageLeft > x - 16)
        nMouseCursorDamageLeft = x - 16;
    nMouseCursorDrawnX = x;
    if (nMouseCursorDamageRight < x + 16)
        nMouseCursorDamageRight = x + 16;
    if (nMouseCursorDamageTop > y - 16)
        nMouseCursorDamageTop = y - 16;
    nMouseCursorDrawnY = y;
    if (nMouseCursorDamageBottom < y + 16)
        nMouseCursorDamageBottom = y + 16;
    bMouseCursorDamagePending = 1;
    bMouseCursorDrawn = 1;
}

/* Function start: 0x435EF0 */
void DrawMouseCursor(void)
{
    int x;
    int y;

    if (nMouseCursorShowCount == 0 ||
        stMouseCursorState.viewport == 0 ||
        stMouseCursorState.shape == 0)
        return;

    DrawSpriteDefault(stMouseCursorState.viewport,
                      stMouseCursorState.x,
                      stMouseCursorState.y,
                      stMouseCursorState.shape,
                      stMouseCursorState.frame);
    x = stMouseCursorState.x;
    if (nMouseCursorDamageLeft > x - 16)
        nMouseCursorDamageLeft = x - 16;
    if (nMouseCursorDamageRight < x + 16)
        nMouseCursorDamageRight = x + 16;
    y = stMouseCursorState.y;
    if (nMouseCursorDamageTop > y - 16)
        nMouseCursorDamageTop = y - 16;
    if (nMouseCursorDamageBottom < y + 16)
        nMouseCursorDamageBottom = y + 16;
    bMouseCursorDamagePending = 1;
}

/* Function start: 0x435FA0 */
void RestoreMouseCursorBackground(void)
{
    int x;
    int y;

    if (nMouseCursorShowCount == 0 ||
        stMouseCursorState.viewport == 0 ||
        stMouseCursorState.shape == 0 ||
        bMouseCursorDrawn == 0)
        return;

    RestoreSpriteBackground(stMouseCursorState.viewport,
                            abCursorSaveArea,
                            (short)nMouseCursorDrawnX,
                            (short)nMouseCursorDrawnY,
                            stMouseCursorState.shape,
                            stMouseCursorState.frame);
    x = nMouseCursorDrawnX;
    if (nMouseCursorDamageLeft > x - 16)
        nMouseCursorDamageLeft = x - 16;
    if (nMouseCursorDamageRight < x + 16)
        nMouseCursorDamageRight = x + 16;
    y = nMouseCursorDrawnY;
    if (nMouseCursorDamageTop > y - 16)
        nMouseCursorDamageTop = y - 16;
    if (nMouseCursorDamageBottom < y + 16)
        nMouseCursorDamageBottom = y + 16;
    bMouseCursorDrawn = 0;
}

/* Function start: 0x436060 */
void RefreshMouseCursorDisplay(void)
{
    nMouseCursorDamageLeft = 319;
    nMouseCursorDamageTop = 199;
    nMouseCursorDamageRight = 0;
    nMouseCursorDamageBottom = 0;
    bMouseCursorDamagePending = 0;
    bMouseCursorDrawn = 0;
    CaptureMouseCursorBackground();
    DrawMouseCursor();
    DIBupdate(nMouseCursorDamageLeft,
              nMouseCursorDamageTop,
              nMouseCursorDamageRight,
              nMouseCursorDamageBottom);
    RestoreMouseCursorBackground();
}

/* Function start: 0x4360D0 */
void EnterAllocationScope(void)
{
    nMouseCursorShowCount = nMouseCursorShowCount + 1;
}

/* Function start: 0x4360E0 */
void LeaveAllocationScope(void)
{
    nMouseCursorShowCount = nMouseCursorShowCount - 1;
}

/* Function start: 0x4360F0 */
void __stdcall SetMouseCursorShape(unsigned char *shape, short frame)
{
    stMouseCursorState.shapeChanged = 1;
    if (stMouseCursorState.viewport != 0 &&
        nMouseCursorShowCount > 0 &&
        pDrawnMouseCursorShape != 0) {
        RestoreSpriteBackground(stMouseCursorState.viewport,
                                abCursorSaveArea,
                                (short)nMouseCursorSavedX,
                                (short)nMouseCursorSavedY,
                                pDrawnMouseCursorShape,
                                (short)stMouseCursorState.frame);
        pDrawnMouseCursorShape = 0;
    }
    stMouseCursorState.shape = shape;
    stMouseCursorState.frame = frame;
}

/* Function start: 0x436160 */
void __stdcall SetMouseHomePosition(short x, short y)
{
    stMouseCursorState.x = x;
    stMouseCursorState.y = y;
    SetMousePositionDuplicate(x, y);
}

/* Function start: 0x436190 */
void __stdcall ApplyPackedMousePosition(ShortPoint point)
{
    stMouseCursorState.x = point.x;
    stMouseCursorState.y = point.y;
    SetMousePositionDuplicate(point.x, point.y);
}

/* Function start: 0x4361C0 */
void __stdcall GetMouseCursorPosition(ShortPoint *point)
{
    point->x = stMouseCursorState.x;
    point->y = stMouseCursorState.y;
}

/* Function start: 0x4361E0 */
void SetFrameTimerPeriod(short period)
{
    SetMultimediaTimerCallback((int)period);
}

/* Function start: 0x4361F0 */
void __stdcall SetFrameTimerAndWait(short period)
{
    SetFrameTimerPeriod(period);
    WaitForFrameTick();
}

/* Function start: 0x436210 */
void __stdcall SetFrameTimerPeriodDirect(short p)
{
    SetMultimediaTimerCallback((int)p);
}

/* Function start: 0x436230 */
void WaitForFrameTick(void)
{
    while (bFrameTickPending != 0) ;
}

/* Function start: 0x436240 */
int IsFrameTickElapsed(void)
{
    return bFrameTickPending == 0;
}

/* Function start: 0x436260 */
unsigned short GetSoundHardwareFlag(void)
{
    return 1;
}

/* Function start: 0x436270 */
void *AllocateUntaggedMemory(unsigned int size)
{
    return AllocateTaggedMemory(size, 0);
}

/* Function start: 0x436290 */
void ReleaseUntaggedMemory(void *memory)
{
    ReleasePacketHandle(memory);
}

/* Function start: 0x4362A0 */
void TimerResetHook(void)
{
}

/* Function start: 0x4362B0 */
unsigned int GetPacketDecompressionResult(void *packet)
{
    (void)packet;
    return 0;
}

/* Function start: 0x4362C0 */
unsigned short IdentityWord(unsigned short v)
{
    return v;
}

/* Function start: 0x4362D0 */
void TimerStopHook(void)
{
}

/* Function start: 0x4362E0 */
unsigned int GetFixedOneMillion(void)
{
    return 0x3e8000;
}

/* Function start: 0x4362F0 */
unsigned int GetFixedOneMillionAlt(void)
{
    return 0x3e8000;
}

/* Function start: 0x4363A0 */
void ClearInputKeyStatePreservingModifiers(void)
{
    unsigned int control = abInputKeyState[0x1d];
    unsigned int alt = abInputKeyState[0x38];

    memset(abInputKeyState, 0,
           sizeof(abInputKeyState));
    abInputKeyState[0x1d] = control;
    abInputKeyState[0x38] = alt;
    ClearDebugPauseFlags();
}

/* Function start: 0x4363E0 */
void ClearInputKeyState(void)
{
    memset(abInputKeyState, 0,
           sizeof(abInputKeyState));
    ClearDebugPauseFlags();
}

/* Function start: 0x436420 */
void SetInputKeyState(int scanCode, unsigned char pressed)
{
    if (scanCode >= 0 && scanCode < 0x80) {
        abInputKeyState[scanCode] = pressed;
        return;
    }
    SystemDebugPrintf("keyboard almost messed up\n");
    ClearDebugPauseFlags();
    PumpMessagesDuringWait();
    exit(1);
}

/* Function start: 0x436460 */
void sort_object_depth(void)
{
    unsigned int distance;
    int previous;
    int best;
    int bestObject;
    int obj;
    int screenOffset;
    int *placed;
    int *sortedEntry;
    int sorted;

    previous = -999999999;
    bestObject = -1;
    memset(anObjectDepthPlaced, 0,
           sizeof(anObjectDepthPlaced));
    obj = 0;
    for (; obj < SPACE_OBJECT_COUNT; obj++) {
        distance = (unsigned short)asObjectDistance[obj];
        if (previous < (int)distance) {
            previous = (int)distance;
            bestObject = obj;
        }
    }
    sorted = 0;
    sortedEntry = anSortedObject;
    for (; sorted < SPACE_OBJECT_COUNT; sorted++, sortedEntry++) {
        best = -1;
        *sortedEntry = bestObject;
        if (bestObject == -1)
            return;
        screenOffset = 0;
        obj = 0;
        anObjectDepthPlaced[bestObject] = 1;
        bestObject = -1;
        placed = anObjectDepthPlaced;
        for (; placed < anObjectDepthPlaced +
                           SPACE_OBJECT_COUNT;
             screenOffset += sizeof(short), placed++, obj++) {
            if (*placed == 0 &&
                *(short *)((unsigned char *)asObjectScreenX +
                           screenOffset) != (short)0x8001) {
                distance = *(unsigned short *)(
                    (unsigned char *)asObjectDistance +
                    screenOffset);
                if (best < (int)distance && previous >= (int)distance) {
                    bestObject = obj;
                    best = (int)distance;
                }
            }
        }
    }
}

/* Function start: 0x436520 */
void draw_sorted_objects_to_buffer(void)
{
    int *sortedEntry;
    int obj;
    int objectClass;
    unsigned char *shape;
    short screenX;
    short screenY;
    int specialObject;
#ifdef SDL_PORT
    float enhancedScreenX;
    float enhancedScreenY;
    short projectedScreenX;
    short projectedScreenY;
#endif

    sortedEntry = anSortedObject;
    do {
        obj = *sortedEntry;
        if (obj < 0)
            return;
        if ((int)aeObjectType[obj] < 0)
            return;
        objectClass = aeObjectClass[obj];
#ifdef SDL_PORT
        enhancedScreenX = (float)(short)(
            asObjectScreenX[obj] + nViewCenterX);
        enhancedScreenY = (float)(short)(
            asObjectScreenY[obj] + nViewCenterY);
        if (objectClass != OBJECT_CLASS_NULL &&
            objectClass != OBJECT_CLASS_FIXED_OBJECT &&
            obj != nNavPointerObject &&
            aObjectViewPosition[obj].z != 0) {
            projectedScreenX = (short)(DivideFixed(
                MultiplyFixed(
                    (short)(nScreenWidth & ~1) << 7,
                    aObjectViewPosition[obj].x),
                aObjectViewPosition[obj].z) >> 8);
            projectedScreenY = (short)(DivideFixed(
                MultiplyFixed(
                    (short)(nScreenWidth & ~1) << 7,
                    aObjectViewPosition[obj].y),
                aObjectViewPosition[obj].z) >> 8);
            if (projectedScreenX == asObjectScreenX[obj] &&
                projectedScreenY == asObjectScreenY[obj]) {
                enhancedScreenX =
                    (float)nViewCenterX +
                    (float)(((double)(nScreenWidth & ~1) * 0.5 *
                             aObjectViewPosition[obj].x) /
                            aObjectViewPosition[obj].z);
                enhancedScreenY =
                    (float)nViewCenterY +
                    (float)(((double)(nScreenWidth & ~1) * 0.5 *
                             aObjectViewPosition[obj].y) /
                            aObjectViewPosition[obj].z);
            }
        } else if (objectClass == OBJECT_CLASS_FIXED_OBJECT &&
                   aeObjectType[obj] == OBJECT_TYPE_THRUSTERS) {
            SdlGetThrusterScreenPosition(
                (short)obj, &enhancedScreenX, &enhancedScreenY);
        }
#endif
        if (objectClass != OBJECT_CLASS_NULL) {
            switch (objectClass) {
            default:
                screenY = asObjectScreenY[obj];
                screenX = (short)(asObjectScreenX[obj] +
                                  nViewCenterX);
                shape = apObjectShape[obj];
                asObjectDrawX[obj] = screenX;
                screenY = (short)(screenY + nViewCenterY);
                asObjectDrawY[obj] = screenY;
                if (shape != 0) {
#ifdef SDL_PORT
                    if (!SdlRecordSpaceSprite(
                            &stSpaceBuffer, enhancedScreenX, enhancedScreenY,
                            shape,
                            asObjectViewFrame[obj],
                            asObjectScreenAngle[obj],
                            asObjectScreenScale[obj],
                            asObjectFlip[obj]))
#endif
                    DrawSpriteScaled(
                        &stSpaceBuffer, screenX, screenY, shape,
                        asObjectViewFrame[obj],
                        asObjectScreenAngle[obj],
                        asObjectScreenScale[obj],
                        asObjectFlip[obj]);
                }
                break;
            case OBJECT_CLASS_STAR:
#ifdef SDL_PORT
                /* WCDX fix: planets use the per-object scaled path above. */
#else
            case OBJECT_CLASS_PLANET:
#endif
            case OBJECT_CLASS_DUST:
                specialObject = (int)nNavPointerObject;
                screenY = asObjectScreenY[obj];
                screenX = (short)(asObjectScreenX[obj] +
                                  nViewCenterX);
                asObjectDrawX[obj] = screenX;
                screenY = (short)(screenY + nViewCenterY);
                asObjectDrawY[obj] = screenY;
                if (specialObject == obj)
                    shape = apObjectShape[obj];
                else
                    shape = pConstellationShape;
#ifdef SDL_PORT
                if (!SdlRecordSpaceSprite(
                        &stSpaceBuffer, enhancedScreenX,
                        enhancedScreenY, shape,
                        asObjectViewFrame[obj], 0, 0x100, 0))
#endif
                DrawSpriteDefault(&stSpaceBuffer, screenX, screenY, shape,
                                  asObjectViewFrame[obj]);
                break;
            }
        }
        sortedEntry++;
    } while (sortedEntry < anSortedObject +
                           SPACE_OBJECT_COUNT);
}

/* Function start: 0x436650 */
void intro_drawbackgroundships(void)
{
    unsigned char *shape;
    int objectClass;
    int obj;

    for (obj = 0; obj < SPACE_OBJECT_COUNT; obj++) {
        if (aeObjectType[obj] < 0)
            return;
        objectClass = aeObjectClass[obj];
        if (objectClass != OBJECT_CLASS_NULL) {
            switch (objectClass) {
            default:
                shape = apObjectShape[obj];
                if (shape != 0) {
                    DrawSolidColourSpriteScaled(
                        &stSpaceBuffer,
                        asObjectDrawX[obj],
                        asObjectDrawY[obj],
                        shape,
                        asObjectViewFrame[obj],
                        asObjectScreenAngle[obj],
                        asObjectScreenScale[obj],
                        asObjectFlip[obj],
                        cPrimaryViewBufferColour);
                }
                break;
            case OBJECT_CLASS_STAR:
#ifdef SDL_PORT
                /* Erase planets with the same scaled geometry used to draw. */
#else
            case OBJECT_CLASS_PLANET:
#endif
            case OBJECT_CLASS_DUST:
                if (obj == nNavPointerObject) {
                    DrawSolidColourSprite(
                        &stSpaceBuffer,
                        asObjectDrawX[obj],
                        asObjectDrawY[obj],
                        apObjectShape[obj],
                        asObjectViewFrame[obj],
                        cPrimaryViewBufferColour);
                } else {
                    DrawSolidColourSprite(
                        &stSpaceBuffer,
                        asObjectDrawX[obj],
                        asObjectDrawY[obj],
                        pConstellationShape,
                        asObjectViewFrame[obj],
                        cPrimaryViewBufferColour);
                }
                break;
            }
        }
    }
}

/* Function start: 0x436740 */
void set_up_screen_viewport(signed char mode)
{
    int modeIndex;
    short viewportWidth;
    const ScreenViewportGeometry *viewportGeometry;

    cScreenViewportMode = mode;
    modeIndex = (int)mode;
    switch (modeIndex) {
    case 4:
    case 5:
        pScreenViewportGeometry =
            &aScreenViewportGeometry[modeIndex];
        break;
    default:
        pScreenViewportGeometry =
            (const ScreenViewportGeometry *)(
                (const unsigned char *)pScreenViewportPacket +
                pScreenViewportPacket
                    ->geometryOffsets[modeIndex]);
        break;
    }

    if (bCockpitlessView != 0 && bCockpitlessView != -2) {
        viewportWidth = pScreenViewportGeometry->width;
        viewportGeometry = pScreenViewportGeometry;
        nScreenWidth = viewportWidth;
        nViewCenterX = (short)(viewportWidth / 2);
        nScreenHeight = viewportGeometry->height;
        nViewCenterY = (short)(nScreenHeight / 2);
        nViewportOriginX = viewportGeometry->originX;
        nViewportOriginY = viewportGeometry->originY;
        switch ((int)cCockpitView) {
        case 0:
            nViewportOriginY += 10;
            nViewCenterY += 10;
            break;
        case 1:
            nViewportOriginY += 25;
            nViewCenterY += 25;
            break;
        case 2:
            nViewportOriginY += 50;
            nViewCenterY += 50;
            break;
        }
        nScreenWidth = 320;
        nScreenHeight = 200;
        return;
    }

    viewportWidth = pScreenViewportGeometry->width;
    viewportGeometry = pScreenViewportGeometry;
    nScreenWidth = viewportWidth;
    nViewCenterX = (short)(viewportWidth / 2);
    nScreenHeight = viewportGeometry->height;
    nViewCenterY = (short)(nScreenHeight / 2);
    nViewportOriginX = viewportGeometry->originX;
    nViewportOriginY = viewportGeometry->originY;
}

/* Function start: 0x4368C0 */
void MouseIdleHook(void)
{
}

/* Function start: 0x4368E0 */
unsigned short GetNavRangeSentinel(void)
{
    return 0x8000;
}

/* Function start: 0x4368F0 */
unsigned short GetOriginalFreeMemory(void)
{
    return 0x8000;
}

/* Function start: 0x436900 */
void StartupHook(unsigned int (*callback)(unsigned int, short))
{
    (void)callback;
}

/* Function start: 0x436910 */
unsigned int JoystickEdgeHook(int button)
{
    (void)button;
}

/* Function start: 0x436940 */
void *AllocateSmallMemoryBlock(short size)
{
    return malloc(size);
}

/* Function start: 0x436950 */
void FreeIfNotNull(void *p)
{
    if (p != 0)
        free(p);
}

/* Function start: 0x436970 */
unsigned int GetStartupErrorCode(int vector)
{
    (void)vector;
    return 0;
}

/* Function start: 0x436980 */
void ShutdownHook(int vector, void *handler)
{
    (void)vector;
    (void)handler;
}

/* Function start: 0x4369A0 */
unsigned short __stdcall SelectDiskDriveHook(short drive)
{
    return 0;
}

/* Function start: 0x4369B0 */
short GetCurrentDiskDriveHook(void)
{
    return 0;
}

/* Function start: 0x4369C0 */
unsigned short __stdcall GetShutdownErrorCode(unsigned char *driveState)
{
    (void)driveState;
    return 0;
}

/* Function start: 0x4369D0 */
void FinishPacketDecompressionHook(void)
{
}

/* Function start: 0x4369E0 */
void ExitCleanupHook(void)
{
}

/* Function start: 0x436A00 */
unsigned int IsVectorWithinRange(FixedVector *vector, short range)
{
    int magnitude = Vector_magnitude(vector);
    int fixedRange = abs((int)range << 8);

    if (fixedRange >= magnitude)
        return 1;
    return 0;
}

/* Function start: 0x436A30 */
unsigned int shrink_vector(FixedVector *vector)
{
    unsigned int shrinking;

    do {
        shrinking = shrink(&vector->x);
        shrinking |= shrink(&vector->y);
        shrinking |= shrink(&vector->z);
    } while (shrinking != 0);
    return 0;
}

/* Function start: 0x436A70 */
unsigned int shrink(int *component)
{
    int value;
    unsigned int fraction;
    unsigned int integerPart;
    short signedIntegerPart;

    value = *component / 2;
    *component = value;
    fraction = value;
    integerPart = (unsigned int)value >> 16;
    signedIntegerPart = (short)integerPart;
    if (signedIntegerPart == 0)
        return (unsigned short)fraction > 0x0f00;
    if (signedIntegerPart == -1)
        return (unsigned short)fraction < 0xf100;
    if (signedIntegerPart != 0)
        integerPart = 1;
    return integerPart;
}

/* Function start: 0x436C70 */
void __stdcall FillGraphicSuffix(char *path, short number, short digits)
{
    char *dot;
    char *suffix;

    suffix = DosStrchr(path, '.');
    dot = suffix;
    suffix++;
    if (dot == 0)
        suffix = path;
    suffix[digits] = '\0';
    ConvertChar_Int(suffix, number, digits);
}

/* Function start: 0x436CB0 */
void __stdcall ConvertChar_Int(char *text, short number, short digits)
{
    while (digits--) {
        text[digits] = (char)(number % 10) + '0';
        number /= 10;
    }
}
