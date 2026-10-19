/* Objective arrival, warp transitions, mines, and cockpit cleanup.
 * Mac targ symbols corroborate the 42a8f0-42acff range.
 */
#include "wc1.h"

/* Function start: 0x42A8F0 */
short find_objective(int type, short index)
{
    short objective;

    objective = 0;
    while (objective < (short)cMissionObjectiveCount) {
        if (aMissionObjectives[objective].type == type) {
            if (index == -1)
                return objective;
            if (aMissionObjectives[objective].index == index)
                return objective;
        }
        objective++;
    }
    return -1;
}

/* Function start: 0x42A950 */
void arrive_from_warp(short obj)
{
    short objective = find_objective(0, nCurrentNavPoint);

    if (objective != -1) {
        if (aMissionObjectives[
                abFlightPath[objective]].type != 1)
            flag_objective(objective, 1);
        if (cCurrentObjective == objective)
            set_next_destination();
    }
    approve_xyz(obj, 2000, 5000);
    unwarp(obj);
    anShipSpeed[obj] =
        (int)asShipMaximumSpeed[obj] << 8;
    fix_velocity(obj);
    if (aeShipSide[obj] == SIDE_IMPERIAL)
        reset_mission_type(obj, MISSION_TYPE_COME_HOME);
    else
        reset_mission_type(obj, MISSION_TYPE_PATROL);
}

/* Function start: 0x42AA10 */
unsigned int unwarp(short obj)
{
    short effect;

    ClearViewport(&stSpaceBuffer, cViewportClearColour);
    bViewportDirty = 1;
    effect = find_vacant_3d_object();
    if (effect != -1) {
        set_objects_data(effect, OBJECT_TYPE_HYPERSPACE_JUMP_FLASH, obj);
        aShipPosition[effect] = aShipPosition[obj];
        aShipVelocity[effect] = aShipVelocity[obj];
        aeShipManeuver[obj] = MANEUVER_NONE;
        asObjectCounter[obj] = 6;
        return 0;
    }
    abShipNavPointIndex[obj] =
        (signed char)aeObjectType[obj];
    set_objects_data(obj, OBJECT_TYPE_HYPERSPACE_JUMP_FLASH, obj);
    return 0;
}

/* Function start: 0x42AAF0 */
unsigned int warp(short obj)
{
    short effect;

    ClearViewport(&stSpaceBuffer, cViewportClearColour);
    bViewportDirty = 1;
    effect = find_vacant_3d_object();
    if (effect != -1) {
        set_objects_data(effect, OBJECT_TYPE_HYPERSPACE_JUMP_FLASH,
                         acObjectOwner[obj]);
        aShipPosition[effect] = aShipPosition[obj];
        aShipVelocity[effect] = aShipVelocity[obj];
        aeShipManeuver[obj] = MANEUVER_WARPING_OUT;
        asObjectCounter[obj] = 6;
        return 0;
    }
    set_objects_data(obj, OBJECT_TYPE_HYPERSPACE_JUMP_FLASH,
                     acObjectOwner[obj]);
    return 0;
}

/* Function start: 0x42ABD0 */
int drop_player_mine(short obj)
{
    enum ObjectType type;
    short weapon;

    for (weapon = 0; weapon < (signed char)aShipWeapons[obj][0]; weapon++) {
        type = ((ShipWeaponSlot *)&aShipWeapons[obj][1])[weapon].type;
        if (aObjectTypeData[type].objectClass == OBJECT_CLASS_MINE &&
            ((ShipWeaponSlot *)&aShipWeapons[obj][1])[weapon].disabled == 0)
            return drop_mine(obj, (signed char)weapon, type, 20);
    }
    return -1;
}

/* Function start: 0x42AC50 */
unsigned int personality_killed(short personality)
{
    if (personality < 8) {
        stCampaignState.personalityDeathMission[personality] =
            (int)stCampaignState.currentMission +
            (int)stCampaignState.currentSeries * 4;
        stCampaignState.promotionScore = MaxShort(
            0, (short)(stCampaignState.promotionScore - 1));
        return 0;
    }
    kill_ace((short)(personality - 9));
    stCampaignState.promotionScore++;
    stCampaignState.missionScore += 25;
    return 0;
}

/* Function start: 0x42ACC0 */
void clean_up_cockpit(void)
{
    acShipTarget[0] = -1;
    nTargetLockMode = 0;
    if (nYourWingman != -1) {
        nAutoEngageTimer = -1;
        acShipTarget[nYourWingman] = -1;
        reset_objective(nYourWingman, OBJECTIVE_HOLD_FORMATION);
    }
    ClearHudGunReadouts();
}
