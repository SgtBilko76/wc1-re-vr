/* Win32 frame timer and developer step waiting.
 * The 42afa0-42afff range remains provisional; see docs/ORDER.md.
 */
#include "wc1.h"

/* Function start: 0x42AFA0 */
void WaitForDebugStep(void)
{
    while (TakeDebugStepFlag() == 0) ;
}

/* Function start: 0x42AFB0 */
void CALLBACK FrameTimerCallback(UINT timerId, UINT message,
                                 DWORD user, DWORD first, DWORD second)
{
    (void)timerId;
    (void)message;
    (void)user;
    (void)first;
    (void)second;
    bFrameTickPending = 0;
}

/* Function start: 0x42AFC0 */
void SetMultimediaTimerCallback(int period)
{
    int milliseconds = period * 1000 / 60;

    if (period == 0) {
        bFrameTickPending = 0;
        if (nFrameTimerId != 0) {
            timeKillEvent(nFrameTimerId);
            nFrameTimerId = 0;
        }
    } else {
        if (nFrameTimerId != 0) {
            timeKillEvent(nFrameTimerId);
            nFrameTimerId = 0;
        }
        bFrameTickPending = 1;
        nFrameTimerId = timeSetEvent(milliseconds, milliseconds,
                                    FrameTimerCallback, 0, 0);
    }
}
