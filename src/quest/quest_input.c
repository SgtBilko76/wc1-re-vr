/*
 * Game-side glue for the Meta Quest build (WC1_QUEST): start-up, logging,
 * and Touch controller input.
 *
 * Touch -> virtual SDL gamepad, read by the port's existing 4button-4axis
 * joystick code (src/sdl/joystick.c).  While piloting:
 *
 *   left stick        pitch / yaw            right stick X/Y  roll / throttle
 *   right trigger     fire guns (pad A)      left trigger     missile (pad B)
 *   right grip        afterburner (pad X)    A                cycle target (pad Y)
 *   B                 select missile (RB)    X                select guns (LB)
 *   Y                 pause (Start)          menu (left)      Escape (Back)
 *   left stick click  nav map (tap)          right stick click autopilot
 *   left grip + A/B/X/Y  D-pad right/down/left/up:
 *                     target lock / stop / comms / full speed
 *                     (comm menu: up/down select, right = send, left = close)
 *
 * Outside flight the pad is neutral; the laser drives the mouse
 * (trigger = left click, grip = right click), B / menu = Escape,
 * Y = "Y", X = "N", A = Enter.
 * Hold the left stick click for one second to recentre the screen.
 */
#include "wc1.h"
#include "../quest/quest_xr.h"

#include <android/log.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#define QUEST_PAD_VENDOR 0x2833 /* Oculus VR */
#define QUEST_PAD_PRODUCT 0x5743 /* "WC" */
#define QUEST_PAD_NAME "Quest Touch (Wing Commander)"

static int g_questPadDeviceIndex = -1;
static SDL_JoystickID g_questPadInstance = -1;

/* ------------------------------------------------------------------ */
/* stdout/stderr -> logcat                                              */
/* ------------------------------------------------------------------ */

static int g_logPipe[2];

static void *QuestLogThread(void *unused)
{
    char buffer[1024];
    size_t used = 0;
    (void)unused;

    for (;;) {
        ssize_t count = read(g_logPipe[0], buffer + used,
                             sizeof(buffer) - 1 - used);
        char *start;
        char *newline;

        if (count <= 0)
            break;
        used += (size_t)count;
        buffer[used] = 0;
        start = buffer;
        while ((newline = strchr(start, '\n')) != 0) {
            *newline = 0;
            __android_log_write(ANDROID_LOG_INFO, "WC1", start);
            start = newline + 1;
        }
        used = strlen(start);
        if (used == sizeof(buffer) - 1) {
            __android_log_write(ANDROID_LOG_INFO, "WC1", start);
            used = 0;
        } else {
            memmove(buffer, start, used);
        }
    }
    return 0;
}

static void QuestRedirectStdio(void)
{
    pthread_t thread;

    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);
    if (pipe(g_logPipe) != 0)
        return;
    dup2(g_logPipe[1], 1);
    dup2(g_logPipe[1], 2);
    if (pthread_create(&thread, 0, QuestLogThread, 0) == 0)
        pthread_detach(thread);
}

/* ------------------------------------------------------------------ */
/* Start-up                                                             */
/* ------------------------------------------------------------------ */

static int QuestDirectoryHasGame(const char *path)
{
    char probe[PATH_MAX];
    char resolved[PATH_MAX];
    struct stat info;

    SDL_snprintf(probe, sizeof(probe), "%s/GAMEDAT/GAME.PAL", path);
    if (!SdlResolvePath(probe, resolved, sizeof(resolved)))
        return 0;
    return stat(resolved, &info) == 0;
}

int SdlQuestStartup(int *argumentCount, char ***arguments)
{
    static char *questArguments[] = {
        "wc1", "--joystick-mode=4button-4axis",
        "--joystick-axes=twin-stick-roll", "--joystick-rumble", 0
    };
    static const char *const candidates[] = {
        "/sdcard/WingCommander", "/sdcard/wingcommander",
        "/sdcard/WC1", "/storage/emulated/0/WingCommander"
    };
    size_t i;

    QuestRedirectStdio();
    printf("Wing Commander VR starting\n");
    *argumentCount = 4;
    *arguments = questArguments;

    for (i = 0; i < SDL_arraysize(candidates); i++) {
        if (QuestDirectoryHasGame(candidates[i])) {
            if (SdlChangeDirectory(candidates[i]) == 0) {
                printf("game data: %s\n", candidates[i]);
                return 1;
            }
        }
    }
    fprintf(stderr, "No game data found. Copy the GOG Wing Commander folder "
                    "(WC.EXE + GAMEDAT) to /sdcard/WingCommander\n");
    /* Show a red screen so the problem is visible in the headset. */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0 && QuestXr_Start()) {
        QuestXr_ShowErrorColour(0x801818);
        while (QuestXr_IsRunning()) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    QuestXr_Shutdown();
                    return 0;
                }
            }
            SDL_Delay(50);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Virtual gamepad                                                      */
/* ------------------------------------------------------------------ */

static int SDLCALL QuestPadRumble(void *userdata, Uint16 low, Uint16 high)
{
    float amplitude = (float)(low > high ? low : high) / 65535.0f;
    (void)userdata;

    if (amplitude > 0.0f) {
        QuestXr_Haptic(0, amplitude, 0.1f);
        QuestXr_Haptic(1, amplitude, 0.1f);
    }
    return 0;
}

void SdlQuestCreatePad(void)
{
    SDL_VirtualJoystickDesc desc;

    SDL_zero(desc);
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;
    desc.vendor_id = QUEST_PAD_VENDOR;
    desc.product_id = QUEST_PAD_PRODUCT;
    desc.button_mask = (1U << (SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1)) - 1;
    desc.axis_mask = (1U << SDL_CONTROLLER_AXIS_MAX) - 1;
    desc.name = QUEST_PAD_NAME;
    desc.Rumble = QuestPadRumble;
    g_questPadDeviceIndex = SDL_JoystickAttachVirtualEx(&desc);
    if (g_questPadDeviceIndex < 0) {
        fprintf(stderr, "virtual pad: %s\n", SDL_GetError());
        return;
    }
    g_questPadInstance = SDL_JoystickGetDeviceInstanceID(g_questPadDeviceIndex);
    printf("virtual Touch pad attached (index %d, instance %d)\n",
           g_questPadDeviceIndex, (int)g_questPadInstance);
}

int SdlQuestIsQuestPad(int deviceIndex)
{
    return SDL_JoystickGetDeviceVendor(deviceIndex) == QUEST_PAD_VENDOR &&
           SDL_JoystickGetDeviceProduct(deviceIndex) == QUEST_PAD_PRODUCT;
}

/* ------------------------------------------------------------------ */
/* Per-pump input service                                               */
/* ------------------------------------------------------------------ */

enum {
    PHYS_RTRIG, PHYS_LTRIG, PHYS_RGRIP, PHYS_LGRIP, PHYS_A, PHYS_B, PHYS_X,
    PHYS_Y, PHYS_LCLICK, PHYS_RCLICK, PHYS_MENU, PHYS_COUNT
};

static int g_prevMode = -1;
static int g_latched[PHYS_COUNT];
static int g_prevPhys[PHYS_COUNT];
static Uint32 g_leftClickStart;
static int g_navPulse;
static int g_prevPointerX = -1, g_prevPointerY = -1;
static int g_prevPrimary, g_prevSecondary;

static Sint16 AxisValue(float v)
{
    if (v > 1.0f)
        v = 1.0f;
    else if (v < -1.0f)
        v = -1.0f;
    return (Sint16)(v * 32767.0f);
}

static void PushKey(SDL_Keycode key, int pressed)
{
    SDL_Event event;

    SDL_zero(event);
    event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = key;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
    SDL_PushEvent(&event);
}

static void TapKey(SDL_Keycode key)
{
    PushKey(key, 1);
    PushKey(key, 0);
}

void SdlQuestServiceInput(void)
{
    QuestXrControllers c;
    QuestXrPointer p;
    SDL_Joystick *pad;
    int phys[PHYS_COUNT];
    int edge[PHYS_COUNT];
    int piloting;
    int i;
    Uint32 now = SDL_GetTicks();

    QuestXr_GetControllers(&c);
    QuestXr_GetPointer(&p);
    piloting = SdlQuestIsPiloting();
    QuestXr_SetPiloting(piloting);

    phys[PHYS_RTRIG] = c.rightTrigger > 0.5f;
    phys[PHYS_LTRIG] = c.leftTrigger > 0.5f;
    phys[PHYS_RGRIP] = c.rightGrip > 0.5f;
    phys[PHYS_LGRIP] = c.leftGrip > 0.5f;
    phys[PHYS_A] = c.a;
    phys[PHYS_B] = c.b;
    phys[PHYS_X] = c.x;
    phys[PHYS_Y] = c.y;
    phys[PHYS_LCLICK] = c.leftStickClick;
    phys[PHYS_RCLICK] = c.rightStickClick;
    phys[PHYS_MENU] = c.menu;

    /* A button held across a flight <-> menu switch stays masked until it
     * is released, so one press never acts in both modes. */
    if (piloting != g_prevMode) {
        for (i = 0; i < PHYS_COUNT; i++)
            g_latched[i] = phys[i];
        g_prevMode = piloting;
        g_leftClickStart = 0;
    }
    for (i = 0; i < PHYS_COUNT; i++) {
        if (!phys[i])
            g_latched[i] = 0;
        if (g_latched[i])
            phys[i] = 0;
        edge[i] = phys[i] && !g_prevPhys[i];
    }

    /* Left stick click: tap = nav map, hold = recentre (XR thread). */
    if (edge[PHYS_LCLICK])
        g_leftClickStart = now;
    if (!phys[PHYS_LCLICK] && g_prevPhys[PHYS_LCLICK] &&
        g_leftClickStart != 0 && now - g_leftClickStart < 600)
        g_navPulse = 2;

    pad = g_questPadInstance >= 0 ? SDL_JoystickFromInstanceID(g_questPadInstance)
                                  : 0;
    if (pad != 0) {
        int mod = phys[PHYS_LGRIP];
        int b[SDL_CONTROLLER_BUTTON_MAX];

        SDL_zeroa(b);
        if (piloting) {
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_LEFTX,
                                       AxisValue(c.leftStickX));
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_LEFTY,
                                       AxisValue(-c.leftStickY));
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX,
                                       AxisValue(c.rightStickX));
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY,
                                       AxisValue(-c.rightStickY));
            b[SDL_CONTROLLER_BUTTON_A] = phys[PHYS_RTRIG];
            b[SDL_CONTROLLER_BUTTON_B] = phys[PHYS_LTRIG];
            b[SDL_CONTROLLER_BUTTON_X] = phys[PHYS_RGRIP];
            if (mod) {
                b[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = phys[PHYS_A];
                b[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = phys[PHYS_B];
                b[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = phys[PHYS_X];
                b[SDL_CONTROLLER_BUTTON_DPAD_UP] = phys[PHYS_Y];
            } else {
                b[SDL_CONTROLLER_BUTTON_Y] = phys[PHYS_A];
                b[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = phys[PHYS_B];
                b[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = phys[PHYS_X];
                b[SDL_CONTROLLER_BUTTON_START] = phys[PHYS_Y];
            }
            b[SDL_CONTROLLER_BUTTON_BACK] = phys[PHYS_MENU];
            b[SDL_CONTROLLER_BUTTON_RIGHTSTICK] = phys[PHYS_RCLICK];
            if (g_navPulse > 0) {
                b[SDL_CONTROLLER_BUTTON_LEFTSTICK] = 1;
                g_navPulse--;
            }
        } else {
            int axis;
            for (axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
                SDL_JoystickSetVirtualAxis(pad, axis,
                                           axis >= SDL_CONTROLLER_AXIS_TRIGGERLEFT
                                               ? -32768 : 0);
            b[SDL_CONTROLLER_BUTTON_BACK] = phys[PHYS_B] || phys[PHYS_MENU];
            b[SDL_CONTROLLER_BUTTON_Y] = phys[PHYS_Y];
            g_navPulse = 0;
        }
        for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
            SDL_JoystickSetVirtualButton(pad, i, (Uint8)b[i]);
    }

    if (!piloting) {
        if (edge[PHYS_A])
            TapKey(SDLK_RETURN);
        if (edge[PHYS_X])
            TapKey(SDLK_n);

        /* Laser pointer -> mouse. */
        if (p.onScreen) {
            int primary = p.primary && !g_latched[PHYS_RTRIG] &&
                          !g_latched[PHYS_LTRIG];
            int secondary = p.secondary;

            if (p.x != g_prevPointerX || p.y != g_prevPointerY)
                SdlQuestInjectMouse(p.x, p.y, g_prevPrimary,
                                    g_prevSecondary, 0);
            if (primary != g_prevPrimary || secondary != g_prevSecondary) {
                int down = (primary && !g_prevPrimary) ||
                           (secondary && !g_prevSecondary);
                SdlQuestInjectMouse(p.x, p.y, primary, secondary,
                                    down ? 2 : 1);
            }
            g_prevPointerX = p.x;
            g_prevPointerY = p.y;
            g_prevPrimary = primary;
            g_prevSecondary = secondary;
        } else if (g_prevPrimary || g_prevSecondary) {
            SdlQuestInjectMouse(g_prevPointerX, g_prevPointerY, 0, 0, 1);
            g_prevPrimary = 0;
            g_prevSecondary = 0;
        }
    } else {
        g_prevPrimary = 0;
        g_prevSecondary = 0;
    }

    for (i = 0; i < PHYS_COUNT; i++)
        g_prevPhys[i] = phys[i];
}
