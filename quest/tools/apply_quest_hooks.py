#!/usr/bin/env python3
"""Apply the WC1_QUEST hooks to the SDL2 port sources (idempotent)."""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]


def edit(rel, old, new, marker):
    path = ROOT / rel
    text = path.read_text()
    if marker in text:
        return
    if text.count(old) != 1:
        sys.exit(f"{rel}: anchor found {text.count(old)} times: {old[:60]!r}")
    path.write_text(text.replace(old, new, 1))
    print(f"patched {rel}")


# --- video.c: present into the headset, vsync from the XR thread -------------
edit("src/sdl/video.c",
     '#include "video_internal.h"\n',
     '#include "video_internal.h"\n'
     '#ifdef WC1_QUEST\n#include "../quest/quest_xr.h"\n#endif\n',
     '../quest/quest_xr.h')
edit("src/sdl/video.c",
     "    SdlShutdownVideo();\n    if (SdlUsingGlRenderer())\n",
     "    SdlShutdownVideo();\n"
     "#ifdef WC1_QUEST\n"
     "    /* The XR thread owns presentation; the Android window stays unused. */\n"
     "    (void)window;\n    return 1;\n#endif\n"
     "    if (SdlUsingGlRenderer())\n",
     "XR thread owns presentation")
edit("src/sdl/video.c",
     "        palette = g_abEgaDitherPalette;\n    }\n    if (SdlUsingGlRenderer())\n"
     "        return SdlGlRendererPresent(pixels, palette);\n",
     "        palette = g_abEgaDitherPalette;\n    }\n"
     "#ifdef WC1_QUEST\n"
     "    if (pixels == 0 || palette == 0)\n        return 0;\n"
     "    QuestXr_SubmitIndexedFrame(pixels, palette);\n    return 1;\n#endif\n"
     "    if (SdlUsingGlRenderer())\n"
     "        return SdlGlRendererPresent(pixels, palette);\n",
     "QuestXr_SubmitIndexedFrame(pixels, palette)")
edit("src/sdl/video.c",
     "    SDL_Rect dest;\n\n    if (SdlUsingGlRenderer()) {\n"
     "        SdlGlRendererWaitForVerticalBlank();\n",
     "    SDL_Rect dest;\n\n"
     "#ifdef WC1_QUEST\n    (void)dest;\n    QuestXr_WaitForDisplayTick();\n    return;\n#endif\n"
     "    if (SdlUsingGlRenderer()) {\n"
     "        SdlGlRendererWaitForVerticalBlank();\n",
     "QuestXr_WaitForDisplayTick();")

# --- events.c: drain headset input every pump; injection helpers ------------
edit("src/sdl/events.c",
     "    SdlServiceOriginFxMusic();\n    while (SDL_PollEvent(&event)) {\n",
     "    SdlServiceOriginFxMusic();\n"
     "#ifdef WC1_QUEST\n    SdlQuestServiceInput();\n#endif\n"
     "    while (SDL_PollEvent(&event)) {\n",
     "    SdlQuestServiceInput();\n")
edit("src/sdl/events.c",
     "void SdlStartEventPump(void)\n{\n",
     "#ifdef WC1_QUEST\n"
     "/* Laser pointer -> the same logical mouse path SdlHandleMouseEvent uses.\n"
     " * buttonEvent: 0 = motion, 2 = button down, 1 = button up. */\n"
     "void SdlQuestInjectMouse(int x, int y, int primaryButton,\n"
     "                         int secondaryButton, int buttonEvent)\n"
     "{\n"
     "    if (x < 0)\n        x = 0;\n    else if (x > 319)\n        x = 319;\n"
     "    if (y < 0)\n        y = 0;\n    else if (y > 199)\n        y = 199;\n"
     "    if (buttonEvent == 0) {\n"
     "        if (bPtrMovedByKeyboardState != 0) {\n"
     "            bPtrMovedByKeyboardState = 0;\n            return;\n        }\n"
     "        SdlQueueMouseMotion((unsigned short)x, (unsigned short)y,\n"
     "                            primaryButton, secondaryButton);\n"
     "    } else {\n"
     "        QueueInputEvent((unsigned short)buttonEvent, (unsigned short)x,\n"
     "                        (unsigned short)y, 0, primaryButton,\n"
     "                        secondaryButton, 0);\n"
     "    }\n"
     "    nHostMouseMessageX = x;\n    nHostMouseMessageY = y;\n"
     "    bHostPrimaryMouseButton = primaryButton;\n"
     "    bHostSecondaryMouseButton = secondaryButton;\n"
     "}\n"
     "#endif\n\n"
     "void SdlStartEventPump(void)\n{\n",
     "void SdlQuestInjectMouse(")

# --- joystick.c: only the virtual Touch pad; piloting query ---------------------
edit("src/sdl/joystick.c",
     "    instanceId = SDL_JoystickGetDeviceInstanceID(deviceIndex);\n"
     "    if (instanceId < 0 || SdlFindJoystick(instanceId) != -1)\n        return;\n",
     "    instanceId = SDL_JoystickGetDeviceInstanceID(deviceIndex);\n"
     "    if (instanceId < 0 || SdlFindJoystick(instanceId) != -1)\n        return;\n"
     "#ifdef WC1_QUEST\n"
     "    /* Android also lists the Touch controllers as gamepads; they never\n"
     "     * report input in an immersive session.  Use only the virtual pad. */\n"
     "    if (!SdlQuestIsQuestPad(deviceIndex))\n        return;\n#endif\n",
     "SdlQuestIsQuestPad(deviceIndex)")
edit("src/sdl/joystick.c",
     "void SdlEndJoystickSpaceflight(void)\n{\n",
     "#ifdef WC1_QUEST\n"
     "int SdlQuestIsPiloting(void)\n{\n"
     "    return g_bSdlJoystickSpaceflightActive &&\n"
     "        pCurrentEventManagerPump == get_player_input &&\n"
     "        nArcadeState == 0;\n}\n#endif\n\n"
     "void SdlEndJoystickSpaceflight(void)\n{\n",
     "int SdlQuestIsPiloting(void)")

# --- launcher.c: Quest start-up --------------------------------------------------
edit("src/sdl/launcher.c",
     '#include "slint/launcher_api.h"\n',
     '#include "slint/launcher_api.h"\n'
     '#ifdef WC1_QUEST\n#include "../quest/quest_xr.h"\n#include <android/log.h>\n'
     'int SdlQuestStartup(int *argumentCount, char ***arguments);\n'
     'void SdlQuestCreatePad(void);\n#endif\n',
     '../quest/quest_xr.h')
edit("src/sdl/launcher.c",
     "#ifdef _WIN32\n    ImmDisableIME((DWORD)-1);\n#endif\n"
     "    if (!SdlParsePortArguments(",
     "#ifdef _WIN32\n    ImmDisableIME((DWORD)-1);\n#endif\n"
     "#ifdef WC1_QUEST\n"
     "    /* Logging, data directory and Quest default options. */\n"
     "    if (!SdlQuestStartup(&argumentCount, &arguments))\n        return 1;\n"
     "#endif\n"
     "    if (!SdlParsePortArguments(",
     "SdlQuestStartup(&argumentCount")
edit("src/sdl/launcher.c",
     "        return 1;\n    }\n\n    windowFlags = SDL_WINDOW_RESIZABLE;\n",
     "        return 1;\n    }\n"
     "#ifdef WC1_QUEST\n"
     "    if (!QuestXr_Start()) {\n"
     "        fprintf(stderr, \"OpenXR start failed\\n\");\n"
     "        SDL_Quit();\n        return 1;\n    }\n"
     "    SdlQuestCreatePad();\n"
     "#endif\n\n"
     "    windowFlags = SDL_WINDOW_RESIZABLE;\n",
     "    SdlQuestCreatePad();\n")
edit("src/sdl/launcher.c",
     "    DIBunInstall();\n    SdlShutdownJoysticks();\n    SDL_DestroyWindow(window);\n"
     "    SDL_Quit();\n",
     "    DIBunInstall();\n    SdlShutdownJoysticks();\n    SDL_DestroyWindow(window);\n"
     "#ifdef WC1_QUEST\n    QuestXr_Shutdown();\n#endif\n"
     "    SDL_Quit();\n",
     "    QuestXr_Shutdown();\n")

# --- wc1sdl.h: prototypes ---------------------------------------------------------
edit("include/wc1sdl.h",
     "#ifndef _WIN32\nint SdlOpen(",
     "#ifdef WC1_QUEST\n"
     "void SdlQuestInjectMouse(int x, int y, int primaryButton,\n"
     "                         int secondaryButton, int buttonEvent);\n"
     "int SdlQuestIsPiloting(void);\n"
     "void SdlQuestServiceInput(void);\n"
     "int SdlQuestIsQuestPad(int deviceIndex);\n"
     "#endif\n\n"
     "#ifndef _WIN32\nint SdlOpen(",
     "void SdlQuestInjectMouse(int x")
# --- stage 2: stereo capture --------------------------------------------------
edit("src/sdl/video.c",
     "    int fullViewportCopy, unsigned char clearColour)\n{\n    if (SdlUsingGlRenderer()) {\n        SdlGlRendererBeginSpaceFrame(\n",
     "    int fullViewportCopy, unsigned char clearColour)\n{\n"
     "#ifdef WC1_QUEST\n"
     "    SdlQuestCaptureSpaceFrame(geometry, viewportMode, fullViewportCopy,\n"
     "                              clearColour);\n"
     "#endif\n"
     "    if (SdlUsingGlRenderer()) {\n        SdlGlRendererBeginSpaceFrame(\n",
     "SdlQuestCaptureSpaceFrame(geometry")
edit("src/sdl/video.c",
     "    SdlEndJoystickSpaceflight();\n    if (SdlUsingGlRenderer())\n        SdlGlRendererCancelSpaceFrame();\n",
     "    SdlEndJoystickSpaceflight();\n"
     "#ifdef WC1_QUEST\n    SdlQuestCancelStereo();\n#endif\n"
     "    if (SdlUsingGlRenderer())\n        SdlGlRendererCancelSpaceFrame();\n",
     "SdlQuestCancelStereo();")
edit("src/sdl/video.c",
     "    QuestXr_SubmitIndexedFrame(pixels, palette);\n    return 1;\n",
     "    if (SdlQuestPresentStereo(pixels, palette))\n        return 1;\n"
     "    QuestXr_SubmitIndexedFrame(pixels, palette);\n    return 1;\n",
     "SdlQuestPresentStereo(pixels, palette)")
edit("src/eventmgr.c",
     "                screenY = (short)(screenY + nCurrentViewCenterPosY);\n"
     "                asObjectDrawPosY[obj] = screenY;\n"
     "                if (shape != 0) {\n",
     "                screenY = (short)(screenY + nCurrentViewCenterPosY);\n"
     "                asObjectDrawPosY[obj] = screenY;\n"
     "#ifdef WC1_QUEST\n"
     "                /* Drawn in 3D by the VR renderer instead. */\n"
     "                if (SdlQuestStereoOwnsObject(obj))\n"
     "                    break;\n"
     "#endif\n"
     "                if (shape != 0) {\n",
     "Drawn in 3D by the VR renderer instead.")
edit("src/eventmgr.c",
     "                if (specialObject == obj)\n"
     "                    shape = apObjShapeArray[obj];\n"
     "                else\n"
     "                    shape = pCurrentConstellationShape;\n",
     "                if (specialObject == obj)\n"
     "                    shape = apObjShapeArray[obj];\n"
     "                else\n"
     "                    shape = pCurrentConstellationShape;\n"
     "#ifdef WC1_QUEST\n"
     "                /* Stars come from the VR star sphere; dust is 3D. */\n"
     "                if (specialObject != obj &&\n"
     "                    (objectClass == OBJECT_CLASS_STAR ||\n"
     "                     SdlQuestStereoOwnsObject(obj)) &&\n"
     "                    SdlQuestStereoOwnsObject(-2))\n"
     "                    break;\n"
     "#endif\n",
     "Stars come from the VR star sphere")
edit("include/wc1sdl.h",
     "int SdlQuestTakeShift(void);\n#endif",
     "int SdlQuestTakeShift(void);\n"
     "struct ScreenViewportGeometry;\n"
     "void SdlQuestCaptureSpaceFrame(const struct ScreenViewportGeometry *geometry,\n"
     "                               int viewportMode, int fullViewportCopy,\n"
     "                               unsigned char clearColour);\n"
     "int SdlQuestStereoOwnsObject(int obj);\n"
     "void SdlQuestCancelStereo(void);\n"
     "int SdlQuestPresentStereo(const unsigned char *pixels,\n"
     "                          const unsigned char *palette);\n"
     "#endif",
     "int SdlQuestStereoOwnsObject(int obj);")
# --- Yes/No prompts: A = yes, B = no on Touch -------------------------------
edit("src/barracks.c",
     "    DrawModalTextPanel(pModalTextPanel, 0, 6, 2, text);\n    DIBslam();\n    DIBslamReal();\n    return 1;\n",
     "    DrawModalTextPanel(pModalTextPanel, 0, 6, 2, text);\n    DIBslam();\n    DIBslamReal();\n"
     "#ifdef WC1_QUEST\n    SdlQuestSetYesNoPrompt(strstr(text, \"(Y/N)\") != 0);\n#endif\n"
     "    return 1;\n",
     "SdlQuestSetYesNoPrompt(strstr(text")
edit("src/barracks.c",
     "void ReleaseModalTextPanel(void)\n{\n",
     "void ReleaseModalTextPanel(void)\n{\n#ifdef WC1_QUEST\n    SdlQuestSetYesNoPrompt(0);\n#endif\n",
     "SdlQuestSetYesNoPrompt(0);")
edit("include/wc1sdl.h",
     "int SdlQuestStereoOwnsObject(int obj);\n",
     "int SdlQuestStereoOwnsObject(int obj);\nvoid SdlQuestSetYesNoPrompt(int active);\n",
     "void SdlQuestSetYesNoPrompt(int active);")
print("done")
