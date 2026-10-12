/* SWC CODE_01 sound player, with SDL replacing the Mac Sound Manager.
 * Four voices share the snd bank for effects and inflight speech. */
#include "wc1.h"
#include "swc.h"

#include <stdio.h>

typedef struct SwcSoundChannel {
    int sound;
    int volume;
    uint64_t position; /* 16.16 sample position. */
} SwcSoundChannel;

static SwcSound swcSounds[501];
static SwcSoundChannel swcSoundChannels[4];
static SDL_AudioDeviceID swcSoundDevice;
static int swcSoundInitialized;

/* Sound Manager completion and sample scheduling are owned by SDL here.
 * No game state, allocation, resource lookup or renderer calls on this thread. */
static void SwcMixSound(void *context, Uint8 *stream, int bytes)
{
    Sint16 *output = (Sint16 *)stream;
    int frame;
    int channel;
    int mixed;

    (void)context;
    for (frame = 0; frame < bytes / (int)sizeof(*output); frame++) {
        mixed = 0;
        for (channel = 0; channel < 4; channel++) {
            SwcSoundChannel *voice = &swcSoundChannels[channel];
            SwcSound *sample;
            uint64_t end;

            if (voice->sound == 0)
                continue;
            sample = &swcSounds[voice->sound];
            end = (uint64_t)sample->samples.size << 16;
            if (voice->sound == 12) {
                /* CODE_01 play_looped_sound +0x66be/+0x66c6 overrides
                   the stored loop with [0x23f8,0x32f5) sample offsets. */
                end = UINT64_C(0x32f5) << 16;
                if (voice->position >= end)
                    voice->position = (UINT64_C(0x23f8) << 16) +
                        (voice->position - end) % (UINT64_C(0xefd) << 16);
            } else if (voice->position >= end) {
                voice->sound = 0;
                continue;
            }
            mixed += ((int)sample->samples.data[voice->position >> 16] - 128) *
                voice->volume;
            voice->position += sample->rate / 22050U;
        }
        output[frame] = (Sint16)SDL_clamp(mixed, -32768, 32767);
    }
}

/* CODE_01 shut_down_sounds +0x655c. Close joins the callback before freeing. */
void SdlFreeSwcSound(void)
{
    size_t sound;

    if (swcSoundDevice != 0)
        SDL_CloseAudioDevice(swcSoundDevice);
    swcSoundDevice = 0;
    for (sound = 0; sound < SDL_arraysize(swcSounds); sound++) {
        SDL_free(swcSounds[sound].samples.data);
        SDL_zero(swcSounds[sound]);
    }
    SDL_zero(swcSoundChannels);
    if (swcSoundInitialized)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    swcSoundInitialized = 0;
}

/* CODE_01 init_sound +0x6052 and CMGetChunk("snd ",id).
 * Cache the small demo bank before flight so playback never reads from disk. */
int SdlInitSwcSound(const char *resourceFork)
{
    SwcCmf cmf = {0};
    SwcBuffer resource = {0};
    SDL_AudioSpec desired;
    char path[4096];
    size_t index;
    int result = -1;

    SdlFreeSwcSound();
    if (!SdlResolvePath("CMFs/Sounds.CMF", path, sizeof(path)) ||
        SwcCMOpen(path, &cmf) != 0)
        goto done;
    for (index = 0; index < cmf.chunkCount; index++) {
        const SwcChunkInfo *chunk = &cmf.chunks[index];

        if (memcmp(chunk->type, "snd ", 4) != 0)
            continue;
        if (chunk->id == 0 || chunk->id >= SDL_arraysize(swcSounds) ||
            swcSounds[chunk->id].samples.data != NULL) {
            SDL_SetError("Invalid or duplicate SWC sound ID %u", chunk->id);
            goto done;
        }
        if (SwcCMGetChunk(&cmf, "snd ", chunk->id, &resource) != 0 ||
            SwcReadSound(&resource, &swcSounds[chunk->id]) != 0)
            goto done;
        SDL_free(resource.data);
        SDL_zero(resource);
    }
    /* stop_afterburner_sound +0x67fc uses GetResource, not the CMF bank. */
    if (SwcReadResource(resourceFork, "snd ", 500, &resource) != 0 ||
        SwcReadSound(&resource, &swcSounds[500]) != 0)
        goto done;
    if (swcSounds[12].samples.size < 0x32f5) {
        SDL_SetError("SWC afterburner sample is shorter than its loop");
        goto done;
    }
    result = 0;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        goto unavailable;
    swcSoundInitialized = 1;
    SDL_zero(desired);
    desired.freq = 22050;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = SwcMixSound;
    swcSoundDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (swcSoundDevice == 0)
        goto unavailable;
    SDL_PauseAudioDevice(swcSoundDevice, 0);
    goto done;
unavailable:
    fprintf(stderr, "SWC flight audio unavailable: %s\n", SDL_GetError());
done:
    SDL_free(resource.data);
    SwcCMClose(&cmf);
    if (result != 0)
        SdlFreeSwcSound();
    return result;
}

void SdlPauseSwcSound(int paused)
{
    if (swcSoundDevice != 0)
        SDL_PauseAudioDevice(swcSoundDevice, paused != 0);
}

/* CODE_01 play_sound +0x626e: already playing IDs are not restarted;
 * idle channels are reused, and a full four-channel mixer rejects the play. */
int SdlPlaySwcSound(int sound, int volume)
{
    int channel;
    int idle = -1;

    if (swcSoundDevice == 0 || sound <= 0 ||
        sound >= (int)SDL_arraysize(swcSounds) || swcSounds[sound].samples.data == NULL)
        return -1;
    SDL_LockAudioDevice(swcSoundDevice);
    for (channel = 0; channel < 4; channel++) {
        if (swcSoundChannels[channel].sound == sound) {
            SDL_UnlockAudioDevice(swcSoundDevice);
            return 1;
        }
        if (idle == -1 && swcSoundChannels[channel].sound == 0)
            idle = channel;
    }
    if (idle != -1) {
        swcSoundChannels[idle].sound = sound;
        swcSoundChannels[idle].volume = SDL_clamp(volume, 0, 255);
        swcSoundChannels[idle].position = 0;
    }
    SDL_UnlockAudioDevice(swcSoundDevice);
    return idle != -1;
}

/* CODE_01 stop_sound +0x61c8 / is_sound_playing +0x5f8e. */
void SdlStopSwcSound(int sound)
{
    int channel;

    if (swcSoundDevice == 0)
        return;
    SDL_LockAudioDevice(swcSoundDevice);
    for (channel = 0; channel < 4; channel++) {
        if (swcSoundChannels[channel].sound == sound)
            swcSoundChannels[channel].sound = 0;
    }
    SDL_UnlockAudioDevice(swcSoundDevice);
}

int SdlSwcSoundPlaying(int sound)
{
    int channel;
    int playing = 0;

    if (swcSoundDevice == 0)
        return 0;
    SDL_LockAudioDevice(swcSoundDevice);
    for (channel = 0; channel < 4; channel++) {
        if (swcSoundChannels[channel].sound == sound)
            playing = 1;
    }
    SDL_UnlockAudioDevice(swcSoundDevice);
    return playing;
}

/* CODE_01 stop_afterburner_sound +0x678a replaces the loop in its own
 * channel with snd/500, preserving the current volume. */
void SdlStopSwcAfterburnerSound(void)
{
    int channel;

    if (swcSoundDevice == 0)
        return;
    SDL_LockAudioDevice(swcSoundDevice);
    for (channel = 0; channel < 4; channel++) {
        if (swcSoundChannels[channel].sound == 12) {
            swcSoundChannels[channel].sound = 500;
            swcSoundChannels[channel].position = 0;
        }
    }
    SDL_UnlockAudioDevice(swcSoundDevice);
}

/* CODE_09 sound_effect +0x1508: mono volume falls by one per 100 units.
 * CODE_05 fire_weapon +0x379a distinguishes the Kilrathi laser (snd/26). */
int SdlSwcSoundEffect(int sound, int object)
{
    FixedVector delta;
    int volume = 127;

    if (object != -1) {
        if (object < 0 || object >= SPACE_OBJECT_COUNT)
            return 0;
        ComputeVectorDelta(&aShipPosition[EYE_OBJECT], &aShipPosition[object], &delta);
        volume -= (Vector_magnitude(&delta) / 100) >> 8;
        if (volume < 10)
            return 1;
        if (sound == 8 && aeShipSide[object] != SIDE_IMPERIAL)
            sound = 26;
    }
    return SdlPlaySwcSound(sound, volume);
}
