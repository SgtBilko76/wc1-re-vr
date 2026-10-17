/*
 *  Ejection and stranded-pilot sequences.
 *
 *  Address range 4046a0-404ccf. The ejection data and literals precede the
 *  campaign unit's mouth-animation table; see docs/ORDER.md.
 */
#include "wc1.h"

static short asEjectionPrimaryFrames[8] = {
    0, 1, 1, 3, 3, 0, 0, 0
}; /* 0x00465550 */

static short asEjectionSecondaryFrames[8] = {
    -1, -1, 2, -1, 4, 0, 0, 0
}; /* 0x00465560 */

static short asEjectionViewScript[12] = {
    3, 11, 14, 70, 3, 10, 14, 80, 3, 4, -1, 0
}; /* 0x00465570 */

static const char *pStrandedMessage =
    "\nWith your carrier\ndestroyed, you drift\nendlessly through\n"
    "the void..."; /* 0x00465588 */

/* Function start: 0x4046A0 */
unsigned int ejection_sequence(void)
{
    FixedVector viewOffset;
    unsigned char *background;
    unsigned char *ejectionShape;
    short frame;
    short y;
    short descentSpeed;
    short spriteFrame;

    free_all_slots();
    free_cockpit();
    PreloadMusicTrackHook(0x1f);
    spacetrack(0x1f, 2, 1);
    frame = 0;
    new_view(9, 0);
    background = FetchDiskPacketRetrying(
        (short)cCockpitLogicalFile, 3, 0);
    ejectionShape = FetchDiskPacketRetrying(2, 1, 0);
    PlaySfxWaveFileByNumber(0x21, -1, 0);
    y = 199;
    bEscapePressed = 0;
    descentSpeed = 4;
    nFrameSkipCounter = 1;
    do {
        if (RefreshCockpitStatus() != 0) {
            DrawSpriteDefault(&stSpaceBuffer, 0, 0, background, 0);
            spriteFrame = MinShort(frame, 4);
            DrawSpriteDefault(
                &stSpaceBuffer, 160, y, ejectionShape,
                asEjectionPrimaryFrames[spriteFrame]);
            if (asEjectionSecondaryFrames[spriteFrame] != -1)
                DrawSpriteDefault(
                    &stSpaceBuffer, 160, y, ejectionShape,
                    asEjectionSecondaryFrames[spriteFrame]);
            DrawSpriteDefault(&stSpaceBuffer, 160, (short)(y + 1),
                              ejectionShape, 5);
            dump_buffer_to_screen();
        }
        if (frame > 1) {
            y = (short)(y - descentSpeed);
            descentSpeed = MinShort((short)(descentSpeed + 4), 20);
        }
        if (bEscapePressed == 1)
            break;
        frame++;
        DIBslam();
        DIBslamReal();
    } while (frame < 10);

    ReleasePacketHandle(ejectionShape);
    ReleasePacketHandle(background);
    GetScreenUpdateFlag();
    if (bEscapePressed != 1) {
        PromptInsertNumberedDisk(8);
        pScreenViewportPacket =
            AllocateTaggedMemory(
                GetPacketSize(pDiskFileRecords[8].name, 8),
                0x40);
        if (pScreenViewportPacket == 0)
            ReportOutOfMemoryAndExit("View Templates");
        else
            LoadPacketIntoBuffer(8, 8,
                                 pScreenViewportPacket);

        aObjectTypeData[OBJECT_TYPE_EJECTED_PILOT].shapeSet =
            FetchDiskPacketRetrying(2, 2, 0);
        nEjectedPilotObject = find_vacant_3d_object();
        set_objects_data(nEjectedPilotObject,
                         OBJECT_TYPE_EJECTED_PILOT, -1);
        asObjectCounter[nEjectedPilotObject] =
            32000;
        copy_frame(0, nEjectedPilotObject);
        aShipPosition[nEjectedPilotObject] =
            aShipPosition[0];
        ScaleFixedVector(
            &aShipUpVector[nEjectedPilotObject],
            -0x500,
            &aShipVelocity[nEjectedPilotObject]);
        AddFixedVectors(
            &aShipVelocity[nEjectedPilotObject],
            &aShipVelocity[0],
            &aShipVelocity[nEjectedPilotObject]);
        new_view(10, nEjectedPilotObject);

        background = FetchDiskPacketRetrying(
            (short)cCockpitLogicalFile, 0, 0);
        ejectionShape = FetchDiskPacketRetrying(
            (short)cCockpitLogicalFile, 5, 0);
        y = 40;
        frame = 0;
        PlaySfxWaveFileByNumber(0x22, -1, 0);
        nFrameSkipCounter = 1;
        do {
            if (RefreshCockpitStatus() != 0) {
                DrawSpriteDefault(&stSpaceBuffer, 0, y,
                                  background, 0);
                DrawSpriteDefault(&stSpaceBuffer, 0, (short)(y - 1),
                                  ejectionShape, 0);
                dump_buffer_to_screen();
            }
            if (bEscapePressed == 1)
                break;
            y = (short)(y + descentSpeed);
            DIBslam();
            frame++;
            DIBslamReal();
        } while (frame < 10);

        ReleasePacketHandle(ejectionShape);
        ReleasePacketHandle(background);
        if (bEscapePressed != 1) {
            load_all_slots();
            aShipForwardVector[EYE_OBJECT] =
                aShipUpVector[0];
            aShipRightVector[EYE_OBJECT] =
                aShipRightVector[0];
            aShipUpVector[EYE_OBJECT] =
                aShipForwardVector[0];
            negate_vector(&aShipUpVector[EYE_OBJECT]);
            ScaleFixedVector(
                &aShipUpVector[nEjectedPilotObject],
                -0x25800, &viewOffset);
            AddFixedVectors(
                &aShipPosition[nEjectedPilotObject],
                &viewOffset,
                &aShipPosition[EYE_OBJECT]);
            nScriptedViewObject =
                nEjectedPilotObject;
            initialize_scripted_view(asEjectionViewScript);
            frame = 0;
            nFrameSkipCounter = 1;
            SetMusBreakpt(0, 0);
            while (1) {
                alter_pitch(4, nEjectedPilotObject);
                if (RefreshCockpitStatus() != 0)
                    dump_buffer_to_screen();
                if (frame == 10) {
                    Explosion(0);
                    PlaySfxWaveFileByNumber(4, -1, 0);
                }
                frame++;
                if (frame > 200 || bEscapePressed != 0)
                    break;
                DIBslam();
                DIBslamReal();
            }
        }
    }

    bEscapePressed = 0;
    bScriptedView = 0;
    if (pScreenViewportPacket != 0) {
        ReleasePacketHandle(pScreenViewportPacket);
        pScreenViewportPacket = 0;
    }
    FadeViewportPaletteToColour(&stScreen, cBlackColour, 1);
    ClearViewport(&stScreen, cBlackColour);
    DIBslam();
    DIBslamReal();
    RestoreGamePalette();
    free_all_slots();
    StopMusicUnlessSuppressed();
    ReleaseMusicTrackHook(0x1f);
    return 0;
}

/* Function start: 0x404BE0 */
void stranded_sequence(void)
{
    short frame;

    nCannedSceneMode = 1;
    free_cockpit();
    force_view(13, 0);
    pIntroFont = FetchDiskPacketRetrying(9, 1, 0);
    frame = 0;
    do {
        if (RefreshCockpitStatus() != 0) {
            if (frame >= 300)
                print_subtitle(&stSpaceBuffer, 56,
                               "THE END");
            else if (frame >= 160)
                print_subtitle(&stSpaceBuffer, 50,
                               pStrandedMessage);
            dump_buffer_to_screen();
        }
        if (bEscapePressed != 0)
            break;
        frame++;
        DIBslam();
        DIBslamReal();
    } while (frame < 400);
    free_all_slots();
    FreePacketAndClear(&pIntroFont, 0);
    stScreen.top = 0;
    stScreen.bottom = 199;
    FadeViewportPaletteToColour(&stScreen, cBlackColour, 1);
    ClearViewport(&stScreen, cBlackColour);
    RestoreGamePalette();
    bEscapePressed = 0;
}
