/*
 * Interactive joystick calibration and button waits.
 * Placement and boundary evidence are recorded in docs/ORDER.md.
 */
#include "wc1.h"

/* Function start: 0x4102B0 */
void CalibrateJoystickInteractive()
{
    InputDeviceSample samples[2];
    short calibration[6];
    short shown;
    int file;
    int failed;
    int fontLoaded;
    int horizontalRange;
    int verticalRange;
    int leftScale;
    int rightScale;
    int upScale;
    int downScale;

    fontLoaded = 0;
    if (nActiveInputDevice == -1)
        return;
    stDefaultTextContext.alignment = 2;
    if (apTextFonts[1] != 0)
        fontLoaded = 1;

    InitializeTextContextFromFont(&stDefaultTextContext, 1,
        cViewportClearColour, (signed char)cBlackColour);
    shown = ShowModalTextPanel(1,
        "Turn AUTO FIRE off if present, press a button");
    if (shown != 0) {
        DIBslamReal();
        WaitForJoystickButtonPress();
        WaitForJoystickButtonRelease();
        ReleaseModalTextPanel();
    }

    shown = ShowModalTextPanel(1,
        "Move stick to the UPPER LEFT, press a button");
    if (shown != 0) {
        DIBslamReal();
        WaitForJoystickButtonPress();
        SampleJoystickDevice(samples, nActiveInputDevice, 0x7fff);
        calibration[0] = (short)(nJoystickMinimumX =
            samples[nActiveInputDevice].x);
        calibration[1] = (short)(nJoystickMinimumY =
            samples[nActiveInputDevice].y);
        WaitForJoystickButtonRelease();
        ReleaseModalTextPanel();
    }

    shown = ShowModalTextPanel(1,
        "Move stick to the LOWER RIGHT, press a button");
    if (shown != 0) {
        DIBslamReal();
        WaitForJoystickButtonPress();
        SampleJoystickDevice(samples, nActiveInputDevice, 0x7fff);
        calibration[2] = (short)(nJoystickMaximumX =
            samples[nActiveInputDevice].x);
        calibration[3] = (short)(nJoystickMaximumY =
            samples[nActiveInputDevice].y);
        WaitForJoystickButtonRelease();
        ReleaseModalTextPanel();
    }

    shown = ShowModalTextPanel(1,
        "Center Joystick, press a button");
    if (shown != 0) {
        DIBslamReal();
        WaitForJoystickButtonPress();
        SampleJoystickDevice(samples, nActiveInputDevice, 0x7fff);
        calibration[4] = (short)(nJoystickCentreX =
            samples[nActiveInputDevice].x);
        calibration[5] = (short)(nJoystickCentreY =
            samples[nActiveInputDevice].y);
        WaitForJoystickButtonRelease();
        ReleaseModalTextPanel();
    }

    if (fontLoaded == 0)
        ReleaseTextFont(1);

    horizontalRange = nJoystickHorizontalRange;
    if (horizontalRange == 0)
        horizontalRange = 9;
    verticalRange = nJoystickVerticalRange;
    if (verticalRange == 0)
        verticalRange = 9;
    leftScale =
        (nJoystickCentreX - nJoystickMinimumX) /
        horizontalRange;
    rightScale =
        (nJoystickMaximumX - nJoystickCentreX) /
        horizontalRange;
    upScale =
        (nJoystickCentreY - nJoystickMinimumY) /
        verticalRange;
    downScale =
        (nJoystickMaximumY - nJoystickCentreY) /
        verticalRange;
    if (leftScale == 0)
        leftScale++;
    if (upScale == 0)
        upScale++;
    if (rightScale == 0)
        rightScale++;
    if (downScale == 0)
        downScale++;
    nJoystickLeftScale = leftScale;
    nJoystickUpScale = upScale;
    nJoystickRightScale = rightScale;
    nJoystickDownScale = downScale;
    nJoystickHorizontalRange = horizontalRange;
    nJoystickVerticalRange = verticalRange;

    nJoystickMinimumX = nJoystickCentreX -
        leftScale * horizontalRange;
    nJoystickMinimumY = nJoystickCentreY -
        upScale * verticalRange;
    nJoystickMaximumX = nJoystickCentreX +
        rightScale * horizontalRange;
    nJoystickMaximumY = nJoystickCentreY +
        downScale * verticalRange;
    nJoystickFailureValue = nJoystickMaximumX * 2;

    if (nJoystickMaximumX <= nJoystickMinimumX ||
        nJoystickMaximumY <= nJoystickMinimumY) {
        shown = ShowModalTextPanel(1,
            "FAILED! Center Joystick, press a button");
        if (shown != 0) {
            DIBslamReal();
            WaitForJoystickButtonPress();
            WaitForJoystickButtonRelease();
            LoadJoystickCalibrationFile(8, 8, 1, 1);
            ReleaseModalTextPanel();
        }
        _unlink("j.cal");
        return;
    }

    file = _open("j.cal", 0x8301, 0x180);
    if (file == -1)
        return;
    failed = _write(file, &nActiveInputDevice, 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[0], 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[1], 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[2], 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[3], 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[4], 2) == -1;
    if (failed == 0)
        failed = _write(file, &calibration[5], 2) == -1;
    _close(file);
    if (failed != 0)
        _unlink("j.cal");
}

/* Function start: 0x4106C0 */
void WaitForJoystickButtonRelease(void)
{
    do {
        SampleJoystickDevice(
            &aInputDeviceSamples[
                nActiveInputDevice],
            nActiveInputDevice, 0);
    } while (aInputDeviceSamples[
                 nActiveInputDevice].buttons != 0);
}

/* Function start: 0x410700 */
void WaitForJoystickButtonPress(void)
{
    do {
        SampleJoystickDevice(
            &aInputDeviceSamples[
                nActiveInputDevice],
            nActiveInputDevice, 0);
    } while (aInputDeviceSamples[
                 nActiveInputDevice].buttons == 0);
}
