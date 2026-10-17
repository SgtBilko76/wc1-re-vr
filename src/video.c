/*
 *  Video resource hooks, palette allocation, and joystick calibration.
 *
 *  Address range 4318f0-431eff (see docs/ORDER.md).
 *  Internal boundaries remain provisional.
 */
#include "wc1.h"

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

/* Function start: 0x431A10 */
void LoadJoystickCalibrationFile(short horizontalRange,
                                 short verticalRange,
                                 short horizontalDeadZone,
                                 short verticalDeadZone)
{
    unsigned short storedCentreX;
    unsigned short storedCentreY;
    unsigned short minimumX;
    unsigned short maximumX;
    unsigned short maximumY;
    unsigned short minimumY;
    short activeDevice;
    InputDeviceSample samples[2];
    int file;
    int failed;
    int centreX;
    int centreY;
    int rangeX;
    int rangeY;

    activeDevice = -1;
    failed = 1;
    if (horizontalRange == 0)
        horizontalRange += (short)failed;
    if (verticalRange == 0)
        verticalRange++;
    nJoystickFailureValue = -1;
    SampleBothJoysticks(samples, -1);
    if (samples[0].x != -1 && samples[0].y != -1)
        activeDevice = 0;
    else if (samples[1].x != -1 && samples[1].y != -1)
        activeDevice = 1;

    if (activeDevice != -1) {
        file = _open("j.cal", 0x8000);
        if (file != -1) {
            failed = _read(file, &activeDevice, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &minimumX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &minimumY, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &maximumX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &maximumY, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &storedCentreX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &storedCentreY, 2) <= 0;
            nJoystickCentreX = (unsigned int)storedCentreX;
            nJoystickCentreY = (unsigned int)storedCentreY;
            _close(file);
            if (failed != 0)
                _unlink("j.cal");
        }

        if (failed != 0) {
            GetJoystickDevCaps(activeDevice,
                               (short *)&minimumX,
                               (short *)&maximumX,
                               (short *)&minimumY,
                               (short *)&maximumY);
            centreX = ((int)minimumX + (int)maximumX) / 2;
            centreY = ((int)maximumY + (int)minimumY) / 2;
        } else {
            centreX = nJoystickCentreX;
            centreY = nJoystickCentreY;
        }

        if (centreX > 10) {
            nJoystickCalibrationMinimumX = centreX - 10;
            nJoystickCalibrationMaximumX = centreX + 10;
        } else {
            nJoystickCalibrationMinimumX = centreX;
            nJoystickCalibrationMaximumX = centreX;
        }
        if (centreY > 10) {
            nJoystickCalibrationMinimumY = centreY - 10;
            nJoystickCalibrationMaximumY = centreY + 10;
        } else {
            nJoystickCalibrationMaximumY = centreY;
            nJoystickCalibrationMinimumY = centreY;
        }

        rangeX = horizontalRange;
        rangeY = verticalRange;
        nJoystickLeftScale = nJoystickCalibrationMinimumX / rangeX;
        nJoystickUpScale = nJoystickCalibrationMinimumY / rangeY;
        if (nJoystickLeftScale == 0)
            nJoystickLeftScale++;
        if (nJoystickUpScale == 0)
            nJoystickUpScale++;
        nJoystickRightScale = nJoystickCalibrationMinimumX / rangeX;
        if (nJoystickRightScale == 0)
            nJoystickRightScale++;
        nJoystickDownScale = nJoystickCalibrationMinimumY / rangeY;
        if (nJoystickDownScale == 0)
            nJoystickDownScale++;

        nJoystickCentreX = centreX;
        nJoystickCentreY = centreY;
        nJoystickMinimumX =
            centreX - rangeX * nJoystickLeftScale;
        nJoystickMinimumY =
            centreY - rangeY * nJoystickUpScale;
        nJoystickMaximumX =
            rangeX * nJoystickLeftScale + centreX;
        nJoystickMaximumY =
            rangeY * nJoystickUpScale + centreY;
        nJoystickHorizontalDeadZone = horizontalDeadZone;
        nJoystickVerticalDeadZone = verticalDeadZone;
    }
    nActiveInputDevice = activeDevice;
}

/* Function start: 0x431D20 */
short ReadCalibratedJoystick(void)
{
    InputDeviceSample *sample;
    int sampleResult;
    int rawX;
    int rawY;
    short normalizedX;
    short normalizedY;
    short device;

    device = nActiveInputDevice;
    if (device == -1)
        return 0;
    sample = &aInputDeviceSamples[device];
    sampleResult = SampleJoystickDevice(
        sample, device,
        nJoystickFailureValue);
    rawX = sample->x;
    if (rawX == nJoystickFailureValue ||
        (rawY = aInputDeviceSamples[device].y) == nJoystickFailureValue ||
        sampleResult != 0) {
        nActiveInputDevice = -1;
        sample->x = aInputDeviceSamples[device].y =
            aInputDeviceSamples[device].buttons = 0;
        return 0;
    }

    nJoystickRawX = rawX;
    nJoystickRawY = rawY;
    if (nJoystickMinimumX > sample->x)
        sample->x = nJoystickMinimumX;
    if (sample->x > nJoystickMaximumX)
        sample->x = nJoystickMaximumX;
    if (aInputDeviceSamples[device].y <
        nJoystickMinimumY)
        aInputDeviceSamples[device].y =
            nJoystickMinimumY;
    if (aInputDeviceSamples[device].y >
        nJoystickMaximumY)
        aInputDeviceSamples[device].y =
            nJoystickMaximumY;

    normalizedY = 0;
    normalizedX = 0;
    rawX = sample->x;
    if (nJoystickCentreX > rawX) {
        normalizedX = (short)((nJoystickCentreX - rawX) /
                              nJoystickLeftScale);
        if (nJoystickHorizontalDeadZone < normalizedX) {
            if (normalizedX != 0)
                normalizedX = (short)-normalizedX;
        } else {
            normalizedX = 0;
        }
    } else if (nJoystickCentreX < rawX) {
        normalizedX = (short)((rawX - nJoystickCentreX) /
                              nJoystickRightScale);
        if (normalizedX <= nJoystickHorizontalDeadZone)
            normalizedX = 0;
    }

    rawY = aInputDeviceSamples[device].y;
    if (nJoystickCentreY > rawY) {
        normalizedY = (short)((nJoystickCentreY - rawY) /
                              nJoystickUpScale);
        if (nJoystickVerticalDeadZone < normalizedY) {
            if (normalizedY != 0)
                normalizedY = (short)-normalizedY;
        } else {
            normalizedY = 0;
        }
    } else if (nJoystickCentreY < rawY) {
        normalizedY = (short)((rawY - nJoystickCentreY) /
                              nJoystickDownScale);
        if (normalizedY <= nJoystickVerticalDeadZone)
            normalizedY = 0;
    }
    sample->x = (int)normalizedX;
    aInputDeviceSamples[device].y = (int)normalizedY;
    return 1;
}

/* Function start: 0x431EA0 */
void __stdcall UnionRectBounds(ShortRect *destination,
                               const ShortRect *first,
                               const ShortRect *second)
{
    destination->left = first->left < second->left ?
        first->left : second->left;
    destination->top = first->top < second->top ?
        first->top : second->top;
    destination->right = first->right > second->right ?
        first->right : second->right;
    destination->bottom = first->bottom > second->bottom ?
        first->bottom : second->bottom;
}
