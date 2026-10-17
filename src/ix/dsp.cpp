/*
 *  ix / D:\rnd\prj\ix\win95\dsp\dsp.cpp
 *
 *  DSP device control and shared configuration. Source anchors and original
 *  storage placement are recorded in docs/ORDER.md.
 *
 *  Implement in ADDRESS ORDER -- MSVC emits functions in source order, so the
 *  list below is the original source order of this file.
 */
#include "ix.h"
#ifndef SDL_PORT
#include <dsound.h>
#endif
#include <stdlib.h>
#include <string.h>

#define IX_DSP_FILE "D:\\rnd\\prj\\ix\\win95\\dsp\\dsp.cpp"

unsigned short nMasterVolume = 0xffff; /* 0x0047198C */
#ifdef SDL_PORT
void *(__cdecl *pIxMalloc)(unsigned int) =
    (void *(__cdecl *)(unsigned int))malloc; /* 0x00471990 */
#else
void *(__cdecl *pIxMalloc)(unsigned int) = malloc; /* 0x00471990 */
#endif
void (__cdecl *pIxFree)(void *) = free; /* 0x00471994 */

HANDLE hMixerThread;                   /* 0x00597D00 */
HWND hDspWindow;                       /* 0x00597D04 */
HANDLE hMixerWakeEvent;                /* 0x00597D08 */
unsigned int dwMixerWriteOffset;       /* 0x00597D0C */
LPDIRECTSOUND pDirectSound;            /* 0x00597D10 */
unsigned int dwMixerBufferSize;        /* 0x00597D14 */
unsigned int dwDspFlags;               /* 0x00597D18 */
DWORD dwMixerThreadId;                /* 0x00597D1C */
LPDIRECTSOUNDBUFFER pMixerBuffer;      /* 0x00597D20 */
short anPanTable[512];                /* 0x00597D28 */
unsigned int dwDspTick;                /* 0x00598128 */
LPDIRECTSOUNDBUFFER pPrimarySoundBuffer; /* 0x0059812C */

/* Function start: 0x00444910 */   /* source line 62 */
int ix_dsp_init(void)
{
    if ((dwDspFlags & 1) == 0) {
        nVoiceCount = 32;
        nStreamCount = 2;
        dwMixerWriteOffset = 0;
        InitializeCriticalSection(&csMixer);
        hMixerWakeEvent = CreateEventA(0, TRUE, FALSE, 0);
        ix_dsp_build_pan_tables();
#ifdef SDL_PORT
        dwDspFlags |= 4;
#endif
        hMixerThread = CreateThread(
            0, 0x1000, ix_mixer_thread_proc, 0, 0,
            &dwMixerThreadId);
        if (hMixerThread == 0) {
            ix_log_printf("Fatal [%s - %d]:\n", IX_DSP_FILE, 62);
            ix_log_printf("Failed to start mixer");
            exit(-1);
        }
        dwDspFlags |= 1;
    }
    return 0;
}

/* Function start: 0x004449CF */
void ix_dsp_shutdown(void)
{
    if ((dwDspFlags & 1) != 0) {
#ifdef SDL_PORT
        if (hMixerThread != 0) {
            dwDspFlags &= ~4U;
            SetEvent(hMixerWakeEvent);
            WaitForSingleObject(hMixerThread, INFINITE);
            CloseHandle(hMixerThread);
            hMixerThread = 0;
        }
#else
        if ((dwDspFlags & 4) != 0) {
            dwDspFlags &= ~4U;
            SetEvent(hMixerWakeEvent);
            WaitForSingleObject(hMixerThread, INFINITE);
        }
        if (pDirectSound != 0) {
            pDirectSound->Release();
            pDirectSound = 0;
        }
#endif
        DeleteCriticalSection(&csMixer);
        CloseHandle(hMixerWakeEvent);
        dwDspFlags &= 0x7ffffffe;
    }
}

/* Function start: 0x00444A62 */
void ix_dsp_configure(int option, void *value)
{
    switch (option) {
    case 0:
        hDspWindow = (HWND)value;
        break;
    case 1:
        pDirectSound = (LPDIRECTSOUND)value;
        break;
    case 2:
        if ((dwDspFlags & 1) != 0)
            SetEvent(hMixerWakeEvent);
        break;
    case 3:
        if (value != 0)
            dwDspFlags |= 0x10;
        else
            dwDspFlags &= ~0x10U;
        break;
    case 4:
        pIxMalloc = (void *(__cdecl *)(unsigned int))value;
        break;
    case 5:
        pIxFree = (void (__cdecl *)(void *))value;
        break;
    }
}

/* Function start: 0x00444B25 */
BOOL CALLBACK ix_dsp_open_driver(LPGUID guid, LPSTR description,
                                 LPSTR module, LPVOID context)
{
#ifdef SDL_PORT
    (void)guid;
    (void)description;
    (void)module;
    (void)context;
    return TRUE;
#else
    HRESULT result;

    if (pDirectSound != 0) {
        pDirectSound->Release();
        pDirectSound = 0;
    }
    result = DirectSoundCreate(guid, &pDirectSound, 0);
    if (result == DS_OK) {
        result = pDirectSound->SetCooperativeLevel(
            hDspWindow,
            (dwDspFlags & 0x10) == 0
                ? DSSCL_WRITEPRIMARY : DSSCL_EXCLUSIVE);
        if (result == DS_OK) {
            ix_log_printf("sound driver: %s [%s]\n", description, module);
            return FALSE;
        }
        if (pDirectSound != 0) {
            pDirectSound->Release();
            pDirectSound = 0;
        }
    }
    return TRUE;
#endif
}

/* Function start: 0x00444BFD */
unsigned int ix_dsp_get_tick(void)
{
    return dwDspTick;
}

/* Function start: 0x00444C12 */
unsigned int ix_dsp_get_flags(void)
{
    return dwDspFlags;
}

/* Function start: 0x00444C27 */
void ix_dsp_set_config_bit1(int enabled)
{
    if (enabled != 0)
        dwDspFlags |= 2;
    else
        dwDspFlags &= ~2U;
}

/* Function start: 0x00444C54 */
void ix_dsp_set_config_bit3(int enabled)
{
    if (enabled != 0)
        dwDspFlags |= 8;
    else
        dwDspFlags &= ~8U;
}

/* Function start: 0x00444C81 */
int ix_dsp_get_voice_count(void)
{
    return nVoiceCount;
}

/* Function start: 0x00444C96 */   /* source line(s) 176: can't change voice count while voices are in use */
void ix_dsp_set_voice_count(int voiceCount)
{
    if (nVoicesAllocated != 0 ||
        nStreamsAllocated != 0) {
        ix_log_printf("Warning [%s - %d]:\n", IX_DSP_FILE, 176);
        ix_log_printf("can't change voice count while voices are in use");
        return;
    }
    if (voiceCount >= 0)
        nVoiceCount = voiceCount < 32 ? voiceCount : 32;
    else
        nVoiceCount = 0;
}

/* Function start: 0x00444D18 */
int ix_dsp_get_stream_count(void)
{
    return nStreamCount;
}

/* Function start: 0x00444D2D */   /* source line(s) 187: can't change stream count while voices are in use */
void ix_dsp_set_stream_count(int streamCount)
{
    if (nVoicesAllocated != 0 ||
        nStreamsAllocated != 0) {
        ix_log_printf("Warning [%s - %d]:\n", IX_DSP_FILE, 187);
        ix_log_printf("can't change stream count while voices are in use");
        return;
    }
    if (streamCount >= 0)
        nStreamCount = streamCount < 2 ? streamCount : 2;
    else
        nStreamCount = 0;
}

/* Function start: 0x00444DAF */
unsigned short ix_dsp_get_master_volume(void)
{
    return nMasterVolume;
}

/* Function start: 0x00444DC5 */
void ix_dsp_set_master_volume(unsigned short volume)
{
    int voice;

    if ((volume & 0xffff) >= 0) {
        nMasterVolume = (unsigned short)(
            (volume & 0xffff) < 0xffff ? (volume & 0xffff) : 0xffff);
    } else {
        nMasterVolume = 0;
    }
    for (voice = 0; voice < nVoiceCount; voice++) {
        if ((voices[voice].flags & IX_VOICE_ACTIVE) != 0)
            ix_dspv_recalc_mix(voice);
    }
}

/* Function start: 0x00444E52 */
void ix_dsp_build_pan_tables(void)
{
    short volumeStep;
    short left;
    short panDistance;
    short panPosition;
    short right;

    volumeStep = 0x3ff;
    left = 0x7fff;
    for (panPosition = 0; panPosition < 0x40; panPosition++) {
        panDistance = (short)(0x20 - panPosition);
        right = (short)(panDistance * volumeStep);
        anPanTable[panPosition * 2] = left;
        anPanTable[panPosition * 2 + 1] = right;
        anPanTable[0x100 + panPosition * 2] =
            (short)(left * -1);
        anPanTable[0x100 + panPosition * 2 + 1] =
            (short)(right * -1);
    }
    right = (short)0x8001;
    for (panPosition = 0x40; panPosition < 0x80; panPosition++) {
        panDistance = (short)(0x60 - panPosition);
        left = (short)(panDistance * volumeStep);
        anPanTable[panPosition * 2] = left;
        anPanTable[panPosition * 2 + 1] = right;
        anPanTable[0x100 + panPosition * 2] =
            (short)(left * -1);
        anPanTable[0x100 + panPosition * 2 + 1] =
            (short)(right * -1);
    }
}

/* Function start: 0x00444F97 */
const char *ix_dsp_result_to_text(int result)
{
#ifdef SDL_PORT
    (void)result;
    return SDL_GetError();
#else
    switch (result) {
    case DS_OK:
        return "The function succeeded.";
    case DSERR_ALLOCATED:
        return "The function failed because resources (such as a priority level) were already in use by another caller.";
    case DSERR_ALREADYINITIALIZED:
        return "This object is already initialized.";
    case DSERR_BADFORMAT:
        return "The specified wave format is not supported.";
    case DSERR_BUFFERLOST:
        return "The buffer memory has been lost and must be restored.";
    case DSERR_CONTROLUNAVAIL:
        return "The control (volume, pan, and so forth) requested by the caller is not available.";
    case DSERR_INVALIDCALL:
        return "This function is not valid for the current state of this object";
    case DSERR_INVALIDPARAM:
        return "An invalid parameter was passed to the returning function.";
    case DSERR_NOAGGREGATION:
        return "This object does not support aggregation.";
    case DSERR_NODRIVER:
        return "No sound driver is available for use.";
    case DSERR_OUTOFMEMORY:
        return "The DirectSound subsystem couldn't allocate sufficient memory to complete the caller's request.";
    case DSERR_PRIOLEVELNEEDED:
        return "The caller does not have the priority level required for the function to succeed.";
    case E_NOINTERFACE:
        return "The requested COM interface is not available.";
    default:
        return "Unknow dsound error!";
    }
#endif
}

/* Function start: 0x00445123 */
void *ix_dsp_alloc(unsigned int bytes)
{
    return pIxMalloc(bytes);
}

/* Function start: 0x00445140 */
#pragma function(memcpy)
void *ix_dsp_copy(void *destination, const void *source, unsigned int bytes)
{
    return memcpy(destination, source, bytes);
}

/* Function start: 0x00445164 */
void ix_dsp_free(void *memory)
{
    pIxFree(memory);
}

/* Function start: 0x00445190 */
void ix_dsp_static_initializer(void)
{
    ix_dsp_static_initialization_hook();
}

/* Function start: 0x004451A5 */
void ix_dsp_static_initialization_hook(void)
{
}
