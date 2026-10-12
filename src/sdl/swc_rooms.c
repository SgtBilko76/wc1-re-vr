/* SDL carrier presentation for the Mac demo's first mission.
 * Room routing: CODE_09 GameFlow +0x1b8e; CODE_07 RecRoom +0x059a;
 * CODE_03 BarracksScreen +0x5c70 and SceneDirector +0x254c.
 * WC1 owns pilot/campaign state, scene tests, substitutions and animation
 * parsing. Mac sprites, palettes, hit testing, audio and events are adapted here.
 * This is not a replacement for WC1's disk-backed room/scene director. */
#include "wc1.h"
#include "swc.h"

#include <string.h>

enum SwcRoomPackId {
    SWC_RECROOM, SWC_BARRACKS, SWC_MEDALS, SWC_SHOTGLASS, SWC_BAR_HEADS,
    SWC_BRIEFING, SWC_COMMANDER, SWC_WINGMEN, SWC_BRIEFING_MAP,
    SWC_DEBRIEFING, SWC_DEBRIEFING_HEADS, SWC_FUNERAL,
    SWC_INTRO_BEFORE_ATTACK, SWC_INTRO_AFTER_ATTACK, SWC_ROOM_PACKS
};

typedef struct SwcRoomArt {
    char type[4];
    uint32_t id;
    SwcBuffer shape;
    uint32_t count;
    SDL_Texture **textures;
    SDL_Rect *bounds;
} SwcRoomArt;

typedef struct SwcRoomPack {
    SwcCmf cmf;
    SDL_Color colors[256];
    SDL_Color dimColors[256];
    SwcRoomArt art[16];
    int count;
} SwcRoomPack;

typedef struct SwcRoomInput {
    int quit;
    int back;
    int activate;
    int clicked;
    int moved;
    int step;
} SwcRoomInput;

typedef struct SwcCarrier {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *font;
    SwcTextFont roomFont;
    SDL_Texture *picture;
    SDL_Texture *dimPicture;
    enum SwcRoomPackId picturePalette;
    int pictureVisible;
    SwcRoomInput pendingInput;
    SwcSpeechMark *marks;
    size_t markCount;
    int haveSpeech;
    const char *resourceFork;
    SwcRoomPack packs[SWC_ROOM_PACKS];
    SwcBuffer script;
    SDL_Scancode armedKey;
    int mouseDown;
    int mouseX;
    int mouseY;
    int focused;
    int paused;
    int drawResult;
    enum SwcCarrierVisit visit;
} SwcCarrier;

typedef struct SwcRoomScene {
    ConversationSceneRecord records[256];
    size_t count;
    unsigned char *text;
    size_t textSize;
} SwcRoomScene;

/* CODE_07 RecRoom +0x05f6..0x064e, +0x0d56..0x0dcc. Mac PtInRect
 * excludes right/bottom edges; characters override the door/board regions.
 * The Paladin/Angel bounds come from RCRM/29 and /28 frame zero. */
static const TitleMenuRegion swcBarRegions[] = {
    {0, 173, 76, 216, 106}, {1, 137, 99, 182, 191}, {2, 254, 103, 307, 193},
    {3, 250, 65, 318, 127}, {4, 268, 26, 320, 65}, {5, 0, 90, 70, 145},
    {-1, 0, 0, 0, 0}
};
static const char *const swcBarLabels[] = {
    "Talk to SHOTGLASS", "Talk to PALADIN", "Talk to ANGEL",
    "Enter BARRACKS", "Check Pilot Ranking", "Enter SIMULATOR"
};
/* CODE_03 BarracksScreen +0x5c70, original Mac rectangles. */
static const TitleMenuRegion swcBunkRegions[] = {
    {0, 235, 57, 260, 114}, {1, 50, 56, 85, 113}, {2, 150, 110, 183, 147},
    {3, 0, 71, 59, 92}, {4, 0, 139, 68, 160}, {5, 43, 74, 90, 90},
    {6, 11, 114, 88, 131}, {7, 250, 73, 320, 90}, {8, 244, 142, 320, 156},
    {9, 224, 75, 255, 88}, {10, 226, 115, 320, 132}, {-1, 0, 0, 0, 0}
};
static const char *const swcBunkLabels[] = {
    "Enter BRIEFING ROOM", "Enter RECROOM", "View MEDALS",
    "BUNK 1", "BUNK 2", "BUNK 3", "BUNK 4",
    "BUNK 5", "BUNK 6", "BUNK 7", "BUNK 8"
};

static SwcRoomArt *SwcGetRoomArt(SwcCarrier *carrier, enum SwcRoomPackId packId,
                                 const char type[4], uint32_t id)
{
    /* Expanded DATA/0 +0x4046 (filenames), +0x3fd2 (clut IDs). */
    static const char *const files[SWC_ROOM_PACKS] = {
        "RecRoomScene.CMF", "Barracks.CMF", "ViewMedals.CMF", "Head10.CMF",
        "RRTalkingHeads.CMF", "Briefing.CMF", "BriefingCommander.CMF",
        "BRTH1.CMF", "BRInterface.CMF", "DeBriefing.CMF", "DeBriefingHeads.CMF",
        "Funeral.CMF", "IntroFilm1.CMF", "IntroFilm2.CMF"
    };
    static const short palettes[SWC_ROOM_PACKS] = {
        135, 150, 151, 137, 136, 170, 171, 173, 172, 190, 191, 193, 130, 131
    };
    SwcRoomPack *pack = &carrier->packs[packId];
    SwcRoomArt *art;
    SwcFrame frame;
    char name[96];
    char path[PATH_MAX];
    uint32_t index;
    int entry;

    if (carrier->drawResult != 0)
        return NULL;
    if (pack->cmf.data == NULL) {
        SDL_snprintf(name, sizeof(name), "CMFs/%s", files[packId]);
        if (!SdlResolvePath(name, path, sizeof(path))) {
            SDL_SetError("Cannot resolve SWC carrier asset %s", name);
            goto fail;
        }
        if (SwcCMOpen(path, &pack->cmf) != 0 ||
            SwcReadPalette(carrier->resourceFork, palettes[packId], pack->colors) != 0)
            goto fail;
        if (packId < SWC_INTRO_BEFORE_ATTACK &&
            SwcReadPalette(carrier->resourceFork, palettes[packId] + 10, pack->dimColors) != 0)
            goto fail;
    }
    for (entry = 0; entry < pack->count; entry++) {
        art = &pack->art[entry];
        if (art->id == id && memcmp(art->type, type, 4) == 0)
            return art;
    }
    if (pack->count == (int)SDL_arraysize(pack->art)) {
        SDL_SetError("Too many SWC carrier shapes in %s", files[packId]);
        goto fail;
    }
    art = &pack->art[pack->count++];
    art->id = id;
    memcpy(art->type, type, 4);
    if (SwcCMGetChunk(&pack->cmf, type, id, &art->shape) != 0 ||
        SwcGetFrameCount(&art->shape, &art->count) != 0)
        goto fail;
    if (art->count == 0 || art->count > 128) {
        SDL_SetError("Invalid SWC carrier frame count");
        goto fail;
    }
    art->textures = SDL_calloc(art->count, sizeof(*art->textures));
    art->bounds = SDL_calloc(art->count, sizeof(*art->bounds));
    if (art->textures == NULL || art->bounds == NULL) {
        SDL_OutOfMemory();
        goto fail;
    }
    for (index = 0; index < art->count; index++) {
        if (SwcGetFramePtr(&art->shape, index, &frame) != 0)
            goto fail;
        if (frame.width > 640 || frame.height > 480) {
            SDL_SetError("Oversized SWC carrier frame");
            goto fail;
        }
        art->bounds[index] = (SDL_Rect){frame.x, frame.y, frame.width, frame.height};
    }
    return art;
fail:
    carrier->drawResult = -1;
    return NULL;
}

static void SwcDrawRoomArt(SwcCarrier *carrier, enum SwcRoomPackId packId,
                            const char type[4], uint32_t id, int frame, int x, int y)
{
    SwcRoomArt *art = SwcGetRoomArt(carrier, packId, type, id);
    SDL_Rect destination;

    if (art == NULL)
        return;
    if (frame < 0 || (uint32_t)frame >= art->count) {
        carrier->drawResult = SDL_SetError("Invalid SWC carrier frame %.4s/%u:%d", type, id, frame);
        return;
    }
    destination = art->bounds[frame];
    if (destination.w == 0 || destination.h == 0)
        return;
    if (art->textures[frame] == NULL) {
        art->textures[frame] = SdlCreateSwcTexture(carrier->renderer, &art->shape,
            (uint32_t)frame, carrier->packs[packId].colors);
        if (art->textures[frame] == NULL) {
            carrier->drawResult = -1;
            return;
        }
    }
    destination.x += x;
    destination.y += y;
    carrier->picturePalette = packId;
    carrier->drawResult = SDL_RenderCopy(carrier->renderer, art->textures[frame],
                                         NULL, &destination);
}

static SwcRoomInput SwcPollCarrier(SwcCarrier *carrier)
{
    SwcRoomInput input = carrier->pendingInput;
    SDL_Event event;

    SDL_zero(carrier->pendingInput);
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE))
            input.quit = 1;
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                carrier->focused = 0;
                carrier->armedKey = SDL_SCANCODE_UNKNOWN;
                carrier->mouseDown = 0;
            } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
                       event.window.event == SDL_WINDOWEVENT_RESTORED) {
                carrier->focused = (SDL_GetWindowFlags(carrier->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
            }
        }
        if (!carrier->focused)
            continue;
        if (event.type == SDL_MOUSEMOTION) {
            carrier->mouseX = event.motion.x;
            carrier->mouseY = event.motion.y;
            input.moved = 1;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            carrier->mouseDown = 1;
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            carrier->mouseX = event.button.x;
            carrier->mouseY = event.button.y;
            input.clicked = carrier->mouseDown;
            carrier->mouseDown = 0;
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            switch (event.key.keysym.sym) {
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE:
            case SDLK_ESCAPE:
                carrier->armedKey = event.key.keysym.scancode;
                break;
            case SDLK_TAB:
                input.step = (event.key.keysym.mod & KMOD_SHIFT) ? -1 : 1;
                break;
            case SDLK_RIGHT:
            case SDLK_DOWN:
                input.step = 1;
                break;
            case SDLK_LEFT:
            case SDLK_UP:
                input.step = -1;
                break;
            case SDLK_p:
                carrier->paused = !carrier->paused;
                break;
            default:
                break;
            }
        }
        /* Finish activation on release: opening a scene must not also skip
         * its first line, and launch input must not fire the player's guns. */
        if (event.type == SDL_KEYUP && carrier->armedKey != SDL_SCANCODE_UNKNOWN &&
            event.key.keysym.scancode == carrier->armedKey) {
            input.back = event.key.keysym.sym == SDLK_ESCAPE;
            input.activate = !input.back;
            carrier->armedKey = SDL_SCANCODE_UNKNOWN;
        }
    }
    SdlPauseSwcSound(!carrier->focused || carrier->paused);
    return input;
}

/* SDL replacement for CODE_01 dump_buffer_to_screen +0x06d4. Retaining
 * the complete picture allows palette transitions without rereading the
 * window backbuffer (undefined after SDL_RenderPresent). */
static int SwcPresentRoom(SwcCarrier *carrier, int brightness)
{
    if (SDL_SetRenderTarget(carrier->renderer, NULL) != 0 ||
        SDL_SetRenderDrawColor(carrier->renderer, 0, 0, 0, 255) != 0 ||
        SDL_RenderClear(carrier->renderer) != 0 ||
        SDL_RenderCopy(carrier->renderer, carrier->picture, NULL, NULL) != 0)
        return -1;
    if (brightness < 255 &&
        (SDL_SetTextureAlphaMod(carrier->dimPicture, 255 - brightness) != 0 ||
         SDL_RenderCopy(carrier->renderer, carrier->dimPicture, NULL, NULL) != 0))
        return -1;
    SDL_RenderPresent(carrier->renderer);
    carrier->pictureVisible = brightness != 0;
    return 0;
}

/* CODE_15 dark +0x03c2 uses the scene's actual clut ID+10. Retain its
 * per-entry colors rather than assuming a fade to black: the demo's dim
 * palettes keep the picture visible, at different levels in each room. */
static int SwcDarkRoom(SwcCarrier *carrier)
{
    SwcRoomPack *pack = &carrier->packs[carrier->picturePalette];
    Uint32 *pixels;
    Uint32 source;
    Uint32 replacement;
    SDL_Color color;
    size_t pixel;
    int entry;
    int result = -1;

    pixels = SDL_malloc(SWC_FRAME_WIDTH * SWC_FRAME_HEIGHT * sizeof(*pixels));
    if (pixels == NULL)
        return SDL_OutOfMemory();
    if (SDL_SetRenderTarget(carrier->renderer, carrier->picture) != 0 ||
        SDL_RenderReadPixels(carrier->renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
            pixels, SWC_FRAME_WIDTH * sizeof(*pixels)) != 0)
        goto done;
    for (pixel = 0; pixel < SWC_FRAME_WIDTH * SWC_FRAME_HEIGHT; pixel++) {
        source = pixels[pixel] & UINT32_C(0xffffff);
        for (entry = 0; entry < 256; entry++) {
            color = pack->colors[entry];
            if (source == ((Uint32)color.r << 16 | (Uint32)color.g << 8 | color.b))
                break;
        }
        if (entry < 256) {
            color = pack->dimColors[entry];
        } else {
            /* SDL-only route lines and keyboard focus colors have no
             * palette index; use the palette's white level for these. */
            color.r = (source >> 16) * pack->dimColors[0].r / 255;
            color.g = ((source >> 8) & 255) * pack->dimColors[0].g / 255;
            color.b = (source & 255) * pack->dimColors[0].b / 255;
        }
        replacement = UINT32_C(0xff000000) | (Uint32)color.r << 16 | (Uint32)color.g << 8 | color.b;
        pixels[pixel] = replacement;
    }
    result = SDL_UpdateTexture(carrier->dimPicture, NULL, pixels, SWC_FRAME_WIDTH * sizeof(*pixels));
done:
    SDL_SetRenderTarget(carrier->renderer, NULL);
    SDL_free(pixels);
    return result;
}

/* CODE_15 dark +0x03c2 switches to clut ID+10; ChangeSWPaletteClut
 * restores the scene palette. SDL interpolates between the two over ten
 * Mac ticks at room boundaries. This interpolation is a host transition;
 * the Mac routines change palette resources directly. */
static int SwcFadeRoom(SwcCarrier *carrier, int entering)
{
    SwcRoomInput input;
    Uint32 previous;
    Uint32 current;
    Uint32 elapsed = 0;
    int playing = carrier->focused && !carrier->paused;
    int brightness;

    if (!entering && !carrier->pictureVisible)
        return 0;
    if (SwcDarkRoom(carrier) != 0)
        return -1;
    previous = SDL_GetTicks();
    for (;;) {
        current = SDL_GetTicks();
        if (playing)
            elapsed += current - previous;
        previous = current;
        input = SwcPollCarrier(carrier);
        if (input.quit)
            return SWC_MOVIE_QUIT;
        playing = carrier->focused && !carrier->paused;
        if (input.back || (playing && (input.activate || input.clicked || input.step))) {
            carrier->pendingInput = input;
            elapsed = 167;
        }
        brightness = (int)(SDL_min(elapsed, 167U) * 255 / 167);
        if (SwcPresentRoom(carrier, entering ? brightness : 255 - brightness) != 0)
            return -1;
        if (elapsed >= 167)
            return 0;
        SDL_Delay(8);
    }
}

/* CODE_12 ShowSplash +0x4bb6: PICT/129 and INTRO.MooV, up to 1500 Mac
 * ticks. SDL keeps the same 25-second limit and the cinematic skip controls. */
int SdlShowSwcSplash(SDL_Window *window, SDL_Renderer *renderer, const char *resourceFork)
{
    SwcCarrier carrier = {0};
    SwcRoomInput input;
    SwcBuffer pixels = {0};
    SDL_Color colors[256];
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;
    Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 previous;
    Uint64 current;
    Uint64 elapsed = 0;
    int playing = 0;
    int result = SWC_MOVIE_ERROR;
    char failure[256];

    carrier.window = window;
    carrier.focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    SdlPauseSwcSound(1);
    SdlClearSwcPcm();
    if (SwcReadSplash(resourceFork, &pixels, colors) != 0)
        goto done;
    surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data, SWC_FRAME_WIDTH,
        SWC_FRAME_HEIGHT, 8, SWC_FRAME_WIDTH, SDL_PIXELFORMAT_INDEX8);
    if (surface == NULL || SDL_SetPaletteColors(surface->format->palette, colors, 0, 256) != 0)
        goto done;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL)
        goto done;
    SdlPlaySwcMusic("INTRO.MooV", 50);
    previous = SDL_GetPerformanceCounter();
    while (1) {
        current = SDL_GetPerformanceCounter();
        if (playing)
            elapsed += current - previous;
        previous = current;
        input = SwcPollCarrier(&carrier);
        playing = carrier.focused && !carrier.paused;
        if (input.quit || input.back || input.activate || input.clicked) {
            result = input.quit ? SWC_MOVIE_QUIT : SWC_MOVIE_SKIPPED;
            break;
        }
        SDL_SetWindowTitle(window, playing
            ? "Super Wing Commander - splash | Space/Enter/Esc/click: skip | P: pause"
            : "Super Wing Commander - splash (paused) | P: resume");
        if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
            SDL_RenderClear(renderer) != 0 || SDL_RenderCopy(renderer, texture, NULL, NULL) != 0)
            goto done;
        SDL_RenderPresent(renderer);
        if (elapsed >= 25 * frequency &&
            carrier.armedKey == SDL_SCANCODE_UNKNOWN && !carrier.mouseDown) {
            result = SWC_MOVIE_FINISHED;
            break;
        }
        SDL_Delay(20);
    }
done:
    SDL_strlcpy(failure, SDL_GetError(), sizeof(failure));
    SdlStopSwcMusic();
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_free(pixels.data);
    if (result == SWC_MOVIE_ERROR)
        SDL_SetError("SWC splash: %s", failure);
    return result;
}

/* CODE_07 intro_ready_attack +0x1906 and intro_ranger_dead +0x1adc.
 * Both draw TKHD/13 at (0,10), with eyes 11..20 and mouth 1..10.
 * The first also draws ST01/1 frame 13 at (50,50)/(250,50), second flipped.
 * Host 20 Hz presentation replaces Mac loop timing; speech marks use 60 Hz. */
int SdlPlaySwcIntroSpeech(SDL_Window *window, SDL_Renderer *renderer,
                           const char *resourceFork, int afterAttack)
{
    /* CODE_05 generate_new_3d_shard, expanded DATA/0 +0x2ede. */
    static const int shardFrames[15] = {20, 0, 4, 24, 28, 28, 20, 20, 24, 8, 8, 24, 28, 12, 12};
    SwcCarrier carrier = {0};
    SwcRoomInput input;
    enum SwcRoomPackId packId = afterAttack ? SWC_INTRO_AFTER_ATTACK : SWC_INTRO_BEFORE_ATTACK;
    SwcRoomPack *pack = &carrier.packs[packId];
    SwcRoomArt *art;
    SwcRoomArt *ship = NULL;
    SwcFrame shipFrame;
    FixedVector shard;
    SDL_Point stars[63] = {{0, 0}};
    int starFrames[63];
    SwcBuffer pcm = {0};
    SwcSpeechMark *marks = NULL;
    size_t markCount = 0;
    size_t mark = 0;
    SDL_Rect destination;
    Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 previous;
    Uint64 current;
    Uint64 elapsed;
    Uint64 duration;
    Uint32 speechTicks;
    Uint64 animationTick = 0;
    Uint64 nextAnimation = 0;
    Uint64 sequenceTime = 0;
    uint32_t frame;
    int shipX[2] = {50, 250};
    int shipY[2] = {50, 50};
    int shipStep = 0;
    int eyes = 0;
    int mouth = -1;
    int entry;
    int line;
    int index;
    int distance;
    int playing = 0;
    int result = SWC_MOVIE_ERROR;
    char relative[64];
    char path[PATH_MAX];
    char failure[256];

    carrier.window = window;
    carrier.renderer = renderer;
    carrier.resourceFork = resourceFork;
    carrier.focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    SdlPauseSwcSound(1);
    SdlClearSwcPcm();
    if (SwcGetRoomArt(&carrier, packId, "TKHD", 13) == NULL)
        goto done;
    if (SwcGetRoomArt(&carrier, packId, "PLNT", 1) == NULL)
        goto done;
    if (!afterAttack) {
        ship = SwcGetRoomArt(&carrier, packId, "ST01", 1);
        if (ship == NULL || SwcGetFramePtr(&ship->shape, 13, &shipFrame) != 0)
            goto done;
        ship->textures[13] = SdlCreateSwcTexture(renderer, &ship->shape, 13, pack->colors);
        if (ship->textures[13] == NULL)
            goto done;
    }
    for (line = afterAttack ? 10 : 3; line < (afterAttack ? 12 : 10); line++) {
        SdlPauseSwcSound(1);
        SDL_snprintf(relative, sizeof(relative), "AIFF/intro.%02d", line);
        if (!SdlResolvePath(relative, path, sizeof(path))) {
            SDL_SetError("Cannot resolve SWC intro speech %s", relative);
            goto done;
        }
        if (SwcReadSpeech(path, &pcm) != 0 ||
            SwcReadSpeechMarks(path, &marks, &markCount) != 0 ||
            SdlQueueSwcPcm(&pcm, 11025, AUDIO_S16MSB) != 0)
            goto done;
        duration = (Uint64)pcm.size * frequency / (11025 * 2);
        if (swcAudioDevice != 0)
            duration += 512 * frequency / 48000;
        SDL_free(pcm.data);
        memset(&pcm, 0, sizeof(pcm));
        mark = 0;
        mouth = -1;
        elapsed = 0;
        playing = 0;
        previous = SDL_GetPerformanceCounter();
        while (1) {
            current = SDL_GetPerformanceCounter();
            if (playing) {
                elapsed += current - previous;
                sequenceTime += current - previous;
            }
            previous = current;
            input = SwcPollCarrier(&carrier);
            playing = carrier.focused && !carrier.paused;
            if (input.quit || input.back || input.activate || input.clicked) {
                result = input.quit ? SWC_MOVIE_QUIT : SWC_MOVIE_SKIPPED;
                goto done;
            }
            SDL_SetWindowTitle(window, playing
                ? "Super Wing Commander - prologue | Space/Enter/Esc/click: skip | P: pause"
                : "Super Wing Commander - prologue (paused) | P: resume");
            speechTicks = swcAudioDevice != 0 ? SdlSwcPcmTicks() : (Uint32)(elapsed * 60 / frequency);
            while (mark < markCount && marks[mark].tick <= speechTicks)
                mouth = marks[mark++].frame;
            if (swcAudioDevice != 0 && !SdlSwcPcmPlaying())
                mouth = -1;
            /* CODE_07 randomeyes changes every tenth draw. The two ships
             * move every sixth draw and return toward their initial points
             * every third update, using the original exclusive RNG bounds. */
            if (sequenceTime >= nextAnimation) {
                animationTick++;
                nextAnimation = sequenceTime + frequency / 20;
                /* init_3d_shards(180), update_3d_shards(0): a stationary
                 * camera looking backward, with 63 shards. Out-of-view
                 * shards regenerate one draw in four. Their zero velocity
                 * leaves visible positions and sprite frames unchanged. */
                for (index = 0; index < (int)SDL_arraysize(stars); index++) {
                    if (animationTick != 1 &&
                        (stars[index].x != INT_MIN || RandomBelowOrEqual(3) != 0))
                        continue;
                    FillFixedVectorWithRandomComponents(4095, &shard);
                    starFrames[index] = shardFrames[RandomBelowOrEqual(14)];
                    shard.x = -shard.x;
                    shard.z = -shard.z;
                    distance = Vector_magnitude(&shard);
                    if (shard.z < 0xa00 || distance == 0 ||
                        (int64_t)shard.z * 256 / distance < 0x94) {
                        stars[index].x = INT_MIN;
                        continue;
                    }
                    stars[index].x = 160 + (int)(((int64_t)shard.x * 0xa000 / shard.z) >> 8);
                    stars[index].y = 120 + (int)(((int64_t)shard.y * 0xa000 / shard.z) >> 8);
                }
                if (animationTick % 10 == 0)
                    eyes = RandomBelowOrEqual(9);
                if (!afterAttack && animationTick % 6 == 0) {
                    for (index = 0; index < 2; index++) {
                        shipStep++;
                        shipX[index] += shipStep % 3 == 0
                            ? (shipX[index] > (index == 0 ? 50 : 250) ? -1 : 1)
                            : (RandomBelowOrEqual(1) ? 1 : -1);
                        shipY[index] += shipStep % 3 == 0
                            ? (shipY[index] > 50 ? -1 : 1)
                            : (RandomBelowOrEqual(1) ? 1 : -1);
                    }
                }
            }
            if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
                SDL_RenderClear(renderer) != 0)
                goto done;
            for (index = 0; index < (int)SDL_arraysize(stars); index++) {
                if (stars[index].x != INT_MIN)
                    SwcDrawRoomArt(&carrier, packId, "PLNT", 1, starFrames[index],
                                   stars[index].x, stars[index].y);
            }
            if (ship != NULL) {
                for (index = 0; index < 2; index++) {
                    destination = (SDL_Rect){shipX[index] - shipFrame.width / 2,
                        shipY[index] - shipFrame.height / 2, shipFrame.width, shipFrame.height};
                    if (SDL_RenderCopyEx(renderer, ship->textures[13], NULL, &destination,
                        0, NULL, index == 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL) != 0)
                        goto done;
                }
            }
            SwcDrawRoomArt(&carrier, packId, "TKHD", 13, 0, 0, 10);
            SwcDrawRoomArt(&carrier, packId, "TKHD", 13, 11 + eyes, 0, 10);
            if (mouth >= 0)
                SwcDrawRoomArt(&carrier, packId, "TKHD", 13, mouth + 1, 0, 10);
            if (carrier.drawResult != 0)
                goto done;
            SDL_RenderPresent(renderer);
            if (elapsed >= duration && !SdlSwcPcmPlaying() &&
                carrier.armedKey == SDL_SCANCODE_UNKNOWN && !carrier.mouseDown)
                break;
            SDL_Delay(20);
        }
        SdlClearSwcPcm();
        SDL_free(marks);
        marks = NULL;
    }
    result = SWC_MOVIE_FINISHED;
done:
    SDL_strlcpy(failure, SDL_GetError(), sizeof(failure));
    SdlClearSwcPcm();
    SDL_free(pcm.data);
    SDL_free(marks);
    for (entry = 0; entry < pack->count; entry++) {
        art = &pack->art[entry];
        if (art->textures != NULL) {
            for (frame = 0; frame < art->count; frame++)
                SDL_DestroyTexture(art->textures[frame]);
        }
        SDL_free(art->textures);
        SDL_free(art->bounds);
        SDL_free(art->shape.data);
    }
    SwcCMClose(&pack->cmf);
    if (result == SWC_MOVIE_ERROR)
        SDL_SetError("SWC intro dialogue: %s", failure);
    return result;
}

static uint32_t SwcSceneOffset(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
        (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static short SwcSceneWord(const uint8_t *bytes)
{
    return (short)(bytes[0] | (unsigned int)bytes[1] << 8);
}

static int SwcValidateSceneTests(const unsigned char *text, size_t count, enum SwcCarrierVisit visit)
{
    unsigned int command;
    unsigned int value;
    unsigned int first;
    int arguments;
    int argument;
    int digits;

    /* Validate before entering WC1's original, unbounded int_value parser.
     * Enyo 1 uses pilot status, kills and full/partial mission score tests;
     * the funeral adds command 30 (pilot lost on this mission).
     * Other campaign scripts need their own state/operand validation. */
    while ((command = *text++) != 0) {
        if (command < 1 || (command > 10 && command != 30 && command != 35 && command != 36) ||
            ((command == 35 || command == 36) && visit != SWC_CARRIER_DEBRIEFING))
            return SDL_SetError("Unsupported SWC carrier branch %u", command);
        arguments = (command >= 2 && command <= 5) || command == 30 ? 2 : 1;
        first = 0;
        for (argument = 0; argument < arguments; argument++) {
            value = 0;
            digits = 0;
            while (*text >= '0' && *text <= '9') {
                if (++digits > 5)
                    return SDL_SetError("Oversized SWC scene branch operand");
                value = value * 10 + *text++ - '0';
            }
            if (digits == 0 || value > INT16_MAX || (*text != ',' && *text != ')'))
                return SDL_SetError("Invalid SWC scene branch operand");
            text++;
            if (argument == 0)
                first = value;
        }
        if (value >= count || ((command == 4 || command == 5 || command == 30) && first >= 8))
            return SDL_SetError("SWC scene branch target or pilot is out of range");
    }
    return 0;
}

static int SwcValidateAnimation(const char *text, int face)
{
    int digits;

    while (*text != 0) {
        if (face && *text == 'R') {
            text++;
            continue;
        }
        if (face) {
            if (!((*text >= '0' && *text <= '9') || (*text >= 'A' && *text <= 'F')))
                return SDL_SetError("Invalid SWC face command");
        } else if (*text != '$' && !(*text >= 'a' && *text <= 'z')) {
            return SDL_SetError("Invalid SWC mouth command");
        }
        text++;
        digits = 0;
        while (*text >= '0' && *text <= '9') {
            if (++digits > 4)
                return SDL_SetError("Oversized SWC animation duration");
            text++;
        }
        if (face) {
            if (digits == 0 || *text != ',')
                return SDL_SetError("Unterminated SWC face command");
            text++;
        }
    }
    return 0;
}

static int SwcLoadRoomScene(SwcCarrier *carrier, int section, SwcRoomScene *scene)
{
    ConversationSceneRecord *record;
    const uint8_t *bytes;
    const unsigned char *end;
    const char *text;
    uint32_t offsets[15];
    short strings[4];
    size_t index;
    size_t length;
    size_t expanded;
    size_t sections = carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL ? 14 : 10;
    int field;

    /* LoadBriefingData +0x1556 and scene_ptr +0x39c6: unlike Mac sprites,
     * these directory entries and the five record shorts are little-endian. */
    if (carrier->script.size < sections * 4 || section < 0 ||
        (size_t)section + 1 >= sections || (section & 1))
        return SDL_SetError("Invalid SWC scene section");
    for (index = 0; index < sections; index++) {
        offsets[index] = SwcSceneOffset(carrier->script.data + index * 4);
        if (offsets[index] > carrier->script.size || offsets[index] < sections * 4 ||
            (index > 0 && offsets[index] < offsets[index - 1]))
            return SDL_SetError("Invalid SWC briefing directory");
    }
    offsets[sections] = (uint32_t)carrier->script.size;
    length = offsets[section + 1] - offsets[section];
    scene->count = length / 13;
    scene->text = carrier->script.data + offsets[section + 1];
    scene->textSize = offsets[section + 2] - offsets[section + 1];
    if (offsets[0] != sections * 4 || length % 13 != 0 || scene->count == 0 ||
        scene->count > SDL_arraysize(scene->records))
        return SDL_SetError("Invalid SWC scene record count");
    for (index = 0; index < scene->count; index++) {
        bytes = carrier->script.data + offsets[section] + index * 13;
        record = &scene->records[index];
        record->shot = (signed char)bytes[0];
        record->textColour = (signed char)bytes[1];
        record->talker = (signed char)bytes[2];
        record->duration = SwcSceneWord(bytes + 3);
        record->testsOffset = strings[0] = SwcSceneWord(bytes + 5);
        record->textOffset = strings[1] = SwcSceneWord(bytes + 7);
        record->mouthAnimationOffset = strings[2] = SwcSceneWord(bytes + 9);
        record->faceAnimationOffset = strings[3] = SwcSceneWord(bytes + 11);
        for (field = 0; field < 4; field++) {
            if (strings[field] < 0 || (size_t)strings[field] >= scene->textSize)
                return SDL_SetError("SWC scene string offset is out of range");
            text = (const char *)scene->text + strings[field];
            end = memchr(text, 0, scene->textSize - strings[field]);
            if (end == NULL || end - (const unsigned char *)text > 255)
                return SDL_SetError("Unterminated or oversized SWC scene string");
        }
        if (SwcValidateSceneTests(scene->text + record->testsOffset, scene->count, carrier->visit) != 0 ||
            SwcValidateAnimation((char *)scene->text + record->mouthAnimationOffset, 0) != 0 ||
            SwcValidateAnimation((char *)scene->text + record->faceAnimationOffset, 1) != 0)
            return -1;
        text = (const char *)scene->text + record->textOffset;
        expanded = 0;
        while (*text != 0) {
            if (*text++ == '$') {
                /* Bound WC1's AddPCName output before its fixed scratch buffer. */
                if (*text == 'C')
                    expanded += strlen(stCampaignState.currentPilot->callsign);
                else if (*text == 'N')
                    expanded += strlen(stCampaignState.currentPilot->name);
                else if (*text == 'R') {
                    if ((unsigned int)stCampaignState.currentPilot->rank >= SDL_arraysize(apszPilotRankNames))
                        return SDL_SetError("Invalid SWC pilot rank in scene substitution");
                    expanded += strlen(apszPilotRankNames[stCampaignState.currentPilot->rank]);
                } else if (*text == 'K' || *text == 'L')
                    expanded += 6;
                else
                    return SDL_SetError("Unsupported SWC carrier text substitution");
                text++;
            } else {
                expanded++;
            }
        }
        if (expanded >= 256)
            return SDL_SetError("Expanded SWC subtitle is too long");
    }
    if (scene->records[scene->count - 1].shot != -2)
        return SDL_SetError("SWC scene has no end command");
    return 0;
}

static int SwcSceneAnimationFrame(const short *commands, Uint32 ticks, int eyes)
{
    const short *cursor = commands;
    const short *repeat = NULL;
    Uint32 period = 0;
    Uint32 prefix = 0;
    Uint32 duration;

    while (*cursor != -1) {
        if (*cursor == -2) {
            repeat = cursor + 2;
            prefix = period;
        } else {
            period += (Uint32)(eyes ? SDL_max(24, cursor[1]) + 1 : SDL_max(1, cursor[1]));
        }
        cursor += 2;
    }
    if (ticks >= period) {
        if (repeat == NULL || period == prefix)
            return -1;
        ticks = prefix + (ticks - prefix) % (period - prefix);
    }
    for (cursor = commands; *cursor != -1; cursor += 2) {
        if (*cursor == -2)
            continue;
        duration = (Uint32)(eyes ? SDL_max(24, cursor[1]) + 1 : SDL_max(1, cursor[1]));
        if (ticks < duration)
            return *cursor == 10 ? -1 : *cursor;
        ticks -= duration;
    }
    return -1;
}

static int SwcStartSpeech(SwcCarrier *carrier, int section, int line, Uint32 *duration)
{
    SwcBuffer pcm = {0};
    SDL_RWops *file;
    char relative[96];
    char path[PATH_MAX];
    Uint32 speechDuration;
    int mission = carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL ? 0 : 4;
    int result = 0;

    SdlClearSwcPcm();
    SDL_free(carrier->marks);
    carrier->marks = NULL;
    carrier->markCount = 0;
    carrier->haveSpeech = 0;
    /* SceneDirector +0x2cb6; DATA/0 +0x4322/+0x4326:
     * "%s%d:%2.2d%2.2d%2.2d%2.2d.AIF", "camp.". The archive groups files
     * by mission: 04 for Enyo 1, 00 for funerals. Keep original record indices.
     * The fresh demo pilot has rank 0, including the $R speech variant. */
    SDL_snprintf(relative, sizeof(relative), "AIFF/camp.0/%02d/%02d%02d%02d00.AIF",
                 mission, mission, section, line);
    if (!SdlResolvePath(relative, path, sizeof(path)))
        return SDL_SetError("Cannot resolve SWC speech %s", relative);
    file = SDL_RWFromFile(path, "rb");
    if (file == NULL) {
        SDL_ClearError();
        return 0; /* Optional speech: subtitles retain a readable duration. */
    }
    SDL_RWclose(file);
    if (SwcReadSpeech(path, &pcm) != 0)
        return -1;
    if (SwcReadSpeechMarks(path, &carrier->marks, &carrier->markCount) != 0) {
        SDL_free(pcm.data);
        return -1;
    }
    carrier->haveSpeech = 1;
    speechDuration = (Uint32)((uint64_t)pcm.size * 1000 / (11025 * 2)) + 250;
    if (swcAudioDevice != 0) {
        result = SdlQueueSwcPcm(&pcm, 11025, AUDIO_S16MSB);
        *duration = speechDuration + 512 * 1000 / 48000;
    } else {
        *duration = SDL_max(*duration, speechDuration);
    }
    SDL_free(pcm.data);
    return result;
}

static void SwcDrawBriefingMap(SwcCarrier *carrier, int objective, Uint32 ticks, int reveal)
{
    SDL_Renderer *renderer = carrier->renderer;
    SDL_Rect panel = {44, 21, 185, 210};
    SDL_Rect clip;
    SDL_Rect marker;
    SDL_Color ink = {100, 245, 165, 255};
    short x;
    short y;
    short nextX;
    short nextY;
    int index;
    int next;
    char label[12];

    SwcDrawRoomArt(carrier, SWC_BRIEFING_MAP, "BRFG", 3, 0, 0, 0);
    if (carrier->drawResult != 0)
        return;
    SetScale();
    /* CODE_14 BriefingMap_ShowGrid +0x0cfe expands the grid two pixels
     * per edge per update. DATA/0 +0x272 gives its origin (40,20).
     * The sprite's own (4,1) offset is applied by SwcDrawRoomArt. */
    clip = panel;
    if (reveal && ticks < 53) {
        clip.y = 124 - (int)ticks * 2;
        clip.h = 2 + (int)ticks * 4;
        SDL_IntersectRect(&clip, &panel, &clip);
    }
    if (SDL_RenderSetClipRect(renderer, &clip) != 0) {
        carrier->drawResult = -1;
        return;
    }
    SwcDrawRoomArt(carrier, SWC_BRIEFING_MAP, "BRFG", 27, 0, 40, 20);
    if (SDL_RenderSetClipRect(renderer, NULL) != 0) {
        carrier->drawResult = -1;
        return;
    }
    if (reveal && ticks < 53)
        return;
    if (SDL_SetRenderDrawColor(renderer, 75, 210, 140, 255) != 0) {
        carrier->drawResult = -1;
        return;
    }
    /* Share WC1's mission objective list and coordinate transform. Labels
     * and route lines remain host drawing; the original grid is preserved. */
    for (index = 0; index < cMissionObjectiveCount; index++) {
        nav_getxy(&x, &y, aMissionObjectives[index].position.x, aMissionObjectives[index].position.z);
        next = (index + 1) % cMissionObjectiveCount;
        nav_getxy(&nextX, &nextY, aMissionObjectives[next].position.x, aMissionObjectives[next].position.z);
        marker = (SDL_Rect){54 + x - 2, 45 + y - 2, 4, 4};
        SDL_snprintf(label, sizeof(label), "%d", index + 1);
        if (SDL_RenderDrawLine(renderer, 54 + x, 45 + y, 54 + nextX, 45 + nextY) != 0 ||
            ((index != objective || ticks >= 30 || ticks % 6 < 3) &&
             SDL_RenderDrawRect(renderer, &marker) != 0) ||
            SdlDrawSwcText(renderer, carrier->font, 60 + x, 45 + y, ink, label) != 0) {
            carrier->drawResult = -1;
            return;
        }
    }
    SwcDrawRoomArt(carrier, SWC_BRIEFING_MAP, "BRFG", 27, 1, 40, 20);
}

static void SwcDrawScene(SwcCarrier *carrier, int shot, int textColour, int talker,
                          int mouth, int face, Uint32 elapsed)
{
    enum SwcRoomPackId pack;
    const char *background;
    uint32_t backgroundId;
    int backgroundFrame;

    if (shot >= 20 && shot <= 30) {
        background = "RCRM";
        backgroundId = 2;
        if (shot == 30) {
            pack = SWC_SHOTGLASS;
            backgroundFrame = 0;
        } else if (shot == 25 || shot == 26) {
            pack = SWC_BAR_HEADS;
            backgroundFrame = talker == 4 ? 2 : 1;
        } else if (carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL && (shot == 20 || shot == 29)) {
            /* SceneDirector +0x29b0: BRFG/20 frame depends on text colour. */
            pack = SWC_FUNERAL;
            background = "BRFG";
            backgroundId = 20;
            backgroundFrame = textColour == 0 ? 0 : 1;
        } else if (carrier->visit == SWC_CARRIER_DEBRIEFING && (shot == 20 || shot == 21 || shot == 29)) {
            /* SceneDirector +0x2960/+0x2988: commander talker 10 uses
             * BRFG/15 frame 1; player/wingman talker 11 uses frame 0. */
            pack = SWC_DEBRIEFING_HEADS;
            background = "BRFG";
            backgroundId = 15;
            backgroundFrame = talker == 10 ? 1 : 0;
        } else if (shot == 20) {
            pack = SWC_COMMANDER;
            background = "BRFG";
            backgroundId = 6;
            backgroundFrame = 0;
        } else if (shot == 21 || shot == 29) {
            pack = SWC_WINGMEN;
            background = "BRFG";
            backgroundId = 6;
            backgroundFrame = textColour == 9 ? 2 : 1;
        } else {
            carrier->drawResult = SDL_SetError("Unsupported SWC carrier portrait %d", shot);
            return;
        }
        SwcDrawRoomArt(carrier, pack, background, backgroundId, backgroundFrame, 0, 0);
        /* CloseTalk +0x3292 uses base frame 21 in debriefing, otherwise 0. */
        SwcDrawRoomArt(carrier, pack, "TKHD", (uint32_t)(shot - 19),
                        carrier->visit == SWC_CARRIER_DEBRIEFING ? 21 : 0, 0, 0);
        if (face >= 0 && face < 10)
            SwcDrawRoomArt(carrier, pack, "TKHD", (uint32_t)(shot - 19), face + 11, 0, 0);
        if (mouth >= 0)
            SwcDrawRoomArt(carrier, pack, "TKHD", (uint32_t)(shot - 19), mouth + 1, 0, 0);
    } else if (shot == 0 || shot == 1) {
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 1, 0, 0, 0);
        /* DrawBriefingLongShot +0x1e54; DATA/0 +0x4306/+0x430a. */
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 2, SDL_max(0, mouth), 90, 90);
    } else if (shot == 2) {
        /* DrawPodiumShot +0x1e02: background 1; body at (25,50). */
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 3, 1, 0, 0);
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 4, mouth + 1, 25, 50);
    } else if (shot == 3 && elapsed < 800) {
        /* PanToScreen +0x2288: commander turns through BRFG/4 11..16
         * at SetPace(7), i.e. the first tick strictly after seven ticks. */
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 3, 1, 0, 0);
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 4,
            11 + (int)((uint64_t)elapsed * 60 / 8000), 25, 50);
    } else if (shot == 3 || shot == 4) {
        SwcDrawBriefingMap(carrier, talker < 0 ? -talker : talker,
            (Uint32)((uint64_t)(elapsed - (shot == 3 ? 800 : 0)) * 30 / 1000), shot == 3);
    } else if (shot == 5) {
        /* CODE_03 Dismissed +0x1eba: this is a separate reverse angle,
         * not the ordinary briefing long shot. */
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 29, 0, 0, 0);
        if (mouth >= 0)
            SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 31, mouth, 240, 0);
        SwcDrawRoomArt(carrier, SWC_BRIEFING, "BRFG", 32, 0, 10, 85);
    } else if (shot == 10 || shot == 11) {
        /* MountGraphics +0x23b8 and DrawDebriefingLongShot +0x1b8c. */
        SwcDrawRoomArt(carrier, SWC_DEBRIEFING, "BRFG", 11, 0, 0, 0);
        SwcDrawRoomArt(carrier, SWC_DEBRIEFING, "BRFG", 12, 0, 40, 45);
        SwcDrawRoomArt(carrier, SWC_DEBRIEFING, "BRFG", 14,
                        shot == 10 ? 0 : mouth + 1, 140, 60);
        if (nDebriefingPersonality >= 0 && nDebriefingPersonality < 8 &&
            stCampaignState.personalityDeathMission[nDebriefingPersonality] == 0)
            SwcDrawRoomArt(carrier, SWC_DEBRIEFING, "BRFG", 13,
                            nDebriefingPersonality, 195, 40);
    } else if (shot == 17 && carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL) {
        /* CODE_03 DrawFuneralLongShot +0x1af8 and MountGraphics +0x2484. */
        SwcDrawRoomArt(carrier, SWC_FUNERAL, "BRFG", 19, 0, 0, 0);
    } else {
        carrier->drawResult = SDL_SetError("Unsupported SWC carrier shot %d", shot);
    }
}

static int SwcPlayRoomScene(SwcCarrier *carrier, int section)
{
    SwcRoomScene scene = {0};
    ConversationSceneRecord *record;
    ConversationSceneRecord *selected;
    SwcRoomInput input;
    short mouth[513];
    short face[513];
    char text[256];
    Uint32 previous;
    Uint32 current;
    Uint32 elapsed;
    Uint32 duration;
    Uint32 speechTicks;
    Uint32 leadIn;
    size_t mark;
    size_t index = 0;
    size_t commands = 0;
    int shot = 0;
    int talker = -1;
    int colour = 0;
    int wasPlaying;
    int mouthFrame;
    int firstPicture = 1;
    int speechStarted;
    int result = -1;

    if (SwcLoadRoomScene(carrier, section, &scene) != 0)
        return -1;
    result = SwcFadeRoom(carrier, 0);
    if (result != 0)
        return result;
    result = -1;
    SDL_SetCursor(SDL_GetDefaultCursor());
    carrier->armedKey = SDL_SCANCODE_UNKNOWN;
    carrier->mouseDown = 0;
    carrier->paused = 0;
    carrier->focused = (SDL_GetWindowFlags(carrier->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    SdlPauseSwcSound(!carrier->focused);
    if (carrier->visit == SWC_CARRIER_PREFLIGHT && section == 0)
        SdlPlaySwcMusic("BRIEFIN0.MooV", 50);
    SDL_SetWindowTitle(carrier->window, carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL
        ? "Super Wing Commander - funeral | Click/Space/Enter: next | Esc: finish | P: pause"
        : section == 0
        ? "Super Wing Commander - briefing | Click/Space/Enter: next | Esc: launch | P: pause"
        : section == 2
        ? "Super Wing Commander - debriefing | Click/Space/Enter: next | Esc: finish | P: pause"
        : "Super Wing Commander - conversation | Click/Space/Enter: next | Esc: bar | P: pause");
    while (index < scene.count) {
        record = &scene.records[index];
        if (record->shot == -2) {
            result = 0;
            break;
        }
        if (++commands > scene.count * 4) {
            SDL_SetError("SWC carrier scene branch loop");
            break;
        }
        selected = ParseTests(record, scene.records, scene.text);
        if (selected != record) {
            index = (size_t)(selected - scene.records);
            continue;
        }
        if (record->shot != -1)
            shot = record->shot & 0x3f;
        if (record->talker != -2)
            talker = record->talker;
        if (record->textColour != -1)
            colour = record->textColour;
        ParseMouthAnimation((char *)scene.text + record->mouthAnimationOffset, mouth);
        ParseFaceAnimation((char *)scene.text + record->faceAnimationOffset, face);
        SDL_strlcpy(text, AddPCName((char *)scene.text + record->textOffset), sizeof(text));
        if (text[0] == 0) {
            index++;
            continue;
        }
        duration = (Uint32)SDL_max(2000, (int)strlen(text) * 50 + 1000);
        leadIn = shot == 3 ? 800 : 0;
        speechStarted = 0;
        mark = 0;
        mouthFrame = -1;
        if (firstPicture) {
            if (SDL_SetRenderTarget(carrier->renderer, carrier->picture) != 0 ||
                SDL_SetRenderDrawColor(carrier->renderer, 0, 0, 0, 255) != 0 ||
                SDL_RenderClear(carrier->renderer) != 0)
                goto done;
            SwcDrawScene(carrier, shot, colour, talker, -1, -1, 0);
            if (carrier->drawResult != 0 ||
                SdlDrawSwcRoomSubtitle(carrier->renderer, &carrier->roomFont, text) != 0)
                goto done;
            result = SwcFadeRoom(carrier, 1);
            if (result != 0)
                goto done;
            result = -1;
            firstPicture = 0;
        }
        previous = SDL_GetTicks();
        elapsed = 0;
        wasPlaying = 0;
        for (;;) {
            current = SDL_GetTicks();
            if (wasPlaying)
                elapsed += current - previous;
            previous = current;
            input = SwcPollCarrier(carrier);
            if (input.quit) {
                result = SWC_MOVIE_QUIT;
                goto done;
            }
            if (input.back) {
                result = carrier->visit >= SWC_CARRIER_PLAYER_FUNERAL ? SWC_MOVIE_SKIPPED : 0;
                goto done;
            }
            wasPlaying = carrier->focused && !carrier->paused;
            if (wasPlaying && !speechStarted && elapsed >= leadIn) {
                if (SwcStartSpeech(carrier, section, (int)index, &duration) != 0)
                    goto done;
                if (shot == 3)
                    duration = SDL_max(duration, 1767U);
                speechStarted = 1;
            }
            if (wasPlaying && (input.activate || input.clicked ||
                (speechStarted && elapsed >= duration + leadIn && !SdlSwcPcmPlaying() &&
                 carrier->armedKey == SDL_SCANCODE_UNKNOWN && !carrier->mouseDown)))
                break;
            if (SDL_SetRenderTarget(carrier->renderer, carrier->picture) != 0 ||
                SDL_SetRenderDrawColor(carrier->renderer, 0, 0, 0, 255) != 0 ||
                SDL_RenderClear(carrier->renderer) != 0)
                goto done;
            if (speechStarted) {
                speechTicks = swcAudioDevice != 0 && carrier->haveSpeech ? SdlSwcPcmTicks() :
                    (Uint32)((uint64_t)(elapsed - leadIn) * 60 / 1000);
                if (carrier->haveSpeech) {
                    while (mark < carrier->markCount && carrier->marks[mark].tick <= speechTicks)
                        mouthFrame = carrier->marks[mark++].frame;
                    if (swcAudioDevice != 0 && !SdlSwcPcmPlaying())
                        mouthFrame = -1;
                } else {
                    mouthFrame = SwcSceneAnimationFrame(mouth, (elapsed - leadIn) / 50, 0);
                }
            }
            /* SceneDirector +0x2e34 selects CloseLook for a listening
             * portrait: its mouth must stay still during another voice. */
            if (shot >= 20 && shot <= 30 && shot - 20 != colour)
                mouthFrame = -1;
            SwcDrawScene(carrier, shot, colour, talker, mouthFrame,
                SwcSceneAnimationFrame(face, (Uint32)((uint64_t)elapsed * 30 / 1000), 1), elapsed);
            if (carrier->drawResult != 0 || SdlDrawSwcRoomSubtitle(carrier->renderer, &carrier->roomFont,
                carrier->paused ? "Paused - P to resume" : elapsed < leadIn ? "" : text) != 0 ||
                SwcPresentRoom(carrier, 255) != 0)
                goto done;
            SDL_Delay(16);
        }
        SdlClearSwcPcm();
        if (shot == 3)
            shot = 4;
        else if (shot == 5)
            shot = 1;
        index++;
    }
done:
    SdlClearSwcPcm();
    SDL_free(carrier->marks);
    carrier->marks = NULL;
    carrier->markCount = 0;
    SDL_SetRenderTarget(carrier->renderer, NULL);
    carrier->paused = 0;
    if (result == 0 || result == SWC_MOVIE_SKIPPED) {
        int fadeResult = SwcFadeRoom(carrier, 0);
        if (fadeResult != 0)
            result = fadeResult;
    }
    return result;
}

static int SwcBarIdleFrame(int character, Uint32 ticks, int hovered)
{
    /* Original byte programs: DATA/0 +0x27ae, +0x2816, +0x27da. A high
     * bit encodes a delay, 255 repeats. WC1's idle frame sets differ. */
    static const uint8_t shotglass[] = {0, 1, 0, 1, 0, 1, 0, 1, 2, 1, 133, 2, 2, 1, 255};
    static const uint8_t paladin[] = {1, 2, 3, 178, 2, 1, 2, 3, 148, 2, 1, 2, 3, 255};
    static const uint8_t angel[] = {2, 138, 3, 4, 148, 3, 4, 3, 4, 138, 0, 1, 255};
    static const uint8_t *const programs[] = {shotglass, paladin, angel};
    /* Hover programs: DATA/0 +0x2782/+0x2788/+0x278a. */
    static const int hoverFrames[] = {3, 4, 5};
    const uint8_t *cursor;
    Uint32 period = 0;
    Uint32 duration;
    int frame = 0;

    if (hovered)
        return hoverFrames[character];
    for (cursor = programs[character]; *cursor != 255; cursor++)
        period += (*cursor & 128) ? (*cursor & 127) + 1 : 1;
    period++; /* The repeat opcode itself consumes an update. */
    ticks %= period;
    for (cursor = programs[character]; *cursor != 255; cursor++) {
        duration = (*cursor & 128) ? (*cursor & 127) + 1 : 1;
        if (!(*cursor & 128))
            frame = *cursor;
        if (ticks < duration)
            return frame;
        ticks -= duration;
    }
    return frame;
}

static void SwcDrawRankings(SwcCarrier *carrier)
{
    /* Same ranking rule as WC1 ShowChalkBoard, also present in CODE_07
     * +0x111a. Read the shared pilot records; only drawing is host-specific. */
    static const int rowY[9] = {54, 76, 97, 117, 136, 157, 177, 197, 215};
    static const char *const ranks[5] = {"2LT", "1LT", "CPT", "MAJ", "LTC"};
    SDL_Color ink = {215, 220, 199, 255};
    int order[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int index;
    int other;
    int swap;
    PilotRecord *pilot;
    char text[64];

    for (index = 0; index < 9; index++) {
        for (other = index + 1; other < 9; other++) {
            if (aPilotRecords[order[index]].kills * 1000 - aPilotRecords[order[index]].missions + 1 <
                aPilotRecords[order[other]].kills * 1000 - aPilotRecords[order[other]].missions + 1) {
                swap = order[index];
                order[index] = order[other];
                order[other] = swap;
            }
        }
    }
    SwcDrawRoomArt(carrier, SWC_RECROOM, "RCRM", 3, 0, 0, 0);
    if (carrier->drawResult != 0)
        return;
    carrier->drawResult = SdlDrawSwcText(carrier->renderer, carrier->font, 46, 34, ink, "PILOT");
    if (carrier->drawResult == 0)
        carrier->drawResult = SdlDrawSwcText(carrier->renderer, carrier->font, 217, 34, ink, "M");
    if (carrier->drawResult == 0)
        carrier->drawResult = SdlDrawSwcText(carrier->renderer, carrier->font, 252, 34, ink, "K");
    for (index = 0; index < 9 && carrier->drawResult == 0; index++) {
        pilot = &aPilotRecords[order[index]];
        SDL_snprintf(text, sizeof(text), "%s %-14s", ranks[SDL_clamp(pilot->rank, 0, 4)], pilot->name);
        carrier->drawResult = SdlDrawSwcText(carrier->renderer, carrier->font, 46,
                                             rowY[index], ink, text);
        if (order[index] != 8 && stCampaignState.personalityDeathMission[order[index]] != 0)
            SDL_strlcpy(text, "KIA", sizeof(text));
        else
            SDL_snprintf(text, sizeof(text), "%3d    %3d", pilot->missions, pilot->kills);
        if (carrier->drawResult == 0)
            carrier->drawResult = SdlDrawSwcText(carrier->renderer, carrier->font, 212,
                                                 rowY[index], ink, text);
    }
}

static void SwcDrawMedalCase(SwcCarrier *carrier)
{
    /* DrawMedals CODE_03 +0x5a9c; DATA/0 +0x42f2. */
    static const int medalFrames[5] = {13, 16, 19, 22, 23};
    int index;
    int count;

    SwcDrawRoomArt(carrier, SWC_MEDALS, "RCRM", 46, 0, 0, 0);
    for (index = 0; index < 12; index++) {
        if (stCampaignState.badges[index] != 0)
            SwcDrawRoomArt(carrier, SWC_MEDALS, "RCRM", 46, index + 1, -109, -77);
    }
    for (index = 0; index < 5; index++) {
        for (count = 0; count < SDL_min(3, stCampaignState.medals[index]); count++)
            SwcDrawRoomArt(carrier, SWC_MEDALS, "RCRM", 46, medalFrames[index] + count, -109, -77);
    }
}

int SdlRunSwcCarrier(SDL_Window *window, SDL_Renderer *renderer,
                      const char *missionPath, const char *resourceFork, enum SwcCarrierVisit visit)
{
    SwcCarrier carrier = {0};
    SwcCmf data = {0};
    SwcBuffer roster = {0};
    SwcBuffer campaign = {0};
    unsigned char campaignData[90] = {0};
    unsigned char *savedCampaignData = pMissionCampaignData;
    SwcRoomInput input;
    SwcRoomArt *art;
    SDL_Cursor *savedCursor = SDL_GetCursor();
    SDL_Cursor *hand = NULL;
    const TitleMenuRegion *regions;
    const char *const *labels;
    const char *notice = NULL;
    SDL_Rect highlight;
    Uint32 previous;
    Uint32 current;
    Uint32 elapsed = 0;
    char failure[256];
    int room = 0; /* 0: bar, 1: barracks, 2: ranking, 3: medals. */
    int previousRoom;
    int fadeIn = 1;
    int simulatorFrame = 0;
    int wallFrame = 0;
    int bunkHover[3] = {-1, -1, -1};
    Uint32 animationTick = 0;
    Uint32 tick;
    Uint32 steps;
    int hover;
    int selected = -1;
    int keyboard = 0;
    int count;
    int pack;
    int entry;
    int sceneResult;
    int oldCursorVisible = SDL_ShowCursor(SDL_QUERY);
    uint32_t frame;
    int result = -1;

    carrier.window = window;
    if (savedCursor == NULL)
        savedCursor = SDL_GetDefaultCursor();
    carrier.renderer = renderer;
    carrier.resourceFork = resourceFork;
    carrier.visit = visit;
    carrier.focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    SdlPauseSwcSound(!carrier.focused);
    carrier.mouseX = carrier.mouseY = -1;
    if (visit < SWC_CARRIER_PREFLIGHT || visit > SWC_CARRIER_WINGMAN_FUNERAL) {
        SDL_SetError("Unsupported SWC carrier visit");
        goto done;
    }
    if (SwcCMOpen(missionPath, &data) != 0 ||
        SwcCMGetChunk(&data, "BRF0", visit >= SWC_CARRIER_PLAYER_FUNERAL ? 1 : 5,
                      &carrier.script) != 0)
        goto done;
    /* RecRoom indexes CMP0/3 by (series-1)*8 + mission*2. This host is
     * deliberately Enyo 1 only: validate its Paladin/Angel roster. */
    if (visit < SWC_CARRIER_PLAYER_FUNERAL) {
        if (SwcCMGetChunk(&data, "CMP0", 3, &roster) != 0)
            goto done;
        if (roster.size < 2 || roster.data[0] != 5 || roster.data[1] != 4) {
            SDL_SetError("Unsupported SWC carrier roster (expected Enyo 1)");
            goto done;
        }
    }
    carrier.font = SdlCreateSwcFont(renderer);
    if (carrier.font == NULL)
        goto done;
    if (SdlCreateSwcTextFont(renderer, &carrier.roomFont, 14) != 0)
        goto done;
    carrier.picture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET, SWC_FRAME_WIDTH, SWC_FRAME_HEIGHT);
    if (carrier.picture == NULL)
        goto done;
    carrier.dimPicture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SWC_FRAME_WIDTH, SWC_FRAME_HEIGHT);
    if (carrier.dimPicture == NULL ||
        SDL_SetTextureBlendMode(carrier.dimPicture, SDL_BLENDMODE_BLEND) != 0)
        goto done;
    hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    SDL_ShowCursor(SDL_ENABLE);
    SDL_SetWindowMouseGrab(window, SDL_FALSE);
    if (visit >= SWC_CARRIER_PLAYER_FUNERAL) {
        /* CODE_03 funeral_sequence +0x4a08 interleaves three movies with
         * BRF0/1: Enyo player's eulogy/farewell 2/0, Spirit's 12/10. */
        if (stCampaignState.currentSeries != 1 || stCampaignState.currentMission != 0 ||
            stCampaignState.currentPilot->rank != 0) {
            SDL_SetError("Unsupported SWC funeral (expected fresh Enyo 1 pilot)");
            goto done;
        }
        SdlPlaySwcMusic("FUNERAL.MooV", 50);
        result = SdlPlaySwcFuneralMovies(window, renderer, 0);
        if (result != SWC_MOVIE_FINISHED)
            goto done;
        result = SwcPlayRoomScene(&carrier, visit == SWC_CARRIER_PLAYER_FUNERAL ? 2 : 12);
        if (result != SWC_MOVIE_FINISHED)
            goto done;
        result = SdlPlaySwcFuneralMovies(window, renderer, 1);
        if (result != SWC_MOVIE_FINISHED)
            goto done;
        result = SwcPlayRoomScene(&carrier, visit == SWC_CARRIER_PLAYER_FUNERAL ? 0 : 10);
        if (result != SWC_MOVIE_FINISHED || visit == SWC_CARRIER_WINGMAN_FUNERAL)
            goto done;
        SdlStopSwcMusic();
        /* CODE_03 the_end +0x4b1e retains a terminal screen until input.
         * SDL draws a static end card; the original background animation is pending. */
        SDL_SetWindowTitle(window, "Super Wing Commander - THE END | Click/Space/Enter/Esc: finish");
        carrier.armedKey = SDL_SCANCODE_UNKNOWN;
        carrier.mouseDown = 0;
        result = SWC_MOVIE_ERROR;
        while (1) {
            input = SwcPollCarrier(&carrier);
            if (input.quit || input.back || input.activate || input.clicked) {
                result = input.quit ? SWC_MOVIE_QUIT : SWC_MOVIE_FINISHED;
                break;
            }
            if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
                SDL_RenderClear(renderer) != 0 ||
                SdlDrawSwcText(renderer, carrier.font, 142, 110,
                    (SDL_Color){245, 245, 230, 255}, "THE END") != 0 ||
                SdlDrawSwcSubtitle(renderer, carrier.font, "Click or press Enter to finish.") != 0)
                break;
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }
        goto done;
    }
    if (visit == SWC_CARRIER_DEBRIEFING) {
        if (SwcCMGetChunk(&data, "CMP0", 2, &campaign) != 0)
            goto done;
        if (stCampaignState.currentSeries != 1 || stCampaignState.currentMission != 0 ||
            campaign.size < 128 || campaign.data[0] != 0 || campaign.data[1] != 0 ||
            campaign.data[2] != 0 || campaign.data[3] != 0) {
            SDL_SetError("Unsupported SWC debriefing campaign (expected Enyo 1/Spirit)");
            goto done;
        }
        /* CODE_09 FullMissionScore +0x17e2 and PlayersMissionScore +0x1840
         * read 16 signed weights at CMP0/2 +(series-1)*128+mission*24+40.
         * Supply WC1's corresponding first-mission score window so its
         * ParseTests cases 35/36 run unchanged. This is not a campaign loader. */
        memcpy(campaignData + 14, campaign.data + 40, 16);
        pMissionCampaignData = campaignData;
        nDebriefingPersonality = 0;
        /* CODE_03 DeBriefing +0x13e4: the zero-score cue is a distinct
         * filename (DEBRIEF1), absent from the supplied demo. */
        count = FullMissionScore();
        SdlPlaySwcMusic(count == 0 ? "DEBRIEF1.MooV" :
            PlayersMissionScore() * 100 / count > 70 ? "DEBRIEFI.MooV" : "DEBRIEF0.MooV", 50);
        result = SwcPlayRoomScene(&carrier, 2);
        goto done;
    }
    previous = SDL_GetTicks();
    while (1) {
        SdlPlaySwcMusic(room == 0 || room == 2 ? "RECROOM.MooV" : "BARRACKS.MooV", 50);
        current = SDL_GetTicks();
        if (carrier.focused && !carrier.paused)
            elapsed += current - previous;
        previous = current;
        input = SwcPollCarrier(&carrier);
        if (input.quit) {
            result = SWC_MOVIE_QUIT;
            break;
        }
        if (input.back)
            carrier.paused = 0;
        if (!carrier.focused || carrier.paused) {
            SDL_Delay(16);
            continue;
        }
        previousRoom = room;
        if (input.quit || (input.back && room == 0 && notice == NULL)) {
            result = SWC_MOVIE_QUIT;
            break;
        }
        if (notice != NULL) {
            if (input.back || input.activate || input.clicked)
                notice = NULL;
        } else if (room >= 2) {
            if (input.back || input.activate || input.clicked) {
                room = room == 2 ? 0 : 1;
                selected = -1;
            }
        } else {
            regions = room == 0 ? swcBarRegions : swcBunkRegions;
            count = room == 0 ? 6 : 11;
            if (input.back) {
                room = 0;
                selected = -1;
            } else {
                if (input.moved || input.clicked) {
                    selected = -1;
                    /* Original PtInRect is half-open. RecRoom scans doors
                     * first, then characters; the last match wins. */
                    for (entry = 0; entry < count; entry++) {
                        hover = room == 0 ? (entry + 3) % 6 : entry;
                        if (carrier.mouseX >= regions[hover].left && carrier.mouseX < regions[hover].right &&
                            carrier.mouseY >= regions[hover].top && carrier.mouseY < regions[hover].bottom)
                            selected = regions[hover].frame;
                    }
                    keyboard = 0;
                }
                if (input.step != 0) {
                    selected = selected < 0 ? (input.step > 0 ? 0 : count - 1) :
                        (selected + input.step + count) % count;
                    keyboard = 1;
                }
                if ((input.activate || input.clicked) && selected >= 0) {
                    if (room == 0 && selected < 3) {
                        static const int sections[3] = {4, 8, 6};

                        sceneResult = SwcPlayRoomScene(&carrier, sections[selected]);
                        previous = SDL_GetTicks();
                        if (sceneResult != 0) {
                            result = sceneResult;
                            break;
                        }
                        fadeIn = 1;
                    } else if (room == 0) {
                        if (selected == 3)
                            room = 1;
                        else if (selected == 4)
                            room = 2;
                        else
                            notice = "Sorry! The flight simulator is not available in this demo.";
                    } else if (selected == 0) {
                        result = SwcPlayRoomScene(&carrier, 0);
                        break;
                    } else if (selected == 1) {
                        room = 0;
                    } else if (selected == 2) {
                        room = 3;
                    } else {
                        notice = "Bunks hold saved games. SWC saving and loading are not implemented yet.";
                    }
                    selected = -1;
                }
            }
        }
        SDL_SetWindowTitle(window, room == 0
            ? "Super Wing Commander - bar | Click: select | Tab/arrows, Enter | Esc: exit"
            : "Super Wing Commander - carrier | Click: select | Tab/arrows, Enter | Esc: back");
        if (hand != NULL)
            SDL_SetCursor(selected >= 0 && room < 2 && notice == NULL ? hand : savedCursor);
        if (room != previousRoom) {
            result = SwcFadeRoom(&carrier, 0);
            if (result != 0)
                break;
            result = -1;
            fadeIn = 1;
            previous = SDL_GetTicks();
        }
        tick = (Uint32)((uint64_t)elapsed * 30 / 1000);
        steps = SDL_min(8U, tick / 2 - animationTick / 2);
        if (room == 1 && tick != animationTick)
            wallFrame = RandomBelowOrEqual(2);
        animationTick = tick;
        simulatorFrame = SDL_clamp(simulatorFrame + (room == 0 && selected == 5 ? (int)steps : -(int)steps), 0, 6);
        for (entry = 0; entry < 3; entry++) {
            bunkHover[entry] = SDL_clamp(bunkHover[entry] +
                (room == 1 && selected == entry ? (int)steps : -(int)steps), -1, entry == 2 ? 4 : 7);
        }
        if (SDL_SetRenderTarget(renderer, carrier.picture) != 0 ||
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 || SDL_RenderClear(renderer) != 0)
            break;
        if (room == 0) {
            SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 1, 0, 0, 0);
            if (selected == 3)
                SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 1, 1, 0, 0);
            SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 34, SwcBarIdleFrame(0, tick, selected == 0), 0, 0);
            SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 29, SwcBarIdleFrame(1, tick, selected == 1), 0, 0);
            SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 28, SwcBarIdleFrame(2, tick, selected == 2), 0, 0);
            SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 11, simulatorFrame, 0, 85);
            if (selected == 4)
                SwcDrawRoomArt(&carrier, SWC_RECROOM, "RCRM", 12, 0, 245, 15);
        } else if (room == 1) {
            SwcDrawRoomArt(&carrier, SWC_BARRACKS, "RCRM", 13, 0, 0, 0);
            for (entry = 0; entry < 3; entry++) {
                if (bunkHover[entry] >= 0)
                    SwcDrawRoomArt(&carrier, SWC_BARRACKS, "RCRM", 13,
                        entry * 8 + 1 + bunkHover[entry], 0, 0);
            }
            /* BarracksScreen: wall display and the small desk light. */
            SwcDrawRoomArt(&carrier, SWC_BARRACKS, "RCRM", 23, wallFrame, 0, 5);
            SwcDrawRoomArt(&carrier, SWC_BARRACKS, "RCRM", 14, 0, 219, 183);
        } else if (room == 2) {
            SwcDrawRankings(&carrier);
        } else {
            SwcDrawMedalCase(&carrier);
        }
        if (carrier.drawResult != 0)
            break;
        if (notice != NULL) {
            if (SdlDrawSwcRoomSubtitle(renderer, &carrier.roomFont, notice) != 0)
                break;
        } else if (room < 2) {
            regions = room == 0 ? swcBarRegions : swcBunkRegions;
            labels = room == 0 ? swcBarLabels : swcBunkLabels;
            if (selected >= 0 && keyboard) {
                highlight = (SDL_Rect){regions[selected].left, regions[selected].top,
                    regions[selected].right - regions[selected].left,
                    regions[selected].bottom - regions[selected].top};
                if (SDL_SetRenderDrawColor(renderer, 245, 225, 135, 255) != 0 ||
                    SDL_RenderDrawRect(renderer, &highlight) != 0)
                    break;
            }
            if (SdlDrawSwcRoomSubtitle(renderer, &carrier.roomFont,
                selected >= 0 ? labels[selected] : "") != 0)
                break;
        }
        result = fadeIn ? SwcFadeRoom(&carrier, 1) : SwcPresentRoom(&carrier, 255);
        if (result != 0)
            break;
        if (fadeIn)
            previous = SDL_GetTicks();
        fadeIn = 0;
        result = -1;
        SDL_Delay(20);
    }
done:
    SDL_strlcpy(failure, SDL_GetError(), sizeof(failure));
    pMissionCampaignData = savedCampaignData;
    SdlClearSwcPcm();
    SdlStopSwcMusic();
    SDL_SetCursor(savedCursor);
    SDL_FreeCursor(hand);
    SDL_ShowCursor(oldCursorVisible);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(carrier.picture);
    SDL_DestroyTexture(carrier.dimPicture);
    SDL_DestroyTexture(carrier.roomFont.texture);
    SDL_free(carrier.marks);
    SDL_DestroyTexture(carrier.font);
    for (pack = 0; pack < SWC_ROOM_PACKS; pack++) {
        for (entry = 0; entry < carrier.packs[pack].count; entry++) {
            art = &carrier.packs[pack].art[entry];
            if (art->textures != NULL) {
                for (frame = 0; frame < art->count; frame++)
                    SDL_DestroyTexture(art->textures[frame]);
            }
            SDL_free(art->textures);
            SDL_free(art->bounds);
            SDL_free(art->shape.data);
        }
        SwcCMClose(&carrier.packs[pack].cmf);
    }
    SwcCMClose(&data);
    SDL_free(roster.data);
    SDL_free(campaign.data);
    SDL_free(carrier.script.data);
    if (result == -1)
        SDL_SetError("SWC carrier: %s", failure);
    return result;
}
