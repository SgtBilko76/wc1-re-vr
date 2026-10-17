/*
 *  Process-level services: memory reporting and exit.
 *
 *  Address range 0x4272f0-0x4274df (provisional -- see docs/ORDER.md).
 *  Boundary evidence: exit_squadron/ShowMemoryStatusDebug; string band 0x46A064-0x46A10C.
 */
#include "wc1.h"

/* Function start: 0x4272F0 */
short LogMemoryUsage(void)
{
    ShutdownHook(0x21, (void *)DAT_0059ab4c);
    EMShutDown();
    GetMessagePumpResult();
    chdir("..");
    ShutdownVideoHook(3);
    if (nOriginDevUnlock != 0) {
        SystemDebugPrintf(
            "Original FMem %lu.  Current FMem %lu.  Current NMem %u.\n",
            dwOriginalFreeMemory,
            GetFixedOneMillionThunkAlt(0),
            (unsigned int)(int)(short)GetOriginalFreeMemory());
    }
    MouseIdleHook();
    MessagePumpHook(8);
    return 0;
}

/* Function start: 0x427370 */
void exit_squadron(const char *msg)
{
    LogMemoryUsage();
    SystemDebugPrintf(msg);
    SystemDebugPrintf("[SYSTEM]: Exit_squadron\n");
    WriteDebugString("[SYSTEM] exit_squadron");
    WriteDebugString(msg);
    ClearDebugPauseFlags();
    AbortToDesktop();
    exit(0);
}

/* Function start: 0x4273C0 */
unsigned int ShowMemoryStatusDebug(void)
{
    TextContext savedContext;
    TextContext *previousContext;
    char value[60];

    previousContext = pCurrentTextContext;
    savedContext = stDefaultTextContext;
    if (nShowMemoryStatus != 0) {
        InitializeTextContextFromFont(
            &stDefaultTextContext, 1,
            (unsigned char)cViewportClearColour,
            cBlackColour);
        SetTextContext(&stDefaultTextContext);
        DrawFormattedText("%X%YCurrent NMem %d.",
                          0, 176, (int)(short)GetOriginalFreeMemory());
        sprintf(value, "%ld", GetFixedOneMillionThunkAlt(0));
        DrawFormattedText("%X%YCurrent FMem %s.", 0, 184, value);
        sprintf(value, "%ld", dwOriginalFreeMemory);
        DrawFormattedText("%X%YOriginal FMem %s.", 0, 0, value);
        stDefaultTextContext = savedContext;
    }
    pCurrentTextContext = previousContext;
    return 0;
}

/* Function start: 0x4274C0 */
unsigned int GetJoystickButtonEdge(unsigned int a, short b)
{
    (void)a;
    if (b < 0)
        return JoystickEdgeHook(-1);
    return 0;
}
