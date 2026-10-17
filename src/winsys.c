/*
 *  Win32 system services: mouse, keyboard and clock wrappers, font
 *  workspaces, CD-ROM location and disc-swap prompting.
 *
 *  Address range 0x402e00-0x4034ff (provisional -- see docs/ORDER.md).
 *  Boundary evidence: one unit, not two. PollKeyboardState's statics and
 *  LocateStreamsDirOnDisc's szStreamsPath interleave in the original's
 *  .bss (0x475B68-0x475D1F), and statics are private to one object;
 *  string band 0x46535C-0x4653FC.
 */
#include "wc1.h"

/* Function start: 0x402E80 */
void SetMousePosition(int x, int y)
{
    SetCursorPos(x, y);
}

/* Function start: 0x402EA0 */
unsigned int PollKeyboardState(void)
{
    nClearedKeyboardState = 0;
    nHomeKeyState = GetAsyncKeyState(VK_HOME);
    nUpArrowKeyState = GetAsyncKeyState(VK_UP);
    nPageUpKeyState = GetAsyncKeyState(VK_PRIOR);
    nLeftArrowKeyState = GetAsyncKeyState(VK_LEFT);
    nRightArrowKeyState = GetAsyncKeyState(VK_RIGHT);
    nEndKeyState = GetAsyncKeyState(VK_END);
    nDownArrowKeyState = GetAsyncKeyState(VK_DOWN);
    nPageDownKeyState = GetAsyncKeyState(VK_NEXT);
    nClearKeyState = GetAsyncKeyState(VK_CLEAR);
    nOemPeriodKeyState = GetAsyncKeyState(0xbe);
    nOemCommaKeyState = GetAsyncKeyState(0xbc);
    nInsertKeyState = GetAsyncKeyState(VK_INSERT);
    nDeleteKeyState = GetAsyncKeyState(VK_DELETE);

    if (nHomeKeyState != 0)
        return 0x47;
    if (nPageUpKeyState != 0)
        return 0x49;
    if (nEndKeyState != 0)
        return 0x4f;
    if (nPageDownKeyState != 0)
        return 0x51;
    if (nInsertKeyState != 0 ||
        nOemCommaKeyState != 0)
        return 0x52;
    if (nDeleteKeyState != 0 ||
        nOemPeriodKeyState != 0)
        return 0x53;
    if (nClearKeyState != 0)
        return 0x4c;
    if (nUpArrowKeyState != 0) {
        if (nLeftArrowKeyState != 0)
            return 0x47;
        return nRightArrowKeyState != 0 ? 0x49 : 0x48;
    }
    if (nDownArrowKeyState != 0) {
        if (nLeftArrowKeyState != 0)
            return 0x4f;
        return nRightArrowKeyState != 0 ? 0x51 : 0x50;
    }
    if (nLeftArrowKeyState != 0)
        return 0x4b;
    return nRightArrowKeyState != 0 ? 0x4d : 0;
}

/* Function start: 0x403060 */
int GetShiftKeyState(void)
{
    return GetAsyncKeyState(VK_SHIFT);
}

/* Function start: 0x403070 */
int GetControlKeyState(void)
{
    return GetAsyncKeyState(VK_CONTROL);
}

/* Function start: 0x403080 */
unsigned int GetKeyboardModifiers(void)
{
    return nSystemKeyDown;
}

/* Function start: 0x403090 */
unsigned int GetGameClockTicks(void)
{
    return (GetTickCount() - dwGameClockBase) * 60 / 1000;
}

/* Function start: 0x4030B0 */
void InitGameClockEpoch(void)
{
    dwGameClockBase = GetTickCount() + (rand() & 3600000);
}

/* Function start: 0x4030D0 */
void WriteDebugString(const char *s)
{
    OutputDebugString(s);
}

/* Function start: 0x4030E0 */
/* Byte-identical duplicate of SetMousePosition (0x00402E80); both exist because
 * VC++ 4.2's linker did no COMDAT folding. */
void SetMousePositionDuplicate(int x, int y)
{
    SetCursorPos(x, y);
}

/* Function start: 0x403100 */
FontWorkspace **AllocateFontWorkspace(short fontIndex)
{
    FontWorkspace **workspace;
    int offset;
    int dimension;

    (void)fontIndex;
    workspace = (FontWorkspace **)malloc(sizeof(*workspace));
    offset = 0;
    dimension = 5;
    *workspace = malloc(sizeof(**workspace));
    do {
        offset += sizeof(FontWorkspace);
        ((FontWorkspace *)((unsigned char *)*workspace + offset -
                           sizeof(FontWorkspace)))->width = dimension;
        ((FontWorkspace *)((unsigned char *)*workspace + offset -
                           sizeof(FontWorkspace)))->height = dimension;
        ((FontWorkspace *)((unsigned char *)*workspace + offset -
                           sizeof(FontWorkspace)))->pixels =
            malloc((*workspace)->width *
                                    (*workspace)->height);
        memset(((FontWorkspace *)((unsigned char *)*workspace + offset -
                                  sizeof(FontWorkspace)))->pixels,
               dimension, (*workspace)->width * (*workspace)->height);
    } while (offset < (int)sizeof(FontWorkspace));
    return workspace;
}

/* Function start: 0x403180 */
void FreeFontWorkspace(FontWorkspace **workspace)
{
    int offset;

    offset = 0;
    do {
        free(((FontWorkspace *)((unsigned char *)*workspace + offset))->pixels);
        offset += sizeof(FontWorkspace);
    } while (offset < (int)sizeof(FontWorkspace));
    free(*workspace);
    free(workspace);
}

/* Function start: 0x4031C0 */
char *LocateStreamsDirOnDisc(void)
{
    char current[256];
    char *result;
    char drive;

    result = szStreamsPath;
    GetCurrentDirectoryA(0xff, current);
    drive = FindCdRomDriveByVolumeLabel("<anydisc>", "\\wc1\\streams\\");
    if (drive != 0) {
        sprintf(result, "%c:%s", drive,
                "\\wc1\\streams\\");
        return result;
    }

#ifdef SDL_PORT
    if (strstr(current, "gamedat") != 0 ||
        strstr(current, "GAMEDAT") != 0)
#else
    if (strstr(current, "gamedat") != 0)
#endif
        sprintf(result, "%s\\..\\%s", current, "streams\\");
    else
        sprintf(result, "%s\\%s", current, "streams\\");
    GetCurrentDirectoryA(0xff, current);
    if (!SetCurrentDirectoryA(result))
        result = 0;
    SetCurrentDirectoryA(current);
    return result;
}

/* Function start: 0x403290 */
char FindCdRomDriveByVolumeLabel(const char *label,
                                 const char *directory)
{
    char drives[26];
    char scanRoot[12];
    char root[12];
    char volume[256];
    char filesystem[64];
    DWORD maximumComponentLength;
    DWORD flags;
    int driveCount = 0;
    int i;
    int scanDrive;
    char drive;
    char result;

    for (scanDrive = 'a'; scanDrive <= 'z'; scanDrive++) {
        sprintf(scanRoot, "%c:\\", (char)scanDrive);
        if (GetDriveTypeA(scanRoot) == DRIVE_CDROM) {
            drives[driveCount] = (char)scanDrive;
            driveCount++;
        }
    }

    result = 0;
    i = 0;
    while (i < driveCount) {
        drive = drives[i];
        sprintf(root, "%c:\\", drive);
        GetVolumeInformationA(root, volume, 0xff, 0,
                              &maximumComponentLength, &flags,
                              filesystem, sizeof(filesystem));
        if (memcmp(label, "<anydisc>", 10) == 0) {
            if (SetCurrentDirOnDrive(drive, directory) != 0) {
                result = drive;
                break;
            }
        } else if (strcmp(volume, label) == 0) {
            if (SetCurrentDirOnDrive(drive, directory) != 0) {
                result = drives[i];
                break;
            }
        }
        i++;
    }
    return result;
}

/* Function start: 0x4033E0 */
int SetCurrentDirOnDrive(char drive, const char *directory)
{
    char path[256];
    char current[256];
    int result;

    result = 0;
    GetCurrentDirectoryA(0xff, current);
    sprintf(path, "%c:%s", drive, directory);
    if (SetCurrentDirectoryA(path) != 0)
        result = 1;
    SetCurrentDirectoryA(current);
    return result;
}

/* Function start: 0x403450 */
int PromptInsertCorrectCd(void)
{
    char title[18];
    char message[1024];

    strcpy(title, "Insert Correct CD");
    sprintf(message,
            "Please place The Kilrathi Saga disc %d into your CD-ROM drive and click OK\n"
            "or click cancel to quit",
            1);
    do {
        if (LocateStreamsDirOnDisc() != 0)
            return 1;
    } while (MessageBoxA(0, message, title,
                         MB_OKCANCEL | MB_ICONEXCLAMATION) != IDCANCEL);
    return 0;
}

/* Function start: 0x4034D0 */
short __stdcall OpenDataFileOrDie(const char *path)
{
    short fd = (short)_open(path, 0x8002);

    nPacketError = (short)errno;
    return fd;
}
