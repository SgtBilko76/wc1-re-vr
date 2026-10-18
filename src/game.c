/*
 *  Shared game state, mission and maneuver tables, and information display.
 *
 *  Address range 433910-433abf (see docs/ORDER.md).
 *  The data ownership follows the declarations-before-literals layout.
 *  The original source filename is not known.
 */
#include "wc1.h"

/* 0x0046c000 */
signed char cViewObject = -1;

/* 0x0046c004 */
short nEyePitchRate = 1;

/* 0x0046c008 */
short nEyeYawRate = 1;

/* 0x0046c00c */
short nEyeRollRate = 1;

/* 0x0046c010 */
short DAT_0046c010 = -1;

/* 0x0046c014 */
unsigned char bCurrentKey = 0x80;

/* 0x0046c018 */
signed char cPreviousKey = (signed char)0x80;

/* 0x0046c01c */
short nCurrentWave = -1;

/* 0x0046c020 */
signed char cCurrentObjective = -1;

/* 0x0046c024 */
int bIntroSecondaryScene = 0;

/* 0x0046c028 */
signed char abHazardObjects[0x14] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* 0x0046c03c */
int nCameraViewMode = -1;

/* 0x0046c040 */
short nExternalViewShip = -1;

/* 0x0046c044 */
short nEjectedPilotObject = 0;

/* 0x0046c048 */
short nClosestVisibleObject = -1;

/* 0x0046c04c */
short nYourWingman = -1;

/* 0x0046c050 */
short nPlayerCollisionObject = -1;

/* 0x0046c054 */
enum ObjectType eSelectedGunType = (enum ObjectType)-1;

/* 0x0046c058 */
int nSelectedReleaseWeaponIndex = -1;

/* 0x0046c05c */
short DAT_0046c05c = 0;

/* 0x0046c060 */
short bTargetLockReadoutDirty = 0;

/* 0x0046c064 */
short nTargetLockCountdown = 0;

/* 0x0046c068 */
short nTargetLockMarkerAngle = 0;

/* 0x0046c06c */
signed char cTargetDisplayObject = -1;

/* Nothing in the shipped executable references this zero word. */
/* 0x0046c070 */
static int DAT_0046c070 = 0;

/* 0x0046c074 */
int bTargetLockAcquired = 0;

/* 0x0046c078 */
short nTargetLockMode = 0;

/* 0x0046c07c */
int bMissileCameraEnabled = 0;

/* 0x0046c080 */
int bEngageAllowed = 0;

/* 0x0046c084 */
short nAutoEngageTimer = -1;

/* 0x0046c088 */
signed char cViableTargetCount = 0;

/* 0x0046c090 */
short asIntroCameraSequence[20] = {
    0, -1000, 0, -4263, 2, 15, 1, 0, 30, 0,
    3, 15, 4, 30, 1, 13, 14, 400, -1, 0
};

/* 0x0046c0b8 */
static short asCannedSequence32[16] = {
    0, 20, 1, 0, 0, 180, 40, 1, 0, 0, 180, 40,
    2, 0, 400, -1
};

/* 0x0046c0d8 */
static short asCannedSequence33[12] = {
    0, 20, 0, 20, 1, 0, -15, -60, 50, 0, 400, -1
};

/* 0x0046c0f0 */
static short asCannedSequence34[12] = {
    0, 20, 0, 20, 1, 0, 15, 60, 50, 0, 400, -1
};

/* 0x0046c108 */
static short asCannedSequence35[42] = {
    0, 17, 0, 6, 3, 0, 6, 3, 0, 6, 3, 0, 6, 3,
    0, 6, 3, 0, 6, 3, 0, 6, 3,
    1, 0, 0, 180, 40, 1, 0, 0, 180, 40,
    1, 0, -30, 60, 40, 0, 400, -1
};

/* 0x0046c160 */
short asCampaignVictoryViewScript[24] = {
    16, 38, 0, 1200, 0, 1600, 1, 180,
    0, 0, 15, 3, 4, 14, 100, 2,
    15, 3, 15, 14, 120, -1, 0, 0
};

/* 0x0046c190 */
static short asCannedSequence37[13] = {
    0, 100, 1, 0, 30, 0, 50, 0, 20, 2, 0, 400, -1
};

/* 0x0046c1b0 */
static short asCannedSequence38[13] = {
    0, 110, 1, 0, 30, 0, 50, 0, 10, 2, 0, 400, -1
};

/* 0x0046c1d0 */
static short asCannedSequence39[6] = {
    0, 10, 2, 0, 400, -1
};

/* 0x0046c1e0 */
static short asCannedSequence40[39] = {
    0, 40,
    3, 1, 0, 0, 90, 60, 3, 1, 0, 0, 90, 60,
    3, 1, 0, 0, 90, 60, 3, 1, 0, 0, 90, 60,
    3, 1, 0, 0, 90, 60, 3, 1, 0, 0, 90, 60, -1
};

/* 0x0046c230 */
static short asCannedSequence41[3] = { 0, 400, -1 };

/* 0x0046c238 */
short asTigerClawEscapeViewScript[12] = {
    0, 0, 0, 0, 2, 15, 3, 15, 14, 400, -1, 0
};

/* 0x0046c250 */
static short asCannedSequence42[15] = {
    0, 120, 1, 0, 0, 180, 50, 3, 1, 0, 0, 180, 50, 3, -1
};

/* 0x0046c270 */
static short asCannedSequence43[21] = {
    0, 120, 1, 0, 0, 180, 50, 3, 1, 0, 0, 180, 50,
    3, 1, 0, 0, 180, 50, 3, -1
};

/* 0x0046c2a0 */
static short asCannedSequence44[33] = {
    0, 5, 3, 0, 5, 3, 0, 5, 3, 0, 5, 3,
    1, 0, 0, 180, 50, 1, 0, 0, 180, 50,
    1, 0, 0, 180, 50, 1, 0, 0, 180, 50, -1
};

/* 0x0046c2e8 */
static short asCannedSequence45[4] = { 0, 20, 2, -1 };

/* 0x0046c2f0 */
MissionNavPoint aMissionNavPoints[MISSION_NAV_POINT_COUNT] = {
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    {
        "", 1, { 0, 0, 0 }, 50000,
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
        { OBJECT_TYPE_DRALTHI, OBJECT_TYPE_HORNET },
        { 32, 33, 34, 35, -1, -1, -1, -1, -1, -1 }
    },
    {
        "", 1, { 0, 0, 0 }, 50000,
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
        { OBJECT_TYPE_ASTEROID_FIELD, (enum ObjectType)-1 },
        { 36, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
    },
    {
        "", 1, { 0, 0, 0 }, 50000,
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
        { OBJECT_TYPE_GRATHA, OBJECT_TYPE_RAPIER },
        { 37, 38, 39, 40, 41, -1, -1, -1, -1, -1 }
    },
    {
        "", 1, { 0, 0, 0 }, 50000,
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
        { OBJECT_TYPE_KRANT, OBJECT_TYPE_SCIMITAR },
        { 42, 43, 44, 45, -1, -1, -1, -1, -1, -1 }
    }
};

/* 0x0046c948 */
MissionShipRecord
    aMissionShips[MISSION_SHIP_STORAGE_COUNT] = {
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { OBJECT_TYPE_DRALTHI, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      16, { 0, 0, 0 }, -155, 0, 0, 0, 40, 3,
      { asCannedSequence32 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_DRALTHI, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      16, { -154521, -25600, -232012 }, -155, 0, 0, 0, 40, 3,
      { asCannedSequence33 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_DRALTHI, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      16, { -61849, 25600, -232012 }, -155, 0, 0, 0, 40, 3,
      { asCannedSequence34 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_HORNET, SIDE_IMPERIAL, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      16, { 324582, 0, 696038 }, -155, 0, 0, 0, 40, 3,
      { asCannedSequence35 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_ASTEROID_FIELD, SIDE_IMPERIAL, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE, 17, { 0, 0, 0 }, 0, 0, 0, 0,
      30000, 3, { 0 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_GRATHA, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      18, { 102400, -153600, -332800 }, 0, 0, -30, 0, 80, 3,
      { asCannedSequence37 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_GRATHA, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      18, { 0, 256000, -332800 }, 0, 0, 90, 0, 62, 3,
      { asCannedSequence38 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_GRATHA, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      18, { 0, 256000, -153600 }, 0, 0, -30, 0, 80, 3,
      { asCannedSequence39 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_RAPIER, SIDE_IMPERIAL, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      18, { 0, 256000, -819200 }, 0, 0, 30, 0, 60, 3,
      { asCannedSequence40 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_RAPIER, SIDE_IMPERIAL, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      18, { 102400, -153600, -819200 }, 0, 0, 0, 0, 80, 3,
      { asCannedSequence41 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_KRANT, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      19, { -51200, 76800, -1382400 }, 0, 0, 0, 0, 50, 3,
      { asCannedSequence42 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_KRANT, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      19, { 153600, 0, -1280000 }, 0, 0, 60, 0, 50, 3,
      { asCannedSequence43 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_KRANT, SIDE_KILRATHI, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE,
      19, { -249856, 0, 1139200 }, 155, 0, 0, 0, 50, 3,
      { asCannedSequence44 }, 0, 0, 0, -1, 0, 0 },
    { OBJECT_TYPE_SCIMITAR, SIDE_IMPERIAL, -1, -1,
      MISSION_TYPE_CANNED_SEQUENCE, 19, { -76800, 0, 768000 },
      155, 0, 0, 0, 50, 3, { asCannedSequence45 },
      0, 0, 0, -1, 0, 0 }
};

/* 0x0046d368 */
int aiIntelligenceEvent[10] = {0};

/* 0x0046d390 */
static signed char aDefenseManeuversNovice[8] = {
    24, 34, 13, 14, -1, 0, 0, 0
};

/* 0x0046d398 */
static signed char aDefenseManeuversVeteran[8] = {
    8, 13, 15, 14, 19, 24, -1, 0
};

/* 0x0046d3a0 */
static signed char aDefenseManeuversElite[16] = {
    8, 15, 17, 23, 19, 9, 20, 34, 14, -1, 0, 0, 0, 0, 0, 0
};

/* 0x0046d3b0 */
static signed char aDefenseManeuversAce[16] = {
    17, 23, 15, 19, 9, 14, 20, 12, -1, 0, 0, 0, 0, 0, 0, 0
};

/* 0x0046d3c0 */
static signed char aDefenseManeuversBoss[16] = {
    15, 19, 12, 11, 17, 23, 7, 35, -1, 0, 0, 0, 0, 0, 0, 0
};

/* 0x0046d3d0 */
signed char *apDefenseManeuvers[5] = {
    aDefenseManeuversNovice,
    aDefenseManeuversVeteran,
    aDefenseManeuversElite,
    aDefenseManeuversAce,
    aDefenseManeuversBoss
};

/* 0x0046d3e8 */
ManeuverChoice aRatedManeuverChoices[13][9][3] = {
    {
        { { 70, 40, 30 }, { 80, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 80, 29, 37 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 50, 7, 8 }, { 75, 14, 17 }, { 100, 21, -1 } },
        { { 80, 41, 29 }, { 80, 41, 24 }, { 100, 21, -1 } },
        { { 60, 26, 29 }, { 70, 26, 29 }, { 90, 26, 29 } },
        { { 50, 16, 12 }, { 50, 13, 20 }, { 100, 21, -1 } },
        { { 100, 19, -1 }, { 50, 9, 20 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 40, 40, 30 }, { 80, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 31, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 55, 12, 19 }, { 70, 11, 12 }, { 100, 21, -1 } },
        { { 60, 29, 41 }, { 80, 29, 41 }, { 100, 33, -1 } },
        { { 80, 26, 29 }, { 50, 26, 29 }, { 100, 29, -1 } },
        { { 50, 15, 16 }, { 50, 16, 32 }, { 100, 21, -1 } },
        { { 50, 7, 10 }, { 50, 14, 9 }, { 100, 21, -1 } },
        { { 50, 25, 2 }, { 50, 25, 2 }, { 100, 2, -1 } }
    },
    {
        { { 40, 40, 30 }, { 60, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 60, 15, 17 }, { 50, 12, 17 }, { 100, 21, -1 } },
        { { 20, 41, 30 }, { 30, 41, 17 }, { 40, 41, 21 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 40, 32, 17 }, { 20, 32, 16 }, { 100, 21, -1 } },
        { { 50, 17, 20 }, { 50, 23, 36 }, { 100, 20, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 21, -1 } }
    },
    {
        { { 50, 40, 30 }, { 90, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 80, 30, 37 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 44, -1 }, { 100, 44, -1 } },
        { { 70, 7, 16 }, { 100, 44, -1 }, { 100, 44, -1 } },
        { { 20, 31, 41 }, { 100, 44, -1 }, { 100, 44, -1 } },
        { { 80, 26, 31 }, { 100, 44, -1 }, { 100, 44, -1 } },
        { { 75, 16, 15 }, { 75, 16, 15 }, { 100, 44, -1 } },
        { { 50, 12, 16 }, { 100, 44, -1 }, { 100, 44, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 60, 40, 30 }, { 100, 40, -1 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 39, -1 } },
        { { 50, 15, 16 }, { 60, 15, 7 }, { 70, 20, 16 } },
        { { 70, 41, 17 }, { 80, 41, 20 }, { 100, 20, -1 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 50, 16, 13 }, { 50, 16, 20 }, { 100, 21, -1 } },
        { { 50, 20, 36 }, { 50, 11, 10 }, { 100, 20, -1 } },
        { { 25, 25, 2 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 40, 40, 30 }, { 80, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 20, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 39, -1 } },
        { { 50, 7, 8 }, { 50, 9, 24 }, { 100, 12, -1 } },
        { { 80, 41, 29 }, { 80, 41, 20 }, { 100, 41, -1 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 20, 32, 16 }, { 50, 16, 20 }, { 100, 21, -1 } },
        { { 50, 19, 14 }, { 50, 14, 12 }, { 100, 8, -1 } },
        { { 50, 17, 23 }, { 50, 17, 23 }, { 30, 25, 2 } }
    },
    {
        { { 50, 30, 31 }, { 60, 40, 31 }, { 100, 39, -1 } },
        { { 80, 29, 37 }, { 80, 29, 37 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 39, -1 } },
        { { 50, 9, 10 }, { 60, 7, 11 }, { 100, 20, -1 } },
        { { 50, 31, 41 }, { 80, 31, 23 }, { 100, 33, -1 } },
        { { 80, 26, 29 }, { 70, 26, 29 }, { 100, 26, -1 } },
        { { 50, 32, 16 }, { 50, 20, 16 }, { 100, 21, -1 } },
        { { 50, 36, 12 }, { 50, 20, 8 }, { 50, 19, 20 } },
        { { 80, 25, 2 }, { 50, 25, 17 }, { 100, 2, -1 } }
    },
    {
        { { 50, 40, 30 }, { 80, 40, 30 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 60, 15, 8 }, { 70, 19, 14 }, { 100, 21, -1 } },
        { { 70, 29, 20 }, { 50, 29, 20 }, { 100, 41, -1 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 50, 16, 19 }, { 50, 16, 8 }, { 100, 21, -1 } },
        { { 50, 36, 12 }, { 50, 19, 20 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 100, -1, -1 }, { 100, -1, -1 }, { 100, -1, -1 } },
        { { 0, -1, -1 }, { 0, -1, -1 }, { 0, -1, -1 } },
        { { 0, -1, -1 }, { 0, -1, -1 }, { 0, -1, -1 } }
    },
    {
        { { 60, 40, 29 }, { 60, 40, 29 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 50, 16, 38 }, { 75, 7, 20 }, { 100, 21, -1 } },
        { { 60, 41, 31 }, { 60, 41, 31 }, { 100, 21, -1 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 50, 32, 20 }, { 50, 32, 20 }, { 100, 21, -1 } },
        { { 50, 19, 20 }, { 50, 9, 20 }, { 100, 21, -1 } },
        { { 100, 25, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 50, 40, 31 }, { 70, 40, 31 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 97, 12, 22 }, { 95, 20, 22 }, { 100, 21, -1 } },
        { { 40, 41, 31 }, { 20, 41, 31 }, { 100, 21, -1 } },
        { { 50, 26, 31 }, { 100, 31, -1 }, { 100, 26, -1 } },
        { { 50, 16, 20 }, { 50, 19, 20 }, { 100, 21, -1 } },
        { { 100, 20, 36 }, { 50, 12, 20 }, { 100, 21, -1 } },
        { { 100, 25, -1 }, { 100, 25, -1 }, { 100, 2, -1 } }
    },
    {
        { { 70, 40, 30 }, { 70, 16, 30 }, { 100, 21, -1 } },
        { { 100, 18, -1 }, { 100, 18, -1 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 50, 23, 20 }, { 75, 12, 17 }, { 100, 21, -1 } },
        { { 30, 41, 29 }, { 50, 41, 20 }, { 100, 21, -1 } },
        { { 100, 26, -1 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 20, 32, 20 }, { 50, 16, 20 }, { 100, 21, -1 } },
        { { 50, 16, 19 }, { 50, 12, 20 }, { 100, 21, -1 } },
        { { 100, 25, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 80, 40, 31 }, { 90, 40, 31 }, { 100, 21, -1 } },
        { { 100, 18, -1 }, { 100, 18, -1 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 50, 15, 20 }, { 75, 16, 19 }, { 100, 21, -1 } },
        { { 50, 41, 31 }, { 50, 41, 31 }, { 100, 21, -1 } },
        { { 80, 26, 31 }, { 100, 26, -1 }, { 100, 26, -1 } },
        { { 50, 16, 20 }, { 50, 32, 20 }, { 100, 21, -1 } },
        { { 70, 19, 17 }, { 50, 11, 20 }, { 100, 21, -1 } },
        { { 100, 25, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    }
};

/* 0x0046d808 */
ManeuverChoice aKilrathiManeuverChoices[5][9][3] = {
    {
        { { 70, 40, 45 }, { 80, 40, 45 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 90, 17, 46 }, { 90, 17, 46 }, { 100, 21, -1 } },
        { { 50, 46, 45 }, { 80, 46, 45 }, { 100, 21, -1 } },
        { { 100, 45, -1 }, { 100, 45, -1 }, { 100, 45, -1 } },
        { { 100, 46, -1 }, { 20, 17, 46 }, { 100, 21, -1 } },
        { { 100, 46, -1 }, { 25, 17, 46 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 40, 40, 45 }, { 70, 40, 45 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 30 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 50, 8, 46 }, { 50, 14, 46 }, { 100, 21, -1 } },
        { { 70, 45, 46 }, { 60, 45, 46 }, { 100, 21, -1 } },
        { { 100, 45, -1 }, { 100, 45, -1 }, { 100, 45, -1 } },
        { { 100, 46, -1 }, { 100, 46, -1 }, { 100, 21, -1 } },
        { { 50, 38, 46 }, { 100, 46, -1 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 80, 40, 29 }, { 80, 40, 29 }, { 100, 21, -1 } },
        { { 80, 29, 37 }, { 50, 29, 40 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 21, -1 } },
        { { 60, 46, 45 }, { 60, 46, 45 }, { 100, 21, -1 } },
        { { 30, 41, 45 }, { 30, 41, 45 }, { 100, 21, -1 } },
        { { 100, 29, -1 }, { 100, 45, -1 }, { 100, 45, -1 } },
        { { 100, 46, -1 }, { 100, 46, -1 }, { 100, 46, -1 } },
        { { 50, 29, 46 }, { 100, 46, -1 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 50, 40, 29 }, { 50, 40, 29 }, { 100, 21, -1 } },
        { { 100, 18, -1 }, { 100, 18, -1 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 39, -1 } },
        { { 30, 12, 29 }, { 98, 46, 22 }, { 100, 21, -1 } },
        { { 90, 29, 31 }, { 100, 29, -1 }, { 100, 21, -1 } },
        { { 100, 45, -1 }, { 100, 45, -1 }, { 100, 45, -1 } },
        { { 100, 46, -1 }, { 100, 46, -1 }, { 100, 46, -1 } },
        { { 50, 36, 46 }, { 100, 46, -1 }, { 100, 21, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    },
    {
        { { 40, 40, 29 }, { 40, 40, 29 }, { 100, 21, -1 } },
        { { 100, 18, -1 }, { 100, 18, -1 }, { 100, 21, -1 } },
        { { 100, 39, -1 }, { 100, 39, -1 }, { 100, 39, -1 } },
        { { 90, 46, 36 }, { 90, 46, 12 }, { 100, 21, -1 } },
        { { 100, 29, -1 }, { 100, 29, -1 }, { 100, 21, -1 } },
        { { 100, 45, -1 }, { 100, 45, -1 }, { 100, 45, -1 } },
        { { 100, 46, -1 }, { 100, 46, -1 }, { 100, 21, -1 } },
        { { 10, 11, 46 }, { 100, 46, -1 }, { 100, 46, -1 } },
        { { 100, 2, -1 }, { 100, 2, -1 }, { 100, 2, -1 } }
    }
};

/* 0x0046d9a0 */
signed char acPilotAggression[24] = {
    3, 3, 3, 2, 2, 3, 2, 3, 0, 3, 2, 5,
    4, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0
};

/* 0x0046d9b8 */
signed char acPilotRecovery[20] = {
    6, 7, 8, 8, 9, 8, 6, 8, 10, 7,
    8, 9, 7, 0, 7, 7, 8, 8, 0, 0
};

/* Linker commons defined here; LINK orders them by identifier hash. */
MouseCursorState stHostMouseState; /* 0x0059af70 */
TextContext *pCurrentTextContext; /* 0x0059af8c */
int nInputTickScale; /* 0x0059af90 */
FixedVector aObjectViewPosition[SPACE_OBJECT_COUNT]; /* 0x0059afa0 */
short anObjectPitchRotation[SPACE_OBJECT_COUNT]; /* 0x0059b2a0 */
int anShipSpeed[64]; /* 0x0059b320 */
short nSpaceFrame; /* 0x0059b420 */
unsigned int anShipAlertFlags[12]; /* 0x0059b430 */
FixedVector vPlayerAcceleration; /* 0x0059b460 */
int anShipFuel[12]; /* 0x0059b470 */
short asObjectDistance[SPACE_OBJECT_COUNT]; /* 0x0059b4a0 */
ShortVector aShipFormationOffset[10]; /* 0x0059b520 */
enum ObjectType aeObjectType[SPACE_OBJECT_COUNT]; /* 0x0059b560 */
short asObjectAnimationDelay[SPACE_OBJECT_COUNT]; /* 0x0059b660 */
FixedVector aShipRightVector[64]; /* 0x0059b6e0 */
FixedVector aShipUpVector[64]; /* 0x0059b9e0 */
FixedVector aShipForwardVector[64]; /* 0x0059bce0 */
HazardField *pActiveHazardField; /* 0x0059bfe0 */
signed char acPlayerComponentDamage[9]; /* 0x0059bff0 */
signed char abFlightPath[MISSION_OBJECTIVE_COUNT]; /* 0x0059c000 */
FixedVector aShipVelocity[SPACE_OBJECT_COUNT]; /* 0x0059c010 */
short anYawGoal[16]; /* 0x0059c310 */
short asObjectCounter[SPACE_OBJECT_COUNT]; /* 0x0059c330 */
short nTargetShip; /* 0x0059c3b0 */
enum SpecialManeuver aeSpecialManeuver[12]; /* 0x0059c3c0 */
enum ShipMissionType aeShipMissionType[12]; /* 0x0059c3f0 */
short asShipCount[16]; /* 0x0059c420 */
short asShipMaximumSpeed[16]; /* 0x0059c440 */
signed char acShipDamage[10]; /* 0x0059c460 */
signed char cMissionObjectiveCount; /* 0x0059c46a */
short asViableTargetDistance[16]; /* 0x0059c470 */
FixedVector aShipPosition[SPACE_OBJECT_COUNT]; /* 0x0059c490 */
unsigned short awObjectMass[SPACE_OBJECT_COUNT]; /* 0x0059c790 */
short asShipAfterburnerTimer[16]; /* 0x0059c810 */
short nShipMissionIndices[10]; /* 0x0059c830 */
signed char acShipCommunicator[16]; /* 0x0059c850 */
FixedVector vStarFieldMotion; /* 0x0059c860 */
signed char cCurrentNavPointIndex; /* 0x0059c86c */
short asObjectFlip[SPACE_OBJECT_COUNT]; /* 0x0059c870 */
short nEyeRollGoal; /* 0x0059c8f0 */
FixedVector vPreviousStarFieldMotion; /* 0x0059c900 */
short nHazardFieldCount; /* 0x0059c90c */
signed char acLastAttacker[16]; /* 0x0059c910 */
signed char acViableTarget[16]; /* 0x0059c920 */
short asActionCount[10]; /* 0x0059c930 */
short nEyeYawGoal; /* 0x0059c944 */
short asObjectScreenScale[SPACE_OBJECT_COUNT]; /* 0x0059c950 */
short asObjectAfterburnerVelocity[SPACE_OBJECT_COUNT]; /* 0x0059c9d0 */
CampaignState stCampaignState; /* 0x0059ca50 */
unsigned char aShipWeapons[10][0x47]; /* 0x0059cab0 */
signed char acShipRating[16]; /* 0x0059cd80 */
short asObjectScreenAngle[SPACE_OBJECT_COUNT]; /* 0x0059cd90 */
short nTargetRange; /* 0x0059ce10 */
unsigned int anVduModeCache[2]; /* 0x0059ce18 */
signed char acObjectOwner[64]; /* 0x0059ce20 */
signed char acShipTarget[16]; /* 0x0059ce60 */
short nPreviousPitchInput; /* 0x0059ce70 */
short nPreviousYawInput; /* 0x0059ce72 */
short nPreviousRollInput; /* 0x0059ce74 */
short anObjectYawRotation[SPACE_OBJECT_COUNT]; /* 0x0059ce80 */
signed char acPilotHitPoints[16]; /* 0x0059cf00 */
signed char acTurnRegulator[16]; /* 0x0059cf10 */
unsigned char abCollisionAlertTarget[0x10]; /* 0x0059cf20 */
int aiPilotLevel[12]; /* 0x0059cf30 */
short asTargetListRange[16]; /* 0x0059cf60 */
short asObjectDrawY[SPACE_OBJECT_COUNT]; /* 0x0059cf80 */
short asObjectDrawX[SPACE_OBJECT_COUNT]; /* 0x0059d000 */
short asPreviousObjectDistance[SPACE_OBJECT_COUNT]; /* 0x0059d080 */
enum ObjectClass aeObjectClass[SPACE_OBJECT_COUNT]; /* 0x0059d100 */
enum ShipObjective aeShipObjective[12]; /* 0x0059d200 */
short asObjectViewFrame[SPACE_OBJECT_COUNT]; /* 0x0059d230 */
int nCurrentObjectiveRange; /* 0x0059d2b0 */
signed char acWingmanMessageState[16]; /* 0x0059d2c0 */
short asCollisionCountdown[16]; /* 0x0059d2d0 */
unsigned char *apObjectShape[SPACE_OBJECT_COUNT]; /* 0x0059d2f0 */
short nPitchInput; /* 0x0059d3f0 */
short nYawInput; /* 0x0059d3f2 */
short nRollInput; /* 0x0059d3f4 */
short asShipWingLeader[16]; /* 0x0059d400 */
/* The Win32 object allocator reserves slots 0 through 9 for live ships. */
short aasShipArmor[10][4]; /* 0x0059d420 */
short asShipWeaponEnergy[10]; /* 0x0059d470 */
signed char acFormationMemberList[16]; /* 0x0059d490 */
signed char acShipIonDriveDamage[16]; /* 0x0059d4a0 */
short anShipMissionShip[16]; /* 0x0059d4b0 */
FixedVector vToTarget; /* 0x0059d4d0 */
short asCannedCommand[16]; /* 0x0059d4e0 */
unsigned short ausVduModeStack[16]; /* 0x0059d500 */
char acShipSequence[10]; /* 0x0059d520 */
short nTargetFacing; /* 0x0059d52a */
FixedVector aShipDestination[10]; /* 0x0059d530 */
short aasShipShield[10][2]; /* 0x0059d5b0 */
enum ShipTactic aeShipTactic[12]; /* 0x0059d5e0 */
signed char abShipExhaustHeat[10]; /* 0x0059d610 */
short nRenderedSpaceFrame; /* 0x0059d61a */
short nEyePitchGoal; /* 0x0059d61c */
signed char acShipStress[16]; /* 0x0059d620 */
short anRollGoal[16]; /* 0x0059d630 */
enum Side aeShipSide[12]; /* 0x0059d650 */
signed char acShipAiCooldown[16]; /* 0x0059d680 */
FixedVector vCollisionDelta; /* 0x0059d690 */
signed char acLastCollisionObject[SPACE_OBJECT_COUNT]; /* 0x0059d6a0 */
short aasShipMaximumShield[10][2]; /* 0x0059d6e0 */
short asObjectCollisionRadius[64]; /* 0x0059d710 */
signed char acShipPointingMode[16]; /* 0x0059d790 */
short anPitchGoal[16]; /* 0x0059d7a0 */
signed char abShipNavPointIndex[16]; /* 0x0059d7c0 */
signed char acTurnInterval[16]; /* 0x0059d7d0 */
short anObjectRollRotation[SPACE_OBJECT_COUNT]; /* 0x0059d7e0 */
signed char abShipTurn[16]; /* 0x0059d860 */
HazardField aHazardFields[8]; /* 0x0059d870 */
short nFacingToTarget; /* 0x0059d920 */
short asObjectScreenY[SPACE_OBJECT_COUNT]; /* 0x0059d930 */
short asObjectScreenX[SPACE_OBJECT_COUNT]; /* 0x0059d9b0 */
short asObjectAnimationIndex[SPACE_OBJECT_COUNT]; /* 0x0059da30 */
signed char cCockpitView; /* 0x0059dab0 */
MissionObjective aMissionObjectives[MISSION_OBJECTIVE_COUNT]; /* 0x0059dac0 */
enum ShipManeuver aeShipManeuver[12]; /* 0x0059dcb0 */
const short *apCannedSequence[12]; /* 0x0059dce0 */
FixedVector aShipMissionSpot[10]; /* 0x0059dd10 */
short asCapitalShipViewFrame[16]; /* 0x0059dd90 */
signed char acObjectCollisionGraceTicks[ /* 0x0059ddb0 */
    SPACE_OBJECT_COUNT];
ObjectResourceSlot aObjectResourceSlots[4]; /* 0x0059ddf0 */
signed char acShipDestroyedWeaponCount[16]; /* 0x0059de30 */
unsigned short awObjectScale[SPACE_OBJECT_COUNT]; /* 0x0059de40 */
signed char acVduModeStackDepth[16]; /* 0x0059dec0 */
signed char acShipSpawnNavPoint[16]; /* 0x0059ded0 */
short asShipAccumulatedDamage[SPACE_OBJECT_COUNT]; /* 0x0059dee0 */
short nCurrentNavPoint; /* 0x0059df60 */

/* Function start: 0x433910 */
void show_info_disp(void)
{
    char objectiveMarker[2];
    char objectiveSummary[20];
    short objective;
    char *marker;

    set_new_vdu(0);
    DrawTextAt(&stLeftVduTextContext,
               stLeftVdu.left, stLeftVdu.top,
               " INFO DISPLAY \n", 0);
    DrawFormattedText("Version: %s\n", pGameVersion);
    DrawFormattedText("Series %d Misn %d\n",
                      (int)stCampaignState.currentSeries,
                      (int)stCampaignState.currentMission);
    DrawFormattedText("PC Kills   =%d\nWing Kills =%d\n",
                      (int)nPlayerKillCount,
                      (int)nWingmanKillCount);
    DrawFormattedText("Series Score   =%d\n",
                      (int)stCampaignState.seriesScore);
    DrawFormattedText("Miss Score %d, Medal %d\n",
                      (int)stCampaignState.missionScore,
                      (int)nMissionMedalScore);
    DrawFormattedText("Promotion Score=%d\n",
                      (int)stCampaignState.promotionScore);
    DrawFormattedText("OBJECTIVES\n");
    strcpy(objectiveSummary, "");
    for (objective = 0; objective < 16; objective++) {
        marker = " ";
        if (objective < cMissionObjectiveCount) {
            sprintf(objectiveMarker, "%c", objective % 10 + '1');
            marker = objectiveMarker;
            if (sighted(objective) != 0)
                marker = "S";
            if (visited(objective) != 0)
                marker = "V";
            if (achieved(objective) != 0)
                marker = "A";
        }
        strcat(objectiveSummary, marker);
    }
    DrawFormattedText("%s\n", objectiveSummary);
}
