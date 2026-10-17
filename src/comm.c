/*
 *  Communication menus, input polling, and graphics-mode setup.
 *
 *  Address range 430710-431eff (see docs/ORDER.md).
 *  Internal boundaries remain provisional.
 */
#include "wc1.h"

/* Function start: 0x430710 */
short __stdcall UpdateInputDeviceTransitions(short raw)
{
    unsigned int now;
    unsigned short buttons;
    unsigned short changed;
    unsigned short previous;
    short device;

    device = nActiveInputDevice;
    if (raw == 0)
        ReadCalibratedJoystick();
    else
        SampleJoystickDevice(&aInputDeviceSamples[device],
                             device, 0);

    asInputButton2DoubleClick[device] = 0;
    asInputButton1DoubleClick[device] = 0;
    asInputButton2Changed[device] = 0;
    asInputButton1Changed[device] = 0;
    buttons = (unsigned short)aInputDeviceSamples[device].buttons;
    previous = (unsigned short)asPreviousInputButtons[device];
    if (buttons != previous) {
        changed = buttons ^ previous;
        now = nTickCount60Hz;
        if ((changed & 1) != 0) {
            asInputButton1Changed[device] = 1;
            if ((buttons & 1) != 0) {
                if ((int)(now -
                    anInputButton1PressTime[device]) <=
                        nInputDoubleClickInterval *
                        nInputTickScale)
                    asInputButton1DoubleClick[device] = 1;
                anInputButton1PressTime[device] = now;
            }
        }
        if ((changed & 2) != 0) {
            asInputButton2Changed[device] = 1;
            if ((buttons & 2) != 0) {
                if ((int)(now -
                    anInputButton2PressTime[device]) <=
                        nInputDoubleClickInterval *
                        nInputTickScale)
                    asInputButton2DoubleClick[device] = 1;
                anInputButton2PressTime[device] = now;
            }
        }
        asPreviousInputButtons[device] = (short)buttons;
    }
    return 1;
}

/* Function start: 0x430830 */
/* Empty, and nothing in the shipped executable calls it, so its purpose
 * cannot be recovered; it sits among the joystick input routines. */
void JoystickInputHook(void)
{
}

/* Function start: 0x430840 */
void PollJoystickButtonEvents(void)
{
    short doubleClick;

    if (nActiveInputDevice == -1 ||
        bInputPollingGuard != 0)
        return;
    bInputPollingGuard++;
    UpdateInputDeviceTransitions(1);
    if (asInputButton1Changed
            [nActiveInputDevice] != 0) {
        doubleClick = 0;
        if (asInputButton1DoubleClick
                [nActiveInputDevice] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)aInputDeviceSamples
                [nActiveInputDevice].buttons & 1) + 1,
            0, doubleClick);
    }
    if (asInputButton2Changed
            [nActiveInputDevice] != 0) {
        doubleClick = 0;
        if (asInputButton2DoubleClick
                [nActiveInputDevice] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)(aInputDeviceSamples
                [nActiveInputDevice].buttons >> 1) & 1) + 1,
            1, doubleClick);
    }
    bInputPollingGuard--;
    stHostMouseState.secondaryButton =
        (unsigned char)aInputDeviceSamples
            [nActiveInputDevice].buttons;
    stHostMouseState.primaryButton =
        stHostMouseState.secondaryButton;
}

/* Function start: 0x430920 */
void PollMenuInputDevices(void)
{
    signed char changes;
    char doubleClick;
    short movementX;
    short movementY;
    int deviceIndex;

    changes = 0;
    if (nActiveInputDevice == -1)
        return;
    stHostMouseState.x =
        stMouseCursorState.x;
    stHostMouseState.y =
        stMouseCursorState.y;
    if (bInputPollingGuard != 0)
        return;
    bInputPollingGuard++;
    UpdateInputDeviceTransitions(0);

    deviceIndex = (int)nActiveInputDevice;
    if (asInputButton1Changed[deviceIndex] != 0) {
        stHostMouseState.x +=
            (short)aInputDeviceSamples
                [deviceIndex].x *
            nMenuPointerSpeed;
        stHostMouseState.y +=
            (short)aInputDeviceSamples
                [deviceIndex].y *
            nMenuPointerSpeed;
        if (asInputButton1DoubleClick[deviceIndex] != 0)
            changes = 3;
        QueueInputEventAtCursor(
            ((unsigned short)aInputDeviceSamples
                [deviceIndex].buttons & 1) + 1,
            0, (short)changes);
        changes = 1;
    }
    deviceIndex = (int)nActiveInputDevice;
    if (asInputButton2Changed[deviceIndex] != 0) {
        stHostMouseState.x +=
            (short)aInputDeviceSamples
                [deviceIndex].x *
            nMenuPointerSpeed;
        stHostMouseState.y +=
            (short)aInputDeviceSamples
                [deviceIndex].y *
            nMenuPointerSpeed;
        doubleClick = 0;
        if (asInputButton1DoubleClick[deviceIndex] != 0)
            doubleClick = 3;
        QueueInputEventAtCursor(
            ((unsigned short)(aInputDeviceSamples
                [deviceIndex].buttons >> 1) & 1) + 1,
            1, (short)doubleClick);
        changes++;
    }
    if (changes == 0) {
        movementX = (short)aInputDeviceSamples
            [nActiveInputDevice].x;
        movementY = (short)aInputDeviceSamples
            [nActiveInputDevice].y;
        stHostMouseState.x +=
            nMenuPointerSpeed * movementX;
        stHostMouseState.y +=
            nMenuPointerSpeed * movementY;
        changes = (unsigned char)movementY | (unsigned char)movementX;
        if (changes != 0) {
            FlushInputEvents();
            stHostMouseState.x = stHostMouseState.x > 0 ?
                stHostMouseState.x : 0;
            stHostMouseState.x = stHostMouseState.x < 319 ?
                stHostMouseState.x : 319;
            stHostMouseState.y = stHostMouseState.y > 0 ?
                stHostMouseState.y : 0;
            stHostMouseState.y = stHostMouseState.y < 199 ?
                stHostMouseState.y : 199;
            SetMousePosition(stHostMouseState.x,
                             stHostMouseState.y);
        }
    }

    stHostMouseState.secondaryButton =
        (unsigned char)aInputDeviceSamples
            [nActiveInputDevice].buttons;
    stHostMouseState.primaryButton =
        stHostMouseState.secondaryButton;
    stHostMouseState.x = stHostMouseState.x > 0 ?
        stHostMouseState.x : 0;
    stHostMouseState.x = stHostMouseState.x < 319 ?
        stHostMouseState.x : 319;
    stHostMouseState.y = stHostMouseState.y > 0 ?
        stHostMouseState.y : 0;
    stHostMouseState.y = stHostMouseState.y < 199 ?
        stHostMouseState.y : 199;
    if (changes != 0) {
        LeaveAllocationScope();
        stMouseCursorState.primaryButton =
            stHostMouseState.primaryButton;
        stMouseCursorState.x =
            stHostMouseState.x;
        stMouseCursorState.y =
            stHostMouseState.y;
        stMouseCursorState.secondaryButton =
            stHostMouseState.secondaryButton;
        stMouseCursorState.flags =
            stHostMouseState.flags;
        EnterAllocationScope();
    }
    bInputPollingGuard--;
}

/* Function start: 0x430BC0 */
short get_face(short rating, unsigned int side)
{
    if (rating == -1)
        return 13 + (side < SIDE_KILRATHI ? -1 : 0);
    if (side == SIDE_KILRATHI)
        rating--;
    return rating;
}

/* Function start: 0x430BF0 */
void LoadCommPortraitShape(short face, signed char alternate)
{
    short section;

    if (face >= 0 && face <= 7)
        section = face + 1;
    else if ((face > 7 && face < 12) || face == 13)
        section = 10;
    else
        section = -1;
    if (section != -1)
        apCommPortraitShapes[face] =
            FetchDiskPacketRetrying(11, section,
                                                     (short)alternate);
}

/* Function start: 0x430C50 */
void ResetCommMenuChoices(short reuse)
{
    short choice;

    if (reuse == 0) {
        memset(abCommMenuChoiceCommand, -1,
               sizeof(abCommMenuChoiceCommand));
        choice = 0;
        do {
            apszCommMenuChoiceText[choice] = 0;
            choice++;
        } while (choice < 7);
    }
    nCommMenuChoiceCount = 0;
    nCommMenuReuseMode = reuse;
}

/* Function start: 0x430CA0 */
short IsCommMenuIdle(void)
{
    return nCommMenuChoiceCount == 0;
}

/* Function start: 0x430CB0 */
void AppendCommMenuChoice(const char *text, short command)
{
    short index;

    if (nCommMenuReuseMode == 1) {
        index = nCommMenuChoiceCount;
        if (apszCommMenuChoiceText[index] != text ||
            abCommMenuChoiceCommand[index] != command)
            nCommMenuReuseMode = 0;
    } else {
        index = nCommMenuChoiceCount;
    }
    apszCommMenuChoiceText[index] = text;
    nCommMenuChoiceCount = index + 1;
    abCommMenuChoiceCommand[index] = (signed char)command;
}

/* Function start: 0x430D30 */
void SendCommMenuChoice(short i)
{
    AppendCommMenuChoice(apszCommMenuText[i], i);
}

/* Function start: 0x430D50 */
void OpenCommMenuForTarget(const char *heading, const char *message)
{
    CockpitMessage(message, cYellowColour, -1);
    pszCommMenuHeading = heading;
}

/* Function start: 0x430D80 */
short IsCommChoiceMenuOpen(void)
{
    return get_mode(1) == 4;
}

/* Function start: 0x430DA0 */
short GetPendingMenuAction(void)
{
    return (short)cPendingCommMenuAction;
}

/* Function start: 0x430DB0 */
void SetPendingMenuAction(unsigned char v)
{
    cPendingCommMenuAction = (signed char)v;
}

/* Function start: 0x430DC0 */
void OpenCommRecipientMenu(void)
{
    push_mode(1, 4);
    SetPendingMenuAction(1);
}

/* Function start: 0x430DE0 */
void CloseCommChoiceMenu(void)
{
    if (get_mode(1) == 4) {
        pop_mode(1);
        return;
    }
    ShutdownEventManager();
    exit_squadron("!stop");
}

/* Function start: 0x430E10 */
short wingman_dead(void)
{
    return nYourWingman == -1;
}

/* Function start: 0x430E30 */
short have_target(void)
{
    return unactive(acShipTarget[0]) == 0;
}

/* Function start: 0x430E50 */
unsigned short CanOpenCommMenu(void)
{
    if (have_target() == 0) {
        if (wingman_dead())
            return 0;
    }
    return 1;
}

/* Function start: 0x430E70 */
void SelectCommRecipient(short recipient)
{
    cCommMenuRecipient = recipient;
    SetPendingMenuAction(2);
}

/* Function start: 0x430E90 */
void BuildCommunicationRecipientMenu(void)
{
    short target;

    ResetCommMenuChoices(nCommMenuReuseMode);
    OpenCommMenuForTarget("VID-COM SYSTEM\n\nSend message to?\n\n",
                          "SELECT");
    if (wingman_dead() != 0) {
        SelectCommRecipient(acShipTarget[0]);
        return;
    }
    if (have_target() == 0 ||
        acShipTarget[0] == nYourWingman) {
        SelectCommRecipient(nYourWingman);
        return;
    }
    AppendCommMenuChoice(
        apWingmanPilots[
            (signed char)acShipRating[
                nYourWingman]]->callsign,
        1);
    target = acShipTarget[0];
    if (target != -1) {
        if (aeShipSide[target] == SIDE_KILRATHI &&
            aeObjectClass[target] == OBJECT_CLASS_SHIP)
            AppendCommMenuChoice("ENEMY TARGET", 2);
        else if (aeShipSide[target] == SIDE_IMPERIAL &&
                 ((aeObjectClass[target] == OBJECT_CLASS_SHIP &&
                   any_enemy(0, 14000) != 0) ||
                  aeObjectType[target] ==
                      OBJECT_TYPE_TIGERS_CLAW))
            AppendCommMenuChoice(
                aObjectTypeData[
                    aeObjectType[acShipTarget[0]]].displayName,
                3);
    }
    SendCommMenuChoice(0);
}

/* Function start: 0x430FC0 */
void BuildCommunicationCommandMenu(void)
{
    signed char rating;
    const char *name;

    ResetCommMenuChoices(nCommMenuReuseMode);
    if (cCommMenuRecipient == nYourWingman) {
        if (aeShipObjective[nYourWingman] ==
                OBJECTIVE_HOLD_FORMATION &&
            any_enemy(0, 14000) != 0) {
            SendCommMenuChoice(7);
        }
        if (nAutoEngageTimer != -1)
            SendCommMenuChoice(8);
        else if (aeShipObjective[nYourWingman] !=
                     OBJECTIVE_HOLD_FORMATION)
            SendCommMenuChoice(9);
        if (bRadioSilence != 0)
            SendCommMenuChoice(11);
        else
            SendCommMenuChoice(10);
    }
    if (aeShipSide[cCommMenuRecipient] ==
            aeShipSide[0]) {
        if (aeObjectType[cCommMenuRecipient] ==
                OBJECT_TYPE_TIGERS_CLAW &&
            bLandingAuthorized == 0) {
            SendCommMenuChoice(12);
        }
        if (have_target() != 0 &&
            aeShipSide[acShipTarget[0]] ==
                SIDE_KILRATHI) {
            SendCommMenuChoice(1);
        }
        if (evaluate_damage(0) < 50 && any_enemy(0, 14000) != 0)
            SendCommMenuChoice(2);
    }
    if (cCommMenuRecipient == nYourWingman)
        SendCommMenuChoice(3);
    if (aeShipSide[cCommMenuRecipient] ==
            SIDE_KILRATHI) {
        SendCommMenuChoice(4);
        SendCommMenuChoice(5);
        SendCommMenuChoice(6);
    }
    if (IsCommMenuIdle() != 0)
        CloseCommChoiceMenu();
    else
        SendCommMenuChoice(0);

    if (IsCommChoiceMenuOpen() != 0) {
        strcpy(szCommMenuHeadingBuffer,
               "VID-COM SYSTEM\n\nTo: ");
        rating = acShipRating[cCommMenuRecipient];
        if (rating == -1) {
            name = aObjectTypeData[
                aeObjectType[
                    cCommMenuRecipient]].displayName;
        } else if (rating < 8) {
            name = apWingmanPilots[rating]->callsign;
        } else {
            name = apszKilrathiAceNames[rating - 9];
        }
        strcat(szCommMenuHeadingBuffer, name);
        strcat(szCommMenuHeadingBuffer, "\n");
        OpenCommMenuForTarget(szCommMenuHeadingBuffer,
                              "CHOOSE");
    }
}

/* Function start: 0x431200 */
void RefreshCommunicationMenu(void)
{
    if (IsCommChoiceMenuOpen() != 0) {
        if (GetPendingMenuAction() == 1)
            BuildCommunicationRecipientMenu();
        if (GetPendingMenuAction() == 2)
            BuildCommunicationCommandMenu();
        if (nCommMenuReuseMode == 0)
            InvalidateVduMode(1);
    }
}

/* Function start: 0x431240 */
void HandleCommunicationMenuRequest(void)
{
    if (IsCommChoiceMenuOpen() != 0)
        CloseCommChoiceMenu();
    if (message_showing() == 0 &&
        IsCommChoiceMenuOpen() == 0 && CanOpenCommMenu() != 0) {
        OpenCommRecipientMenu();
        ResetCommMenuChoices(0);
        RefreshCommunicationMenu();
    }
}

/* Function start: 0x431290 */
void show_communications_disp(void)
{
    signed char choice;
#ifdef SDL_PORT
    int selectedChoice;
    unsigned char normalColour;
#endif

    if (IsCommChoiceMenuOpen() == 0)
        HandleCommunicationMenuRequest();
    if (IsCommChoiceMenuOpen() != 0) {
        set_new_vdu(1);
        DrawTextAt(&stRightVduTextContext, stRightVdu.left,
                   stRightVdu.top, pszCommMenuHeading, 2);
        choice = 0;
#ifdef SDL_PORT
        selectedChoice = SdlGetCommunicationMenuSelection();
        normalColour = stRightVduTextContext.colour;
#endif
        if (nCommMenuChoiceCount > 0) {
            do {
#ifdef SDL_PORT
                if ((int)choice == selectedChoice)
                    stRightVduTextContext.colour = cYellowColour;
#endif
                DrawFormattedText("\n%d %s", (int)choice + 1,
                                  apszCommMenuChoiceText[
                                      (int)choice]);
#ifdef SDL_PORT
                stRightVduTextContext.colour = normalColour;
#endif
                choice++;
            } while (choice < nCommMenuChoiceCount);
        }
        DrawSpriteDefault(&stRightVdu,
                          (short)(stRightVdu.left + 36),
                          (short)(stRightVdu.top + 10),
                          pCommMenuCursorShape, 0x19);
        nCommMenuReuseMode = 1;
    }
}

/* Function start: 0x431350 */
unsigned int Chosen_communicate_option(short choice)
{
    int action;

    PlaySfxWaveFileByNumber(0x19, -1, 0);
    action = GetPendingMenuAction();
    switch (action) {
    case 0:
        CloseCommChoiceMenu();
        return 0;
    case 1:
        if (abCommMenuChoiceCommand[choice] == 0) {
            CloseCommChoiceMenu();
            return 0;
        }
        if (abCommMenuChoiceCommand[choice] == 1) {
            SelectCommRecipient(nYourWingman);
            RefreshCommunicationMenu();
            return 0;
        }
        SelectCommRecipient(acShipTarget[0]);
        RefreshCommunicationMenu();
        return 0;
    case 2:
        CloseCommChoiceMenu();
        request(0, (short)cCommMenuRecipient,
                (short)abCommMenuChoiceCommand[choice]);
        return 0;
    }
    return 0;
}

/* Function start: 0x431400 */
void talk_equiv(void)
{
    RefreshCommunicationMenu();
}

/* Function start: 0x431410 */
void FreeCommDisplayResources(void)
{
#ifdef SDL_PORT
    /* The original indexes the portrait table with its -1 inactive sentinel,
       aliasing the final two palette-allocation words at 0x0059E17C. */
    if (nCommPortraitIndex != -1)
#endif
    FreePacketAndClear(&apCommPortraitShapes[
        nCommPortraitIndex], 0);
    FreePacketAndClear(&pConfedCommBackground, 0);
    FreePacketAndClear(&pKilrathiCommBackground, 0);
    FreePacketAndClear(&pCommStaticShape, 0);
    nCommSpeakerRating = -1;
    nCommSpeakerObject = -1;
    nCommPortraitIndex = -1;
}

/* Function start: 0x431470 */
void EndCommSessionWithWingman(void)
{
#ifdef SDL_PORT
    if (nCommPortraitIndex != -1 &&
#else
    if (
#endif
        apCommPortraitShapes[nCommPortraitIndex] != 0)
        malf_noise(1, 1, 12, 23, 1);
    FreeCommDisplayResources();
    if (get_mode(1) == 6)
        pop_mode(1);
}

/* Function start: 0x4314C0 */
void EndCommMenu(void)
{
    clear_message_time();
    if (get_mode(1) == 6)
        EndCommSessionWithWingman();
    pszPendingHudMessage = 0;
}

/* Function start: 0x4314F0 */
void ShowCentredPrompt(char *text, unsigned short arg)
{
    DosStrcpy(szHudMessageBuffer, text);
    SetHudMessageText(szHudMessageBuffer,
                      cYellowColour, arg);
}

/* Function start: 0x431520 */
short LoadCommDisplayResources(short rating, enum Side side)
{
    short loaded;

    loaded = 1;
    switch (side) {
    case SIDE_IMPERIAL:
        if (pConfedCommBackground == 0)
            pConfedCommBackground =
                FetchDiskPacketRetrying(11, 0, 0);
        loaded = pConfedCommBackground != 0;
        break;
    case SIDE_KILRATHI:
        if (pKilrathiCommBackground == 0)
            pKilrathiCommBackground =
                FetchDiskPacketRetrying(11, 9, 0);
        loaded = pKilrathiCommBackground != 0;
        break;
    }
    if (pCommStaticShape == 0)
        pCommStaticShape =
            FetchDiskPacketRetrying(11, 11, 0);
    if (loaded != 0 && pCommStaticShape != 0)
        return 1;
    return 0;
}

/* Function start: 0x4315C0 */
char *ExpandCommMessageTokens(const char *text)
{
    char *destination;
    const char *marker;
    short length;

    szTextScratchBuffer[0] = '\0';
    while ((marker = DosStrchr(text, '$')) != 0) {
        destination = DosStrchr(szTextScratchBuffer, '\0');
        while (marker != text)
            *destination++ = *text++;
        *destination = '\0';
        marker++;
        switch (*marker++) {
        case 'C':
            DosStrcat(
                szTextScratchBuffer,
                stCampaignState.currentPilot->callsign);
            break;
        case 'N':
        case 'P':
            DosStrcat(szTextScratchBuffer,
                      stCampaignState.currentPilot->name);
            break;
        case 'R':
            DosStrcat(szTextScratchBuffer,
                      apszPilotRankNames[
                          stCampaignState.currentPilot->rank]);
            length = DosStrlen(szTextScratchBuffer);
            if (szTextScratchBuffer[length - 1] == '.' &&
                *marker == '.') {
                length = DosStrlen(szTextScratchBuffer);
                szTextScratchBuffer[length - 1] = '\0';
            }
            break;
        }
        text = marker;
    }
    DosStrcat(szTextScratchBuffer, text);
    return szTextScratchBuffer;
}

/* Function start: 0x4316E0 */
void real_vid_transmit(short obj, short message)
{
    char text[84];
    char *expandedText;
    char *speech;

#ifdef SDL_PORT
    if (SdlSwcFlightActive()) {
        SdlSwcTransmit(obj, message);
        return;
    }
#endif
    nCommSpeakerObject = obj;
    nCommSpeakerRating =
        (short)acShipRating[obj];
    nCommPortraitIndex =
        get_face(nCommSpeakerRating,
                 aeShipSide[obj]);
    if (nCommPortraitIndex == -1)
        return;
    if (bCommVideoEnabled != 0 && bVideoImagesSuppressed == 0) {
        if (apCommPortraitShapes[
                nCommPortraitIndex] == 0)
            LoadCommPortraitShape(nCommPortraitIndex, 0);
        if (apCommPortraitShapes[
                nCommPortraitIndex] != 0 &&
            LoadCommDisplayResources(nCommSpeakerRating,
                aeShipSide[
                    nCommSpeakerObject]) != 0) {
            push_mode(1, 6);
            malf_noise(1, 3, 12, 23, 1);
            if (aeShipSide[nCommSpeakerObject] == SIDE_IMPERIAL)
                DrawSpriteDefault(&stRightVdu, stRightVdu.left,
                                  stRightVdu.top,
                                  pConfedCommBackground, 0);
            else
                DrawSpriteDefault(&stRightVdu, stRightVdu.left,
                                  stRightVdu.top,
                                  pKilrathiCommBackground, 0);
            DrawSpriteDefault(
                &stRightVdu, stRightVdu.left,
                stRightVdu.top,
                apCommPortraitShapes[
                    nCommPortraitIndex],
                0);
        }
    }
    speech = aapszPilotSpeech[
        nCommPortraitIndex][message];
    if (nCommSpeakerRating >= 0 &&
        nCommSpeakerRating <= 7) {
#ifdef SDL_PORT
        /* MSVC 4.20 accepts %Fs as its legacy far-string conversion. */
        sprintf(text, "%s: %s",
                apWingmanPilots[
                    nCommSpeakerRating]->callsign,
                speech);
#else
        sprintf(text, szConfedCommFormat,
                apWingmanPilots[
                    nCommSpeakerRating]->callsign,
                speech);
#endif
    } else if (nCommSpeakerRating >= 9 &&
               nCommSpeakerRating <= 12) {
#ifdef SDL_PORT
        sprintf(text, "%s: %s",
                apszKilrathiAceNames[
                    nCommSpeakerRating - 9],
                speech);
#else
        sprintf(text, szKilrathiAceCommFormat,
                apszKilrathiAceNames[
                    nCommSpeakerRating - 9],
                speech);
#endif
    } else {
#ifdef SDL_PORT
        sprintf(text, "%s: %s",
                aObjectTypeData[aeObjectType[obj]].displayName,
                speech);
#else
        sprintf(text, szShipCommFormat,
                aObjectTypeData[aeObjectType[obj]].displayName,
                speech);
#endif
    }
    expandedText = ExpandCommMessageTokens(text);
    ShowCentredPrompt(expandedText, (unsigned short)MeasureMessageWidth(text));
}

/* Function start: 0x4318F0 */
void __stdcall ShutdownVideoHook(short mode)
{
    ReleaseVideoResourcesHook();
}

/* Function start: 0x431900 */
short __stdcall ReserveContiguousPaletteEntries(short entryCount)
{
    short entry;
    short freeEntries;
    short firstEntry;

    freeEntries = 0;
    entry = 0;
    firstEntry = 0;
    for (;;) {
        if (awPaletteEntryAllocation[entry] != 0) {
            freeEntries = 0;
            firstEntry = (short)(entry + 1);
        } else
            freeEntries++;
        if (freeEntries == entryCount)
            break;
        entry++;
        if (entry >= 256)
            return -1;
    }

    freeEntries = 0;
    while (freeEntries < entryCount) {
        awPaletteEntryAllocation[firstEntry + freeEntries] =
            entryCount;
        freeEntries++;
    }
    return firstEntry;
}

/* Function start: 0x431970 */
void __stdcall ReleaseContiguousPaletteEntries(short firstEntry)
{
    short entry;
    short entryCount;

    entry = 0;
    entryCount = awPaletteEntryAllocation[firstEntry];
    while (entry < entryCount) {
        awPaletteEntryAllocation[firstEntry + entry] = 0;
        entry++;
    }
}

/* Function start: 0x4319B0 */
void PrintPaletteAllocationMap(void)
{
    short index;
    short row;
    short column;

    index = 0;
    row = 4;
    do {
        column = 0x40;
        do {
#if !defined(SDL_PORT) || defined(SDL_PORT_LEGACY_DEBUG_OUTPUT)
            printf("%c", awPaletteEntryAllocation[index++] < 1 ?
                   '_' : '.');
#else
            index++;
#endif
            column--;
        } while (column != 0);
#if !defined(SDL_PORT) || defined(SDL_PORT_LEGACY_DEBUG_OUTPUT)
        printf("\n");
#endif
        row--;
    } while (row != 0);
}

/* Function start: 0x431A10 */
void LoadJoystickCalibrationFile(short horizontalRange,
                                 short verticalRange,
                                 short horizontalDeadZone,
                                 short verticalDeadZone)
{
    unsigned short storedCentreX;
    unsigned short storedCentreY;
    unsigned short minimumX;
    unsigned short maximumX;
    unsigned short maximumY;
    unsigned short minimumY;
    short activeDevice;
    InputDeviceSample samples[2];
    int file;
    int failed;
    int centreX;
    int centreY;
    int rangeX;
    int rangeY;

    activeDevice = -1;
    failed = 1;
    if (horizontalRange == 0)
        horizontalRange += (short)failed;
    if (verticalRange == 0)
        verticalRange++;
    nJoystickFailureValue = -1;
    SampleBothJoysticks(samples, -1);
    if (samples[0].x != -1 && samples[0].y != -1)
        activeDevice = 0;
    else if (samples[1].x != -1 && samples[1].y != -1)
        activeDevice = 1;

    if (activeDevice != -1) {
        file = _open("j.cal", 0x8000);
        if (file != -1) {
            failed = _read(file, &activeDevice, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &minimumX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &minimumY, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &maximumX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &maximumY, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &storedCentreX, 2) <= 0;
            if (failed == 0)
                failed = _read(file, &storedCentreY, 2) <= 0;
            nJoystickCentreX = (unsigned int)storedCentreX;
            nJoystickCentreY = (unsigned int)storedCentreY;
            _close(file);
            if (failed != 0)
                _unlink("j.cal");
        }

        if (failed != 0) {
            GetJoystickDevCaps(activeDevice,
                               (short *)&minimumX,
                               (short *)&maximumX,
                               (short *)&minimumY,
                               (short *)&maximumY);
            centreX = ((int)minimumX + (int)maximumX) / 2;
            centreY = ((int)maximumY + (int)minimumY) / 2;
        } else {
            centreX = nJoystickCentreX;
            centreY = nJoystickCentreY;
        }

        if (centreX > 10) {
            nJoystickCalibrationMinimumX = centreX - 10;
            nJoystickCalibrationMaximumX = centreX + 10;
        } else {
            nJoystickCalibrationMinimumX = centreX;
            nJoystickCalibrationMaximumX = centreX;
        }
        if (centreY > 10) {
            nJoystickCalibrationMinimumY = centreY - 10;
            nJoystickCalibrationMaximumY = centreY + 10;
        } else {
            nJoystickCalibrationMaximumY = centreY;
            nJoystickCalibrationMinimumY = centreY;
        }

        rangeX = horizontalRange;
        rangeY = verticalRange;
        nJoystickLeftScale = nJoystickCalibrationMinimumX / rangeX;
        nJoystickUpScale = nJoystickCalibrationMinimumY / rangeY;
        if (nJoystickLeftScale == 0)
            nJoystickLeftScale++;
        if (nJoystickUpScale == 0)
            nJoystickUpScale++;
        nJoystickRightScale = nJoystickCalibrationMinimumX / rangeX;
        if (nJoystickRightScale == 0)
            nJoystickRightScale++;
        nJoystickDownScale = nJoystickCalibrationMinimumY / rangeY;
        if (nJoystickDownScale == 0)
            nJoystickDownScale++;

        nJoystickCentreX = centreX;
        nJoystickCentreY = centreY;
        nJoystickMinimumX =
            centreX - rangeX * nJoystickLeftScale;
        nJoystickMinimumY =
            centreY - rangeY * nJoystickUpScale;
        nJoystickMaximumX =
            rangeX * nJoystickLeftScale + centreX;
        nJoystickMaximumY =
            rangeY * nJoystickUpScale + centreY;
        nJoystickHorizontalDeadZone = horizontalDeadZone;
        nJoystickVerticalDeadZone = verticalDeadZone;
    }
    nActiveInputDevice = activeDevice;
}

/* Function start: 0x431D20 */
short ReadCalibratedJoystick(void)
{
    InputDeviceSample *sample;
    int sampleResult;
    int rawX;
    int rawY;
    short normalizedX;
    short normalizedY;
    short device;

    device = nActiveInputDevice;
    if (device == -1)
        return 0;
    sample = &aInputDeviceSamples[device];
    sampleResult = SampleJoystickDevice(
        sample, device,
        nJoystickFailureValue);
    rawX = sample->x;
    if (rawX == nJoystickFailureValue ||
        (rawY = aInputDeviceSamples[device].y) == nJoystickFailureValue ||
        sampleResult != 0) {
        nActiveInputDevice = -1;
        sample->x = aInputDeviceSamples[device].y =
            aInputDeviceSamples[device].buttons = 0;
        return 0;
    }

    nJoystickRawX = rawX;
    nJoystickRawY = rawY;
    if (nJoystickMinimumX > sample->x)
        sample->x = nJoystickMinimumX;
    if (sample->x > nJoystickMaximumX)
        sample->x = nJoystickMaximumX;
    if (aInputDeviceSamples[device].y <
        nJoystickMinimumY)
        aInputDeviceSamples[device].y =
            nJoystickMinimumY;
    if (aInputDeviceSamples[device].y >
        nJoystickMaximumY)
        aInputDeviceSamples[device].y =
            nJoystickMaximumY;

    normalizedY = 0;
    normalizedX = 0;
    rawX = sample->x;
    if (nJoystickCentreX > rawX) {
        normalizedX = (short)((nJoystickCentreX - rawX) /
                              nJoystickLeftScale);
        if (nJoystickHorizontalDeadZone < normalizedX) {
            if (normalizedX != 0)
                normalizedX = (short)-normalizedX;
        } else {
            normalizedX = 0;
        }
    } else if (nJoystickCentreX < rawX) {
        normalizedX = (short)((rawX - nJoystickCentreX) /
                              nJoystickRightScale);
        if (normalizedX <= nJoystickHorizontalDeadZone)
            normalizedX = 0;
    }

    rawY = aInputDeviceSamples[device].y;
    if (nJoystickCentreY > rawY) {
        normalizedY = (short)((nJoystickCentreY - rawY) /
                              nJoystickUpScale);
        if (nJoystickVerticalDeadZone < normalizedY) {
            if (normalizedY != 0)
                normalizedY = (short)-normalizedY;
        } else {
            normalizedY = 0;
        }
    } else if (nJoystickCentreY < rawY) {
        normalizedY = (short)((rawY - nJoystickCentreY) /
                              nJoystickDownScale);
        if (normalizedY <= nJoystickVerticalDeadZone)
            normalizedY = 0;
    }
    sample->x = (int)normalizedX;
    aInputDeviceSamples[device].y = (int)normalizedY;
    return 1;
}

/* Function start: 0x431EA0 */
void __stdcall UnionRectBounds(ShortRect *destination,
                               const ShortRect *first,
                               const ShortRect *second)
{
    destination->left = first->left < second->left ?
        first->left : second->left;
    destination->top = first->top < second->top ?
        first->top : second->top;
    destination->right = first->right > second->right ?
        first->right : second->right;
    destination->bottom = first->bottom > second->bottom ?
        first->bottom : second->bottom;
}
