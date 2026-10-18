/*
 *  Cockpit component-damage display.
 *
 *  Address range 42c800-42cdaf (see docs/ORDER.md).
 *  Initialized damage tables follow the preceding sound/configuration literals.
 */
#define WC1_DAMAGE_SOURCE
#include "wc1.h"

unsigned char *pDamageDisplayBackground = 0; /* 0x0046a748 */
/* 0x0046a750 */
static ShortPoint aDamageDisplayPositions[9] = {
    {36, 37}, {36, 28}, {36, 30}, {36, 23}, {36, 19},
    {36, 15}, {36, 24}, {36, 16}, {36, 22}
};
/* 0x0046a778 */
const char *apszComponentNames[9] = {
    "Ion drive",
    "Power plant",
    "Shield gen'r",
    "Computer sys",
    "InterCom unit",
    "Target track",
    "Accel absorbers",
    "Ejector system",
    "Repair systems"
};
/* 0x0046a7a0 */
static unsigned char abDamageDisplayFrames[9] = {
    21, 22, 23, 24, 25, 26, 27, 28, 29
};
/* 0x0046a7b0 */
static const char *apszDamageSeverityNames[5] = {
    "Ok",
    "Light",
    "Moderate",
    "Heavy",
    "Destroyed"
};

static int nDisplayedComponentDamage; /* 0x00476554 */

/* Linker commons defined here; LINK orders them by identifier hash. */
short nDamageDisplayTicks; /* 0x005a7786 */
short nWeaponDisplayOriginX; /* 0x005a7788 */
short nWeaponDisplayOriginY; /* 0x005a778a */
char szDamageStatusText[0x48]; /* 0x005a7790 */
ShortPoint stDamageSpritePosition; /* 0x005a77d8 */
signed char cDamageDisplayComponent; /* 0x005a77dc */
signed char cDamageDisplayFrame; /* 0x005a77dd */
signed char cDamagedComponentCount; /* 0x005a77de */
int nDamageDisplayState; /* 0x005a77e0 */
int nDamageDisplayPhase; /* 0x005a77e4 */

/* Function start: 0x42C800 */
void show_damage_disp(void)
{
    signed char component;
    char message[40];

    nDamageDisplayState = 0;
    cDamagedComponentCount = 0;
    for (component = 0; component < 9; component++) {
        if (acPlayerComponentDamage[component] >= 1)
            cDamagedComponentCount++;
    }

    set_new_vdu(0);
    DrawTextAt(&stLeftVduTextContext,
               stLeftVdu.left, stLeftVdu.top,
               "DAMAGE REPORT", 2);
    DrawViewportLine(&stLeftVdu,
                     (short)(stLeftVdu.left + 2),
                     (short)(stLeftVdu.top + 6),
                     (short)(stLeftVdu.right - 2),
                     (short)(stLeftVdu.top + 6),
                     cPrimaryTextColour);
    if (cDamagedComponentCount == 0) {
        DrawTextAt(&stLeftVduTextContext, stLeftVdu.left,
                   (short)(stLeftVdu.top + 20),
                   "NO INTERNAL\n\nDAMAGE", 2);
        return;
    }

    component = cCockpitView;
    nWeaponDisplayOriginX =
        (short)(aWeaponDisplayOrigins[component].x +
                stLeftVdu.left);
    nWeaponDisplayOriginY =
        (short)(aWeaponDisplayOrigins[component].y +
                stLeftVdu.top);
    /* The original reloads X from memory for the sprite argument. */
    DrawSpriteDefault(&stLeftVdu,
                      *(volatile short *)&nWeaponDisplayOriginX,
                      nWeaponDisplayOriginY,
                      pCockpitWeaponShape, 0);
    sprintf(message, "%d Unit%c Damaged",
            (int)cDamagedComponentCount,
            (int)(signed char)(cDamagedComponentCount == 1 ? ' ' : 's'));
    ShowComponentHitHudMessage(message, cPrimaryTextColour, -1);
}

/* Function start: 0x42C970 */
void UpdateDamageDisplay(void)
{
    short previousComponentCount;
    signed char componentIndex;
    signed char componentCount;
    signed char component;
    signed char attempts;
    signed char damage;

    previousComponentCount = cDamagedComponentCount;
    componentCount = 0;
    for (componentIndex = 0; componentIndex < 9; componentIndex++) {
        if (acPlayerComponentDamage[componentIndex] >= 1)
            componentCount++;
    }

    cDamagedComponentCount = componentCount;
    if ((short)componentCount != previousComponentCount) {
        InvalidateVduMode(0);
        return;
    }
    cDamagedComponentCount = componentCount;
    if (componentCount == 0)
        return;

    if (bCockpitlessView == 0) {
        nDamageDisplayTicks--;
        if (nDamageDisplayTicks > 0)
            return;
        if (nDamageDisplayPhase == 1) {
            component = cDamageDisplayComponent;
            attempts = 0;
            nDamageDisplayTicks = 50;
            do {
                component++;
                if (component >= 9)
                    component = 0;
                damage = acPlayerComponentDamage[component];
                nDisplayedComponentDamage = (int)damage;
                if (nDisplayedComponentDamage >= 1) {
                    cDamageDisplayComponent = component;
                    break;
                }
                attempts++;
            } while (attempts < 9);

            sprintf(
                szDamageStatusText,
                "%s\nDamage: %s",
                apszComponentNames[
                    cDamageDisplayComponent],
                apszDamageSeverityNames[(int)damage]);
            DrawTextAt(&stLeftVduTextContext,
                       (short)(stLeftVdu.left + 1),
                       (short)(stLeftVdu.top + 7),
                       szDamageStatusText, 2);
            cDamageDisplayFrame =
                (signed char)abDamageDisplayFrames[
                    cDamageDisplayComponent];
            stDamageSpritePosition =
                aDamageDisplayPositions[
                    cDamageDisplayComponent];
            stDamageSpritePosition.x =
                (short)(stDamageSpritePosition.x +
                        nWeaponDisplayOriginX);
            stDamageSpritePosition.y =
                (short)(stDamageSpritePosition.y +
                        nWeaponDisplayOriginY);
            CaptureSpriteBackground(
                &stLeftVdu, pDamageDisplayBackground,
                stDamageSpritePosition.x,
                stDamageSpritePosition.y,
                pCockpitWeaponShape,
                (short)cDamageDisplayFrame);
            DrawViewportLine(
                &stLeftVdu, (short)(stLeftVdu.left + 36),
                (short)(stLeftVdu.top + 22),
                stDamageSpritePosition.x,
                stDamageSpritePosition.y,
                0xa9);
            DrawSpriteDefault(
                &stLeftVdu, stDamageSpritePosition.x,
                stDamageSpritePosition.y,
                pCockpitWeaponShape,
                (short)cDamageDisplayFrame);
        } else {
            RestoreSpriteBackground(
                &stLeftVdu, pDamageDisplayBackground,
                stDamageSpritePosition.x,
                stDamageSpritePosition.y,
                pCockpitWeaponShape,
                (short)cDamageDisplayFrame);
            stLeftVduTextContext.colour = cBlackColour;
            DrawTextAt(&stLeftVduTextContext,
                       (short)(stLeftVdu.left + 1),
                       (short)(stLeftVdu.top + 7),
                       szDamageStatusText, 2);
            stLeftVduTextContext.colour = cPrimaryTextColour;
            DrawViewportLine(
                &stLeftVdu, (short)(stLeftVdu.left + 36),
                (short)(stLeftVdu.top + 22),
                stDamageSpritePosition.x,
                stDamageSpritePosition.y,
                cBlackColour);
            nDamageDisplayTicks = 2;
        }
        nDamageDisplayPhase =
            nDamageDisplayPhase == 0;
        return;
    }

    nDamageDisplayTicks--;
    if (nDamageDisplayTicks <= 0) {
        component = cDamageDisplayComponent;
        attempts = 0;
        nDamageDisplayTicks = 50;
        for (;;) {
            component++;
            if (component >= 9)
                component = 0;
            nDisplayedComponentDamage =
                (int)acPlayerComponentDamage[component];
            if (nDisplayedComponentDamage >= 1) {
                cDamageDisplayComponent = component;
                return;
            }
            attempts++;
            if (attempts >= 9)
                return;
        }
    }

    sprintf(
        szDamageStatusText,
        "%s\nDamage: %s",
        apszComponentNames[cDamageDisplayComponent],
        apszDamageSeverityNames[
            nDisplayedComponentDamage]);
    DrawTextAt(&stLeftVduTextContext, (short)(stLeftVdu.left + 1),
               (short)(stLeftVdu.top + 7),
               szDamageStatusText, 2);
    cDamageDisplayFrame =
        (signed char)abDamageDisplayFrames[
            cDamageDisplayComponent];
    stDamageSpritePosition =
        aDamageDisplayPositions[
            cDamageDisplayComponent];
    stDamageSpritePosition.x =
        (short)(stDamageSpritePosition.x +
                nWeaponDisplayOriginX);
    stDamageSpritePosition.y =
        (short)(stDamageSpritePosition.y +
                nWeaponDisplayOriginY);
    CaptureSpriteBackground(
        &stLeftVdu, pDamageDisplayBackground,
        stDamageSpritePosition.x,
        stDamageSpritePosition.y,
        pCockpitWeaponShape,
        (short)cDamageDisplayFrame);
    DrawViewportLine(&stLeftVdu,
                     (short)(stLeftVdu.left + 36),
                     (short)(stLeftVdu.top + 22),
                     stDamageSpritePosition.x,
                     stDamageSpritePosition.y, 0xa9);
    DrawSpriteDefault(&stLeftVdu,
                      stDamageSpritePosition.x,
                      stDamageSpritePosition.y,
                      pCockpitWeaponShape,
                      (short)cDamageDisplayFrame);
}

/* Function start: 0x42CDA0 */
unsigned short GetJoystickPresentUnused(void)
{
    return 1;
}
