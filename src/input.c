/*
 *  Joystick transitions and menu input polling.
 *
 *  Address range 430710-430bbf (see docs/ORDER.md).
 */
#include "wc1.h"

short nInputDoubleClickInterval = 1; /* 0x0046af54 */
short nMenuPointerSpeed = 2; /* 0x0046af58 */

/* Function start: 0x430710 */
short __stdcall UpdateInputDeviceTransitions(short raw)
{
    unsigned int now;
    unsigned short buttons;
    unsigned short changed;
    unsigned short previous;
    short device;

    device = nActiveInputDevice;
    if (raw == 0)
        ReadCalibratedJoystick();
    else
        SampleJoystickDevice(&aInputDeviceSamples[device],
                             device, 0);

    asInputButton2DoubleClick[device] = 0;
    asInputButton1DoubleClick[device] = 0;
    asInputButton2Changed[device] = 0;
    asInputButton1Changed[device] = 0;
    buttons = (unsigned short)aInputDeviceSamples[device].buttons;
    previous = (unsigned short)asPreviousInputButtons[device];
    if (buttons != previous) {
        changed = buttons ^ previous;
        now = nTickCount60Hz;
        if ((changed & 1) != 0) {
            asInputButton1Changed[device] = 1;
            if ((buttons & 1) != 0) {
                if ((int)(now -
                    anInputButton1PressTime[device]) <=
                        nInputDoubleClickInterval *
                        nInputTickScale)
                    asInputButton1DoubleClick[device] = 1;
                anInputButton1PressTime[device] = now;
            }
        }
        if ((changed & 2) != 0) {
            asInputButton2Changed[device] = 1;
            if ((buttons & 2) != 0) {
                if ((int)(now -
                    anInputButton2PressTime[device]) <=
                        nInputDoubleClickInterval *
                        nInputTickScale)
                    asInputButton2DoubleClick[device] = 1;
                anInputButton2PressTime[device] = now;
            }
        }
        asPreviousInputButtons[device] = (short)buttons;
    }
    return 1;
}

/* Function start: 0x430830 */
/* Empty, and nothing in the shipped executable calls it, so its purpose
 * cannot be recovered; it sits among the joystick input routines. */
void JoystickInputHook(void)
{
}

/* Function start: 0x430840 */
void PollJoystickButtonEvents(void)
{
    short doubleClick;

    if (nActiveInputDevice == -1 ||
        bInputPollingGuard != 0)
        return;
    bInputPollingGuard++;
    UpdateInputDeviceTransitions(1);
    if (asInputButton1Changed
            [nActiveInputDevice] != 0) {
        doubleClick = 0;
        if (asInputButton1DoubleClick
                [nActiveInputDevice] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)aInputDeviceSamples
                [nActiveInputDevice].buttons & 1) + 1,
            0, doubleClick);
    }
    if (asInputButton2Changed
            [nActiveInputDevice] != 0) {
        doubleClick = 0;
        if (asInputButton2DoubleClick
                [nActiveInputDevice] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)(aInputDeviceSamples
                [nActiveInputDevice].buttons >> 1) & 1) + 1,
            1, doubleClick);
    }
    bInputPollingGuard--;
    stHostMouseState.secondaryButton =
        (unsigned char)aInputDeviceSamples
            [nActiveInputDevice].buttons;
    stHostMouseState.primaryButton =
        stHostMouseState.secondaryButton;
}

/* Function start: 0x430920 */
void PollMenuInputDevices(void)
{
    signed char changes;
    char doubleClick;
    short movementX;
    short movementY;
    int deviceIndex;

    changes = 0;
    if (nActiveInputDevice == -1)
        return;
    stHostMouseState.x =
        stMouseCursorState.x;
    stHostMouseState.y =
        stMouseCursorState.y;
    if (bInputPollingGuard != 0)
        return;
    bInputPollingGuard++;
    UpdateInputDeviceTransitions(0);

    deviceIndex = (int)nActiveInputDevice;
    if (asInputButton1Changed[deviceIndex] != 0) {
        stHostMouseState.x +=
            (short)aInputDeviceSamples
                [deviceIndex].x *
            nMenuPointerSpeed;
        stHostMouseState.y +=
            (short)aInputDeviceSamples
                [deviceIndex].y *
            nMenuPointerSpeed;
        if (asInputButton1DoubleClick[deviceIndex] != 0)
            changes = 3;
        QueueInputEventAtCursor(
            ((unsigned short)aInputDeviceSamples
                [deviceIndex].buttons & 1) + 1,
            0, (short)changes);
        changes = 1;
    }
    deviceIndex = (int)nActiveInputDevice;
    if (asInputButton2Changed[deviceIndex] != 0) {
        stHostMouseState.x +=
            (short)aInputDeviceSamples
                [deviceIndex].x *
            nMenuPointerSpeed;
        stHostMouseState.y +=
            (short)aInputDeviceSamples
                [deviceIndex].y *
            nMenuPointerSpeed;
        doubleClick = 0;
        if (asInputButton1DoubleClick[deviceIndex] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)(aInputDeviceSamples
                [deviceIndex].buttons >> 1) & 1) + 1,
            1, (short)doubleClick);
        changes++;
    }
    if (changes == 0) {
        movementX = (short)aInputDeviceSamples
            [nActiveInputDevice].x;
        movementY = (short)aInputDeviceSamples
            [nActiveInputDevice].y;
        stHostMouseState.x +=
            nMenuPointerSpeed * movementX;
        stHostMouseState.y +=
            nMenuPointerSpeed * movementY;
        changes = (unsigned char)movementY | (unsigned char)movementX;
        if (changes != 0) {
            FlushInputEvents();
            stHostMouseState.x = stHostMouseState.x > 0 ?
                stHostMouseState.x : 0;
            stHostMouseState.x = stHostMouseState.x < 319 ?
                stHostMouseState.x : 319;
            stHostMouseState.y = stHostMouseState.y > 0 ?
                stHostMouseState.y : 0;
            stHostMouseState.y = stHostMouseState.y < 199 ?
                stHostMouseState.y : 199;
            SetMousePosition(stHostMouseState.x,
                             stHostMouseState.y);
        }
    }

    stHostMouseState.secondaryButton =
        (unsigned char)aInputDeviceSamples
            [nActiveInputDevice].buttons;
    stHostMouseState.primaryButton =
        stHostMouseState.secondaryButton;
    stHostMouseState.x = stHostMouseState.x > 0 ?
        stHostMouseState.x : 0;
    stHostMouseState.x = stHostMouseState.x < 319 ?
        stHostMouseState.x : 319;
    stHostMouseState.y = stHostMouseState.y > 0 ?
        stHostMouseState.y : 0;
    stHostMouseState.y = stHostMouseState.y < 199 ?
        stHostMouseState.y : 199;
    if (changes != 0) {
        LeaveAllocationScope();
        stMouseCursorState.primaryButton =
            stHostMouseState.primaryButton;
        stMouseCursorState.x =
            stHostMouseState.x;
        stMouseCursorState.y =
            stHostMouseState.y;
        stMouseCursorState.secondaryButton =
            stHostMouseState.secondaryButton;
        stMouseCursorState.flags =
            stHostMouseState.flags;
        EnterAllocationScope();
    }
    bInputPollingGuard--;
}
