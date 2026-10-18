/* Gun and release-weapon selection.
 * Mac select symbols corroborate the 42ad00-42af9f range.
 */
#include "wc1.h"

/* Function start: 0x42AD00 */
short find_next_gun(short obj, enum ObjectType currentGun)
{
    unsigned char *loadout = aShipWeapons[obj];
    int foundCurrent = 0;
    short weapon = 0;
    short firstGun = -1;
    short weaponCount = (signed char)loadout[0];
    ShipWeaponSlot *weaponSlot = (ShipWeaponSlot *)&loadout[1];

    if (weaponCount > 0) {
        do {
            enum ObjectType type = weaponSlot->type;

            if (aObjectTypeData[type].objectClass ==
                    OBJECT_CLASS_PROJECTILE) {
                if (firstGun == -1)
                    firstGun = (short)type;
                if (foundCurrent == 0) {
                    if (currentGun == type)
                        foundCurrent = 1;
                } else if (currentGun != type) {
                    return (short)weaponSlot->type;
                }
            }
            weapon++;
            weaponSlot++;
        } while (weapon < weaponCount);
    }
    if (foundCurrent != 0 && firstGun != currentGun)
        firstGun = 0x80;
    return firstGun;
}

/* Function start: 0x42ADA0 */
int select_guns(short obj, short selectedGun)
{
    short weaponCount;
    int found;
    ShipWeaponSlot *weaponSlot;

    (void)obj;
    weaponCount = (signed char)aShipWeapons[0][0];
    found = 0;
    weaponSlot = (ShipWeaponSlot *)&aShipWeapons[0][1];
    if (weaponCount > 0) {
        do {
            if (aObjectTypeData[weaponSlot->type].objectClass ==
                    OBJECT_CLASS_PROJECTILE) {
                if (selectedGun == weaponSlot->type || selectedGun == 0x80) {
                    weaponSlot->disabled = 0;
                    found = 1;
                } else {
                    weaponSlot->disabled = 1;
                }
            }
            weaponSlot++;
            weaponCount--;
        } while (weaponCount != 0);
    }
    if (found != 0)
        return selectedGun;
    return -1;
}

/* Function start: 0x42AE10 */
unsigned int select_new_gun(void)
{
    eSelectedGunType = (enum ObjectType)select_guns(
        0, find_next_gun(0, eSelectedGunType));
#ifdef SDL_PORT
    /* SWC CODE_15 +0x0222: fifteen status-text updates per selection. */
    if (SdlSwcFlightActive())
        anSwcWeaponNoticeTicks[0] = 15;
#endif
    if (get_mode(0) == 1)
        InvalidateVduMode(0);
    return 0;
}

/* Function start: 0x42AE50 */
unsigned int select_new_release_weapon(enum ObjectType preferredType)
{
    signed char weaponCount;
    int currentWeapon;
    short firstWeapon;
    signed char weapon;
    ShipWeaponSlot *weaponSlots;

    weaponCount = (signed char)aShipWeapons[0][0];
    currentWeapon = nSelectedReleaseWeaponIndex;
    weapon = (signed char)(currentWeapon + 1);
    weaponSlots = (ShipWeaponSlot *)&aShipWeapons[0][1];

    if (weaponCount <= weapon)
        weapon = 0;
    if (currentWeapon == -1) {
        if (preferredType != -1) {
            weapon = 0;
            if (weaponCount > 0) {
                for (; weapon <
                           (signed char)aShipWeapons[0][0];
                     weapon++) {
                    if (weaponSlots[weapon].type == preferredType) {
                        currentWeapon = weapon;
                        weaponSlots[currentWeapon].disabled = 0;
                        break;
                    }
                }
            }
        }
        nSelectedReleaseWeaponIndex = currentWeapon;
        if (currentWeapon == -1) {
            weapon = 0;
            if (weaponCount > 0) {
                for (; weapon < weaponCount; weapon++) {
                    if (aObjectTypeData[
                            weaponSlots[weapon].type].objectClass !=
                            OBJECT_CLASS_PROJECTILE) {
                        currentWeapon = weapon;
                        nSelectedReleaseWeaponIndex = currentWeapon;
                        weaponSlots[currentWeapon].disabled = 0;
                        break;
                    }
                }
            }
        }
    } else {
        firstWeapon = weapon;
        do {
            enum ObjectType type;

            if (currentWeapon == weapon)
                break;
            type = weaponSlots[weapon].type;
            if (aObjectTypeData[type].objectClass !=
                    OBJECT_CLASS_PROJECTILE &&
                weaponSlots[currentWeapon].type != type) {
                weaponSlots[currentWeapon].disabled = 1;
                currentWeapon = weapon;
                nSelectedReleaseWeaponIndex = currentWeapon;
                weaponSlots[currentWeapon].disabled = 0;
                break;
            }
            weapon++;
            if (weaponCount <= weapon)
                weapon = 0;
        } while (weapon != firstWeapon);
    }
#ifdef SDL_PORT
    /* SWC CODE_15 +0x0398, including automatic selection after firing. */
    if (SdlSwcFlightActive())
        anSwcWeaponNoticeTicks[1] = 15;
#endif
    if (get_mode(0) == 1)
        InvalidateVduMode(0);
    return 0;
}
