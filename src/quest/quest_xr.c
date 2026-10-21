/*
 * OpenXR render thread for the Wing Commander Quest port.  See quest_xr.h.
 */
#include "quest_xr.h"

#include <android/log.h>
#include <jni.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#define XR_USE_PLATFORM_ANDROID 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "SDL.h"
#include "SDL_system.h"

#define LOG_TAG "WC1VR"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* Screen layout (metres, LOCAL space relative to the recentre anchor). */
#define SCREEN_DISTANCE 2.4f
#define SCREEN_WIDTH_M 2.6f
#define SCREEN_HEIGHT_M (SCREEN_WIDTH_M * 3.0f / 4.0f)
#define SCREEN_DROP_M 0.05f
/* 320x200 shown at 4:3 with integer scaling (5x horizontally, 6x vertically). */
#define SCREEN_PIXELS_W 1600
#define SCREEN_PIXELS_H 1200
#define BEAM_WIDTH_M 0.004f
#define BEAM_MAX_M 6.0f

typedef struct Vec3 { float x, y, z; } Vec3;

static Vec3 v3(float x, float y, float z) { Vec3 r = { x, y, z }; return r; }
static Vec3 v3add(Vec3 a, Vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static Vec3 v3sub(Vec3 a, Vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static Vec3 v3scale(Vec3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static float v3dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 v3cross(Vec3 a, Vec3 b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}
static Vec3 v3norm(Vec3 a)
{
    float l = sqrtf(v3dot(a, a));
    return l > 1e-6f ? v3scale(a, 1.0f / l) : v3(0, 0, -1);
}
static Vec3 fromXr(XrVector3f v) { return v3(v.x, v.y, v.z); }
static XrVector3f toXr(Vec3 v) { XrVector3f r = { v.x, v.y, v.z }; return r; }

static Vec3 quatRotate(XrQuaternionf q, Vec3 v)
{
    Vec3 u = v3(q.x, q.y, q.z);
    Vec3 t = v3scale(v3cross(u, v), 2.0f);
    return v3add(v3add(v, v3scale(t, q.w)), v3cross(u, t));
}

static XrQuaternionf quatYaw(float yaw)
{
    XrQuaternionf q = { 0.0f, sinf(yaw * 0.5f), 0.0f, cosf(yaw * 0.5f) };
    return q;
}

/* Rotation matrix with columns x, y, z -> quaternion. */
static XrQuaternionf quatFromBasis(Vec3 x, Vec3 y, Vec3 z)
{
    XrQuaternionf q;
    float trace = x.x + y.y + z.z;

    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (y.z - z.y) / s;
        q.y = (z.x - x.z) / s;
        q.z = (x.y - y.x) / s;
    } else if (x.x > y.y && x.x > z.z) {
        float s = sqrtf(1.0f + x.x - y.y - z.z) * 2.0f;
        q.w = (y.z - z.y) / s;
        q.x = 0.25f * s;
        q.y = (y.x + x.y) / s;
        q.z = (z.x + x.z) / s;
    } else if (y.y > z.z) {
        float s = sqrtf(1.0f + y.y - x.x - z.z) * 2.0f;
        q.w = (z.x - x.z) / s;
        q.x = (y.x + x.y) / s;
        q.y = 0.25f * s;
        q.z = (z.y + y.z) / s;
    } else {
        float s = sqrtf(1.0f + z.z - x.x - y.y) * 2.0f;
        q.w = (x.y - y.x) / s;
        q.x = (z.x + x.z) / s;
        q.y = (z.y + y.z) / s;
        q.z = 0.25f * s;
    }
    return q;
}

/* ------------------------------------------------------------------ */
/* Shared state between the game thread and the XR thread              */
/* ------------------------------------------------------------------ */

static pthread_t g_thread;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_tickCond = PTHREAD_COND_INITIALIZER;
static volatile int g_threadRunning;
static volatile int g_quit;
static unsigned int g_tickCount;

static unsigned char g_frameRgba[QUEST_FRAME_WIDTH * QUEST_FRAME_HEIGHT * 4];
static unsigned int g_frameSerial;
static unsigned int g_errorColour;
static int g_errorColourSet;

static QuestXrControllers g_controllers;
static QuestXrPointer g_pointer;
static volatile int g_piloting;

static float g_hapticAmplitude[2];
static float g_hapticSeconds[2];

/* ------------------------------------------------------------------ */
/* XR thread state                                                      */
/* ------------------------------------------------------------------ */

typedef struct Swapchain {
    XrSwapchain handle;
    int width, height;
    uint32_t count;
    XrSwapchainImageOpenGLESKHR images[8];
    GLuint framebuffers[8];
} Swapchain;

static JavaVM *g_vm;
static jobject g_activity;

static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLConfig g_eglConfig;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;

static XrInstance g_instance = XR_NULL_HANDLE;
static XrSystemId g_system = XR_NULL_SYSTEM_ID;
static XrSession g_session = XR_NULL_HANDLE;
static XrSpace g_localSpace = XR_NULL_HANDLE;
static XrSpace g_viewSpace = XR_NULL_HANDLE;
static XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;
static int g_sessionRunning;
static int g_hasRefreshRateExt;

static XrActionSet g_actionSet;
static XrAction g_aimAction, g_triggerAction, g_gripAction, g_stickAction,
    g_stickClickAction, g_aAction, g_bAction, g_xAction, g_yAction,
    g_menuAction, g_hapticAction;
static XrPath g_handPath[2];
static XrSpace g_aimSpace[2];

static Swapchain g_screenChain;
static Swapchain g_beamChain;
static int g_srgb;
static GLuint g_frameTexture;
static GLuint g_frameReadFbo;
static unsigned int g_uploadedSerial = ~0U;

/* Recentre anchor: head position + yaw. */
static int g_anchorValid;
static Vec3 g_anchorPos;
static float g_anchorYaw;
static int g_recentreRequested = 1;
static double g_recentreHoldStart;
static int g_aimHand = 1;

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#define XR_CHECK(call)                                                   \
    do {                                                                 \
        XrResult xr_check_result = (call);                               \
        if (XR_FAILED(xr_check_result)) {                                \
            LOGE("%s failed: %d (%s:%d)", #call, (int)xr_check_result,   \
                 __FILE__, __LINE__);                                    \
            return 0;                                                    \
        }                                                                \
    } while (0)

/* ------------------------------------------------------------------ */
/* EGL                                                                  */
/* ------------------------------------------------------------------ */

static int InitEgl(void)
{
    static const EGLint configAttribs[] = {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, 0x40 /* EGL_OPENGL_ES3_BIT_KHR */,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE
    };
    static const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
    };
    static const EGLint surfaceAttribs[] = {
        EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE
    };
    EGLint count = 0;

    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY ||
        !eglInitialize(g_eglDisplay, 0, 0)) {
        LOGE("eglInitialize failed");
        return 0;
    }
    if (!eglChooseConfig(g_eglDisplay, configAttribs, &g_eglConfig, 1,
                         &count) || count < 1) {
        LOGE("eglChooseConfig failed");
        return 0;
    }
    g_eglContext = eglCreateContext(g_eglDisplay, g_eglConfig,
                                    EGL_NO_CONTEXT, contextAttribs);
    if (g_eglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: 0x%x", eglGetError());
        return 0;
    }
    g_eglSurface = eglCreatePbufferSurface(g_eglDisplay, g_eglConfig,
                                           surfaceAttribs);
    if (g_eglSurface == EGL_NO_SURFACE ||
        !eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface,
                        g_eglContext)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError());
        return 0;
    }
    LOGI("GL: %s / %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    return 1;
}

static void ShutdownEgl(void)
{
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        if (g_eglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_eglDisplay, g_eglSurface);
        if (g_eglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_eglDisplay, g_eglContext);
        eglTerminate(g_eglDisplay);
    }
    g_eglDisplay = EGL_NO_DISPLAY;
    g_eglContext = EGL_NO_CONTEXT;
    g_eglSurface = EGL_NO_SURFACE;
}

/* ------------------------------------------------------------------ */
/* OpenXR setup                                                         */
/* ------------------------------------------------------------------ */

static int HasExtension(const XrExtensionProperties *props, uint32_t count,
                        const char *name)
{
    uint32_t i;
    for (i = 0; i < count; i++)
        if (strcmp(props[i].extensionName, name) == 0)
            return 1;
    return 0;
}

static int InitInstance(void)
{
    PFN_xrInitializeLoaderKHR initializeLoader = 0;
    XrLoaderInitInfoAndroidKHR loaderInfo;
    XrInstanceCreateInfoAndroidKHR androidInfo;
    XrInstanceCreateInfo createInfo;
    XrExtensionProperties *props;
    uint32_t extCount = 0;
    const char *extensions[4];
    uint32_t enabled = 0;
    XrSystemGetInfo systemInfo;

    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                          (PFN_xrVoidFunction *)&initializeLoader);
    if (initializeLoader != 0) {
        memset(&loaderInfo, 0, sizeof(loaderInfo));
        loaderInfo.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInfo.applicationVM = g_vm;
        loaderInfo.applicationContext = g_activity;
        initializeLoader((const XrLoaderInitInfoBaseHeaderKHR *)&loaderInfo);
    }

    XR_CHECK(xrEnumerateInstanceExtensionProperties(0, 0, &extCount, 0));
    props = (XrExtensionProperties *)calloc(extCount ? extCount : 1,
                                            sizeof(*props));
    {
        uint32_t i;
        for (i = 0; i < extCount; i++)
            props[i].type = XR_TYPE_EXTENSION_PROPERTIES;
    }
    xrEnumerateInstanceExtensionProperties(0, extCount, &extCount, props);
    if (!HasExtension(props, extCount, XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME)) {
        LOGE("runtime lacks XR_KHR_opengl_es_enable");
        free(props);
        return 0;
    }
    extensions[enabled++] = XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
    if (HasExtension(props, extCount,
                     XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME))
        extensions[enabled++] = XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME;
    if (HasExtension(props, extCount,
                     XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME)) {
        extensions[enabled++] = XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME;
        g_hasRefreshRateExt = 1;
    }
    free(props);

    memset(&androidInfo, 0, sizeof(androidInfo));
    androidInfo.type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR;
    androidInfo.applicationVM = g_vm;
    androidInfo.applicationActivity = g_activity;

    memset(&createInfo, 0, sizeof(createInfo));
    createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    createInfo.next = &androidInfo;
    strcpy(createInfo.applicationInfo.applicationName, "Wing Commander VR");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "wc1-re");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 34);
    createInfo.enabledExtensionCount = enabled;
    createInfo.enabledExtensionNames = extensions;
    XR_CHECK(xrCreateInstance(&createInfo, &g_instance));

    memset(&systemInfo, 0, sizeof(systemInfo));
    systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XR_CHECK(xrGetSystem(g_instance, &systemInfo, &g_system));
    return 1;
}

static XrPath MakePath(const char *text)
{
    XrPath path = XR_NULL_PATH;
    xrStringToPath(g_instance, text, &path);
    return path;
}

static int CreateAction(XrAction *action, const char *name,
                        XrActionType type, int perHand)
{
    XrActionCreateInfo info;

    memset(&info, 0, sizeof(info));
    info.type = XR_TYPE_ACTION_CREATE_INFO;
    info.actionType = type;
    strcpy(info.actionName, name);
    strcpy(info.localizedActionName, name);
    if (perHand) {
        info.countSubactionPaths = 2;
        info.subactionPaths = g_handPath;
    }
    XR_CHECK(xrCreateAction(g_actionSet, &info, action));
    return 1;
}

static int InitActions(void)
{
    XrActionSetCreateInfo setInfo;
    XrActionSuggestedBinding bindings[32];
    XrInteractionProfileSuggestedBinding suggested;
    XrSessionActionSetsAttachInfo attach;
    int n = 0;
    int hand;

    memset(&setInfo, 0, sizeof(setInfo));
    setInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    strcpy(setInfo.actionSetName, "wc1");
    strcpy(setInfo.localizedActionSetName, "Wing Commander");
    XR_CHECK(xrCreateActionSet(g_instance, &setInfo, &g_actionSet));

    g_handPath[0] = MakePath("/user/hand/left");
    g_handPath[1] = MakePath("/user/hand/right");

    if (!CreateAction(&g_aimAction, "aim", XR_ACTION_TYPE_POSE_INPUT, 1) ||
        !CreateAction(&g_triggerAction, "trigger",
                      XR_ACTION_TYPE_FLOAT_INPUT, 1) ||
        !CreateAction(&g_gripAction, "grip", XR_ACTION_TYPE_FLOAT_INPUT, 1) ||
        !CreateAction(&g_stickAction, "stick",
                      XR_ACTION_TYPE_VECTOR2F_INPUT, 1) ||
        !CreateAction(&g_stickClickAction, "stick_click",
                      XR_ACTION_TYPE_BOOLEAN_INPUT, 1) ||
        !CreateAction(&g_aAction, "a", XR_ACTION_TYPE_BOOLEAN_INPUT, 0) ||
        !CreateAction(&g_bAction, "b", XR_ACTION_TYPE_BOOLEAN_INPUT, 0) ||
        !CreateAction(&g_xAction, "x", XR_ACTION_TYPE_BOOLEAN_INPUT, 0) ||
        !CreateAction(&g_yAction, "y", XR_ACTION_TYPE_BOOLEAN_INPUT, 0) ||
        !CreateAction(&g_menuAction, "menu", XR_ACTION_TYPE_BOOLEAN_INPUT, 0) ||
        !CreateAction(&g_hapticAction, "haptic",
                      XR_ACTION_TYPE_VIBRATION_OUTPUT, 1))
        return 0;

#define BIND(xr_action, xr_path)                              \
    do {                                                      \
        bindings[n].action = (xr_action);                     \
        bindings[n].binding = MakePath(xr_path);              \
        n++;                                                  \
    } while (0)
    BIND(g_aimAction, "/user/hand/left/input/aim/pose");
    BIND(g_aimAction, "/user/hand/right/input/aim/pose");
    BIND(g_triggerAction, "/user/hand/left/input/trigger/value");
    BIND(g_triggerAction, "/user/hand/right/input/trigger/value");
    BIND(g_gripAction, "/user/hand/left/input/squeeze/value");
    BIND(g_gripAction, "/user/hand/right/input/squeeze/value");
    BIND(g_stickAction, "/user/hand/left/input/thumbstick");
    BIND(g_stickAction, "/user/hand/right/input/thumbstick");
    BIND(g_stickClickAction, "/user/hand/left/input/thumbstick/click");
    BIND(g_stickClickAction, "/user/hand/right/input/thumbstick/click");
    BIND(g_aAction, "/user/hand/right/input/a/click");
    BIND(g_bAction, "/user/hand/right/input/b/click");
    BIND(g_xAction, "/user/hand/left/input/x/click");
    BIND(g_yAction, "/user/hand/left/input/y/click");
    BIND(g_menuAction, "/user/hand/left/input/menu/click");
    BIND(g_hapticAction, "/user/hand/left/output/haptic");
    BIND(g_hapticAction, "/user/hand/right/output/haptic");
#undef BIND

    memset(&suggested, 0, sizeof(suggested));
    suggested.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
    suggested.interactionProfile =
        MakePath("/interaction_profiles/oculus/touch_controller");
    suggested.countSuggestedBindings = (uint32_t)n;
    suggested.suggestedBindings = bindings;
    XR_CHECK(xrSuggestInteractionProfileBindings(g_instance, &suggested));

    for (hand = 0; hand < 2; hand++) {
        XrActionSpaceCreateInfo spaceInfo;

        memset(&spaceInfo, 0, sizeof(spaceInfo));
        spaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        spaceInfo.action = g_aimAction;
        spaceInfo.subactionPath = g_handPath[hand];
        spaceInfo.poseInActionSpace.orientation.w = 1.0f;
        XR_CHECK(xrCreateActionSpace(g_session, &spaceInfo,
                                     &g_aimSpace[hand]));
    }

    memset(&attach, 0, sizeof(attach));
    attach.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attach.countActionSets = 1;
    attach.actionSets = &g_actionSet;
    XR_CHECK(xrAttachSessionActionSets(g_session, &attach));
    return 1;
}

static int CreateSwapchain(Swapchain *chain, int width, int height,
                           int64_t format)
{
    XrSwapchainCreateInfo info;
    uint32_t i;

    memset(chain, 0, sizeof(*chain));
    memset(&info, 0, sizeof(info));
    info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                      XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT |
                      XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    info.format = format;
    info.sampleCount = 1;
    info.width = (uint32_t)width;
    info.height = (uint32_t)height;
    info.faceCount = 1;
    info.arraySize = 1;
    info.mipCount = 1;
    XR_CHECK(xrCreateSwapchain(g_session, &info, &chain->handle));
    chain->width = width;
    chain->height = height;
    XR_CHECK(xrEnumerateSwapchainImages(chain->handle, 0, &chain->count, 0));
    if (chain->count > 8)
        chain->count = 8;
    for (i = 0; i < chain->count; i++)
        chain->images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
    XR_CHECK(xrEnumerateSwapchainImages(
        chain->handle, chain->count, &chain->count,
        (XrSwapchainImageBaseHeader *)chain->images));
    glGenFramebuffers((GLsizei)chain->count, chain->framebuffers);
    for (i = 0; i < chain->count; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, chain->framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, chain->images[i].image, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return 1;
}

static int AcquireImage(Swapchain *chain, uint32_t *index)
{
    XrSwapchainImageAcquireInfo acquire = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    XrSwapchainImageWaitInfo wait = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };

    XR_CHECK(xrAcquireSwapchainImage(chain->handle, &acquire, index));
    wait.timeout = XR_INFINITE_DURATION;
    XR_CHECK(xrWaitSwapchainImage(chain->handle, &wait));
    return 1;
}

static void ReleaseImage(Swapchain *chain)
{
    XrSwapchainImageReleaseInfo release = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(chain->handle, &release);
}

static int InitSession(void)
{
    PFN_xrGetOpenGLESGraphicsRequirementsKHR getRequirements = 0;
    XrGraphicsRequirementsOpenGLESKHR requirements;
    XrGraphicsBindingOpenGLESAndroidKHR binding;
    XrSessionCreateInfo sessionInfo;
    XrReferenceSpaceCreateInfo spaceInfo;
    int64_t formats[64];
    uint32_t formatCount = 0;
    int64_t chosen = GL_RGBA8;
    uint32_t i;
    uint32_t index;

    xrGetInstanceProcAddr(g_instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                          (PFN_xrVoidFunction *)&getRequirements);
    if (getRequirements == 0) {
        LOGE("xrGetOpenGLESGraphicsRequirementsKHR missing");
        return 0;
    }
    memset(&requirements, 0, sizeof(requirements));
    requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    XR_CHECK(getRequirements(g_instance, g_system, &requirements));

    memset(&binding, 0, sizeof(binding));
    binding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    binding.display = g_eglDisplay;
    binding.config = g_eglConfig;
    binding.context = g_eglContext;

    memset(&sessionInfo, 0, sizeof(sessionInfo));
    sessionInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionInfo.next = &binding;
    sessionInfo.systemId = g_system;
    XR_CHECK(xrCreateSession(g_instance, &sessionInfo, &g_session));

    memset(&spaceInfo, 0, sizeof(spaceInfo));
    spaceInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XR_CHECK(xrCreateReferenceSpace(g_session, &spaceInfo, &g_localSpace));
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    XR_CHECK(xrCreateReferenceSpace(g_session, &spaceInfo, &g_viewSpace));

    if (!InitActions())
        return 0;

    XR_CHECK(xrEnumerateSwapchainFormats(g_session, 64, &formatCount,
                                         formats));
    for (i = 0; i < formatCount; i++) {
        if (formats[i] == GL_SRGB8_ALPHA8) {
            chosen = GL_SRGB8_ALPHA8;
            g_srgb = 1;
            break;
        }
    }
    LOGI("swapchain format 0x%llx (srgb=%d)", (long long)chosen, g_srgb);
    if (!CreateSwapchain(&g_screenChain, SCREEN_PIXELS_W, SCREEN_PIXELS_H,
                         chosen) ||
        !CreateSwapchain(&g_beamChain, 16, 16, chosen))
        return 0;

    /* The beam image never changes: fill it once. */
    if (AcquireImage(&g_beamChain, &index)) {
        glBindFramebuffer(GL_FRAMEBUFFER, g_beamChain.framebuffers[index]);
        glViewport(0, 0, 16, 16);
        /* Premultiplied light blue. */
        glClearColor(0.25f * 0.85f, 0.65f * 0.85f, 1.0f * 0.85f, 0.85f);
        glClear(GL_COLOR_BUFFER_BIT);
        glFinish();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ReleaseImage(&g_beamChain);
    }

    /* Game frame texture: sRGB-encoded palette colours.  With an sRGB
     * swapchain, an sRGB source texture makes the blit an identity copy. */
    glGenTextures(1, &g_frameTexture);
    glBindTexture(GL_TEXTURE_2D, g_frameTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, g_srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                   QUEST_FRAME_WIDTH, QUEST_FRAME_HEIGHT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &g_frameReadFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_frameReadFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, g_frameTexture, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return 1;
}

static void RequestRefreshRate(void)
{
    PFN_xrEnumerateDisplayRefreshRatesFB enumerateRates = 0;
    PFN_xrRequestDisplayRefreshRateFB requestRate = 0;
    float rates[16];
    uint32_t count = 0;
    float best = 0.0f;
    uint32_t i;

    if (!g_hasRefreshRateExt)
        return;
    xrGetInstanceProcAddr(g_instance, "xrEnumerateDisplayRefreshRatesFB",
                          (PFN_xrVoidFunction *)&enumerateRates);
    xrGetInstanceProcAddr(g_instance, "xrRequestDisplayRefreshRateFB",
                          (PFN_xrVoidFunction *)&requestRate);
    if (enumerateRates == 0 || requestRate == 0)
        return;
    if (XR_FAILED(enumerateRates(g_session, 16, &count, rates)))
        return;
    for (i = 0; i < count; i++)
        if (rates[i] <= 72.5f && rates[i] > best)
            best = rates[i];
    if (best > 0.0f) {
        requestRate(g_session, best);
        LOGI("display refresh rate %.1f Hz", best);
    }
}

/* ------------------------------------------------------------------ */
/* Per-frame work                                                       */
/* ------------------------------------------------------------------ */

static void HandleEvents(void)
{
    XrEventDataBuffer event;

    for (;;) {
        memset(&event, 0, sizeof(event));
        event.type = XR_TYPE_EVENT_DATA_BUFFER;
        if (xrPollEvent(g_instance, &event) != XR_SUCCESS)
            break;
        switch (event.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            const XrEventDataSessionStateChanged *changed =
                (const XrEventDataSessionStateChanged *)&event;
            g_sessionState = changed->state;
            LOGI("session state %d", (int)g_sessionState);
            if (g_sessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo begin;
                memset(&begin, 0, sizeof(begin));
                begin.type = XR_TYPE_SESSION_BEGIN_INFO;
                begin.primaryViewConfigurationType =
                    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                if (XR_SUCCEEDED(xrBeginSession(g_session, &begin))) {
                    g_sessionRunning = 1;
                    RequestRefreshRate();
                }
            } else if (g_sessionState == XR_SESSION_STATE_STOPPING) {
                xrEndSession(g_session);
                g_sessionRunning = 0;
            } else if (g_sessionState == XR_SESSION_STATE_EXITING ||
                       g_sessionState == XR_SESSION_STATE_LOSS_PENDING) {
                g_sessionRunning = 0;
                g_quit = 1;
            }
            break;
        }
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            g_recentreRequested = 1;
            break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            g_quit = 1;
            break;
        default:
            break;
        }
    }
}

static float GetFloat(XrAction action, int hand)
{
    XrActionStateGetInfo info = { XR_TYPE_ACTION_STATE_GET_INFO };
    XrActionStateFloat state = { XR_TYPE_ACTION_STATE_FLOAT };

    info.action = action;
    info.subactionPath = hand >= 0 ? g_handPath[hand] : XR_NULL_PATH;
    if (XR_FAILED(xrGetActionStateFloat(g_session, &info, &state)) ||
        !state.isActive)
        return 0.0f;
    return state.currentState;
}

static int GetBool(XrAction action, int hand)
{
    XrActionStateGetInfo info = { XR_TYPE_ACTION_STATE_GET_INFO };
    XrActionStateBoolean state = { XR_TYPE_ACTION_STATE_BOOLEAN };

    info.action = action;
    info.subactionPath = hand >= 0 ? g_handPath[hand] : XR_NULL_PATH;
    if (XR_FAILED(xrGetActionStateBoolean(g_session, &info, &state)) ||
        !state.isActive)
        return 0;
    return state.currentState ? 1 : 0;
}

static XrVector2f GetVec2(XrAction action, int hand)
{
    XrActionStateGetInfo info = { XR_TYPE_ACTION_STATE_GET_INFO };
    XrActionStateVector2f state = { XR_TYPE_ACTION_STATE_VECTOR2F };
    XrVector2f zero = { 0.0f, 0.0f };

    info.action = action;
    info.subactionPath = g_handPath[hand];
    if (XR_FAILED(xrGetActionStateVector2f(g_session, &info, &state)) ||
        !state.isActive)
        return zero;
    return state.currentState;
}

static void ApplyHaptics(void)
{
    int hand;

    for (hand = 0; hand < 2; hand++) {
        float amplitude;
        float seconds;

        pthread_mutex_lock(&g_lock);
        amplitude = g_hapticAmplitude[hand];
        seconds = g_hapticSeconds[hand];
        g_hapticAmplitude[hand] = 0.0f;
        pthread_mutex_unlock(&g_lock);
        if (amplitude > 0.0f) {
            XrHapticActionInfo info = { XR_TYPE_HAPTIC_ACTION_INFO };
            XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };

            info.action = g_hapticAction;
            info.subactionPath = g_handPath[hand];
            vibration.amplitude = amplitude;
            vibration.duration = (XrDuration)(seconds * 1e9);
            vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
            xrApplyHapticFeedback(g_session, &info,
                                  (const XrHapticBaseHeader *)&vibration);
        }
    }
}

static int LocatePose(XrSpace space, XrTime time, XrPosef *pose)
{
    XrSpaceLocation location = { XR_TYPE_SPACE_LOCATION };

    if (XR_FAILED(xrLocateSpace(space, g_localSpace, time, &location)))
        return 0;
    if ((location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) == 0 ||
        (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) == 0)
        return 0;
    *pose = location.pose;
    return 1;
}

/* Screen centre and axes in LOCAL space. */
static void ScreenFrame(Vec3 *centre, Vec3 *right, Vec3 *up, Vec3 *normal,
                        XrQuaternionf *orientation)
{
    XrQuaternionf q = quatYaw(g_anchorYaw);

    *orientation = q;
    *right = quatRotate(q, v3(1, 0, 0));
    *up = v3(0, 1, 0);
    *normal = quatRotate(q, v3(0, 0, 1));
    *centre = v3add(g_anchorPos,
                    v3add(quatRotate(q, v3(0, 0, -SCREEN_DISTANCE)),
                          v3(0, -SCREEN_DROP_M, 0)));
}

static void UpdateInput(XrTime time, XrPosef *headPose, int headValid,
                        XrCompositionLayerQuad *beamLayer, int *beamVisible)
{
    XrActiveActionSet active;
    XrActionsSyncInfo sync;
    QuestXrControllers c;
    QuestXrPointer p;
    XrPosef aim[2];
    int aimValid[2];
    int hand;
    int focused = g_sessionState == XR_SESSION_STATE_FOCUSED;
    int piloting = g_piloting;

    memset(&c, 0, sizeof(c));
    memset(&p, 0, sizeof(p));
    *beamVisible = 0;

    if (focused) {
        memset(&active, 0, sizeof(active));
        active.actionSet = g_actionSet;
        memset(&sync, 0, sizeof(sync));
        sync.type = XR_TYPE_ACTIONS_SYNC_INFO;
        sync.countActiveActionSets = 1;
        sync.activeActionSets = &active;
        xrSyncActions(g_session, &sync);

        {
            XrVector2f left = GetVec2(g_stickAction, 0);
            XrVector2f right = GetVec2(g_stickAction, 1);
            c.leftStickX = left.x;
            c.leftStickY = left.y;
            c.rightStickX = right.x;
            c.rightStickY = right.y;
        }
        c.leftTrigger = GetFloat(g_triggerAction, 0);
        c.rightTrigger = GetFloat(g_triggerAction, 1);
        c.leftGrip = GetFloat(g_gripAction, 0);
        c.rightGrip = GetFloat(g_gripAction, 1);
        c.leftStickClick = GetBool(g_stickClickAction, 0);
        c.rightStickClick = GetBool(g_stickClickAction, 1);
        c.a = GetBool(g_aAction, -1);
        c.b = GetBool(g_bAction, -1);
        c.x = GetBool(g_xAction, -1);
        c.y = GetBool(g_yAction, -1);
        c.menu = GetBool(g_menuAction, -1);
        c.active = 1;
        ApplyHaptics();
    }

    /* Recentre: left stick click held for one second (a short tap is the
     * nav map, handled on the game side). */
    if (c.leftStickClick) {
        double now = NowSeconds();
        if (g_recentreHoldStart == 0.0)
            g_recentreHoldStart = now;
        else if (now - g_recentreHoldStart > 1.0 && g_recentreHoldStart > 0.0) {
            g_recentreRequested = 1;
            g_recentreHoldStart = -1.0; /* wait for release */
        }
    } else {
        g_recentreHoldStart = 0.0;
    }
    if (g_recentreRequested && headValid) {
        Vec3 forward = quatRotate(headPose->orientation, v3(0, 0, -1));
        g_anchorPos = fromXr(headPose->position);
        g_anchorYaw = atan2f(-forward.x, -forward.z);
        g_anchorValid = 1;
        g_recentreRequested = 0;
        LOGI("recentred: pos %.2f %.2f %.2f yaw %.2f", g_anchorPos.x,
             g_anchorPos.y, g_anchorPos.z, g_anchorYaw);
    }

    for (hand = 0; hand < 2; hand++)
        aimValid[hand] = focused && LocatePose(g_aimSpace[hand], time,
                                               &aim[hand]);

    /* The hand whose trigger was pressed last aims; the right hand is the
     * default. */
    if (c.leftTrigger > 0.6f && c.rightTrigger < 0.2f)
        g_aimHand = 0;
    else if (c.rightTrigger > 0.6f)
        g_aimHand = 1;
    if (!aimValid[g_aimHand] && aimValid[1 - g_aimHand])
        g_aimHand = 1 - g_aimHand;

    if (!piloting && g_anchorValid && aimValid[g_aimHand]) {
        Vec3 centre, right, up, normal;
        XrQuaternionf screenQ;
        Vec3 origin = fromXr(aim[g_aimHand].position);
        Vec3 dir = quatRotate(aim[g_aimHand].orientation, v3(0, 0, -1));
        float length = BEAM_MAX_M;
        float denom;

        ScreenFrame(&centre, &right, &up, &normal, &screenQ);
        denom = v3dot(dir, normal);
        if (denom < -1e-4f) {
            float t = v3dot(v3sub(centre, origin), normal) / denom;
            if (t > 0.0f) {
                Vec3 hit = v3add(origin, v3scale(dir, t));
                Vec3 rel = v3sub(hit, centre);
                float u = v3dot(rel, right) / SCREEN_WIDTH_M + 0.5f;
                float v = 0.5f - v3dot(rel, up) / SCREEN_HEIGHT_M;

                if (u >= 0.0f && u < 1.0f && v >= 0.0f && v < 1.0f) {
                    p.onScreen = 1;
                    p.x = (int)(u * QUEST_FRAME_WIDTH);
                    p.y = (int)(v * QUEST_FRAME_HEIGHT);
                    length = t;
                }
            }
        }
        if (g_aimHand == 1) {
            p.primary = c.rightTrigger > 0.55f;
            p.secondary = c.rightGrip > 0.55f;
        } else {
            p.primary = c.leftTrigger > 0.55f;
            p.secondary = c.leftGrip > 0.55f;
        }

        /* Beam quad: Y along the ray, facing the head. */
        {
            Vec3 mid = v3add(origin, v3scale(dir, length * 0.5f));
            Vec3 toHead = headValid ? v3sub(fromXr(headPose->position), mid)
                                    : v3scale(dir, -1.0f);
            Vec3 z = v3sub(toHead, v3scale(dir, v3dot(toHead, dir)));
            Vec3 x;

            z = v3norm(z);
            x = v3norm(v3cross(dir, z));
            z = v3cross(x, dir);
            memset(beamLayer, 0, sizeof(*beamLayer));
            beamLayer->type = XR_TYPE_COMPOSITION_LAYER_QUAD;
            beamLayer->layerFlags =
                XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            beamLayer->space = g_localSpace;
            beamLayer->eyeVisibility = XR_EYE_VISIBILITY_BOTH;
            beamLayer->subImage.swapchain = g_beamChain.handle;
            beamLayer->subImage.imageRect.extent.width = 16;
            beamLayer->subImage.imageRect.extent.height = 16;
            beamLayer->pose.position = toXr(mid);
            beamLayer->pose.orientation = quatFromBasis(x, dir, z);
            beamLayer->size.width = BEAM_WIDTH_M;
            beamLayer->size.height = length;
            *beamVisible = 1;
        }
    }

    pthread_mutex_lock(&g_lock);
    g_controllers = c;
    g_pointer = p;
    pthread_mutex_unlock(&g_lock);
}

static void DrawScreen(uint32_t index)
{
    unsigned int serial;

    pthread_mutex_lock(&g_lock);
    serial = g_frameSerial;
    if (serial != g_uploadedSerial && !g_errorColourSet) {
        glBindTexture(GL_TEXTURE_2D, g_frameTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, QUEST_FRAME_WIDTH,
                        QUEST_FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                        g_frameRgba);
        g_uploadedSerial = serial;
    }
    pthread_mutex_unlock(&g_lock);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_screenChain.framebuffers[index]);
    glViewport(0, 0, SCREEN_PIXELS_W, SCREEN_PIXELS_H);
    if (g_errorColourSet) {
        glClearColor(((g_errorColour >> 16) & 255) / 255.0f,
                     ((g_errorColour >> 8) & 255) / 255.0f,
                     (g_errorColour & 255) / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    } else if (g_uploadedSerial == ~0U) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, g_frameReadFbo);
        /* Texture row 0 holds the top frame row, but OpenXR treats GL
         * swapchain images with a bottom-left origin: flip vertically. */
        glBlitFramebuffer(0, 0, QUEST_FRAME_WIDTH, QUEST_FRAME_HEIGHT,
                          0, SCREEN_PIXELS_H, SCREEN_PIXELS_W, 0,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

static void RunFrame(void)
{
    XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState frameState = { XR_TYPE_FRAME_STATE };
    XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
    XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
    XrCompositionLayerQuad screenLayer;
    XrCompositionLayerQuad beamLayer;
    const XrCompositionLayerBaseHeader *layers[2];
    uint32_t layerCount = 0;
    XrPosef headPose;
    int headValid;
    int beamVisible = 0;
    uint32_t index;

    if (XR_FAILED(xrWaitFrame(g_session, &waitInfo, &frameState)))
        return;
    if (XR_FAILED(xrBeginFrame(g_session, &beginInfo)))
        return;

    headValid = LocatePose(g_viewSpace, frameState.predictedDisplayTime,
                           &headPose);
    UpdateInput(frameState.predictedDisplayTime, &headPose, headValid,
                &beamLayer, &beamVisible);

    if (frameState.shouldRender && g_anchorValid &&
        AcquireImage(&g_screenChain, &index)) {
        Vec3 centre, right, up, normal;
        XrQuaternionf q;

        DrawScreen(index);
        ReleaseImage(&g_screenChain);
        ScreenFrame(&centre, &right, &up, &normal, &q);
        memset(&screenLayer, 0, sizeof(screenLayer));
        screenLayer.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
        screenLayer.space = g_localSpace;
        screenLayer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        screenLayer.subImage.swapchain = g_screenChain.handle;
        screenLayer.subImage.imageRect.extent.width = SCREEN_PIXELS_W;
        screenLayer.subImage.imageRect.extent.height = SCREEN_PIXELS_H;
        screenLayer.pose.orientation = q;
        screenLayer.pose.position = toXr(centre);
        screenLayer.size.width = SCREEN_WIDTH_M;
        screenLayer.size.height = SCREEN_HEIGHT_M;
        layers[layerCount++] = (const XrCompositionLayerBaseHeader *)&screenLayer;
        if (beamVisible)
            layers[layerCount++] =
                (const XrCompositionLayerBaseHeader *)&beamLayer;
    }

    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = layerCount;
    endInfo.layers = layers;
    xrEndFrame(g_session, &endInfo);
}

static void SignalTick(void)
{
    pthread_mutex_lock(&g_lock);
    g_tickCount++;
    pthread_cond_broadcast(&g_tickCond);
    pthread_mutex_unlock(&g_lock);
}

static void *XrThreadMain(void *unused)
{
    JNIEnv *env = 0;
    (void)unused;

    (*g_vm)->AttachCurrentThread(g_vm, &env, 0);
    if (!InitEgl() || !InitInstance() || !InitSession()) {
        LOGE("OpenXR initialisation failed");
        g_threadRunning = 0;
        SignalTick();
        (*g_vm)->DetachCurrentThread(g_vm);
        return 0;
    }
    LOGI("OpenXR ready");
    while (!g_quit) {
        HandleEvents();
        if (g_sessionRunning) {
            RunFrame();
        } else {
            QuestXrControllers none;
            memset(&none, 0, sizeof(none));
            pthread_mutex_lock(&g_lock);
            g_controllers = none;
            memset(&g_pointer, 0, sizeof(g_pointer));
            pthread_mutex_unlock(&g_lock);
            usleep(10000);
        }
        SignalTick();
    }
    LOGI("OpenXR thread exiting");
    if (g_session != XR_NULL_HANDLE) {
        if (g_sessionRunning)
            xrEndSession(g_session);
        xrDestroySession(g_session);
    }
    if (g_instance != XR_NULL_HANDLE)
        xrDestroyInstance(g_instance);
    ShutdownEgl();
    g_threadRunning = 0;
    SignalTick();
    (*g_vm)->DetachCurrentThread(g_vm);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int QuestXr_Start(void)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();

    if (g_threadRunning)
        return 1;
    if (env == 0 || activity == 0) {
        LOGE("no JNI environment / activity");
        return 0;
    }
    (*env)->GetJavaVM(env, &g_vm);
    g_activity = (*env)->NewGlobalRef(env, activity);
    (*env)->DeleteLocalRef(env, activity);
    g_quit = 0;
    g_threadRunning = 1;
    if (pthread_create(&g_thread, 0, XrThreadMain, 0) != 0) {
        g_threadRunning = 0;
        return 0;
    }
    return 1;
}

void QuestXr_Shutdown(void)
{
    if (!g_threadRunning)
        return;
    g_quit = 1;
    pthread_join(g_thread, 0);
}

int QuestXr_IsRunning(void)
{
    return g_threadRunning;
}

void QuestXr_SubmitIndexedFrame(const unsigned char *pixels,
                                const unsigned char *palette)
{
    int i;
    unsigned char *out;

    if (pixels == 0 || palette == 0)
        return;
    pthread_mutex_lock(&g_lock);
    out = g_frameRgba;
    for (i = 0; i < QUEST_FRAME_WIDTH * QUEST_FRAME_HEIGHT; i++) {
        const unsigned char *entry = palette + pixels[i] * 4;
        out[0] = entry[2];
        out[1] = entry[1];
        out[2] = entry[0];
        out[3] = 255;
        out += 4;
    }
    g_frameSerial++;
    pthread_mutex_unlock(&g_lock);
}

void QuestXr_WaitForDisplayTick(void)
{
    struct timespec deadline;
    unsigned int start;

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 30 * 1000000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000;
    }
    pthread_mutex_lock(&g_lock);
    start = g_tickCount;
    while (g_threadRunning && g_tickCount == start) {
        if (pthread_cond_timedwait(&g_tickCond, &g_lock, &deadline) != 0)
            break;
    }
    pthread_mutex_unlock(&g_lock);
}

void QuestXr_GetControllers(QuestXrControllers *controllers)
{
    pthread_mutex_lock(&g_lock);
    *controllers = g_controllers;
    pthread_mutex_unlock(&g_lock);
}

void QuestXr_GetPointer(QuestXrPointer *pointer)
{
    pthread_mutex_lock(&g_lock);
    *pointer = g_pointer;
    pthread_mutex_unlock(&g_lock);
}

void QuestXr_SetPiloting(int piloting)
{
    g_piloting = piloting ? 1 : 0;
}

void QuestXr_ShowErrorColour(unsigned int rgb)
{
    pthread_mutex_lock(&g_lock);
    g_errorColour = rgb;
    g_errorColourSet = 1;
    pthread_mutex_unlock(&g_lock);
}

void QuestXr_Haptic(int hand, float amplitude, float seconds)
{
    if (hand < 0 || hand > 1)
        return;
    pthread_mutex_lock(&g_lock);
    g_hapticAmplitude[hand] = amplitude;
    g_hapticSeconds[hand] = seconds;
    pthread_mutex_unlock(&g_lock);
}
