# Wing Commander VR (Meta Quest) — stage 1

Standalone Quest build of the wc1-re reconstruction (SDL2 port of the 1990 DOS
original).  Stage 1 shows the game on a large world-fixed screen in the
headset, with a laser pointer for menus, a VR keyboard for names, and the Touch
controllers as a flight stick.  Stereo 3D space and head-look come in stage 2.

## Install

1. Install the APK:
   `adb install -r WingCommanderVR.apk`
2. Copy the game data. Use the GOG *Wing Commander* (DOS) installer and unpack
   it on the PC, e.g. `innoextract setup_wing_commander_1.0_(28045).exe`.
   Copy `WC.EXE` and the whole `GAMEDAT` folder to the headset:
   ```
   adb shell mkdir -p /sdcard/WingCommander/GAMEDAT
   adb push WC.EXE /sdcard/WingCommander/
   adb push GAMEDAT/. /sdcard/WingCommander/GAMEDAT/
   ```
3. Start *Wing Commander VR* from Library → Unknown Sources. On the very first
   start, allow "Access to all files" when asked (or:
   `adb shell appops set com.bilko.wc1vr MANAGE_EXTERNAL_STORAGE allow`).
   A red screen means the game data was not found.

Saved games and pilot rosters are written to `/sdcard/WingCommander/GAMEDAT`
as in the DOS game.

## Controls

**Menus, rooms and briefings** (laser beam from the controller):

| Control | Action |
| --- | --- |
| Aim + trigger | Left click (either hand; the hand that pulls last aims) |
| Grip | Right click |
| A | Enter |
| B or Menu | Escape |
| Y / X | Answer "Y" / "N" |
| A / B in a "(Y/N)" prompt (load, save, quit) | Yes / No |
| Hold left stick click 1 s | Recentre the screen in front of you |

Text prompts (name, callsign, saved games) open a keyboard below the screen:
point and pull the trigger. Shift is on for the first letter and after a space.

**Flying:**

| Control | Action |
| --- | --- |
| Left stick | Pitch / yaw |
| Right stick left/right | Roll |
| Right stick up/down | Faster / slower |
| Right trigger | Guns |
| Left trigger | Fire missile |
| Right grip (hold) | Afterburner |
| A | Cycle target |
| B | Select missile |
| X | Select guns |
| Y | Pause |
| Menu | Escape |
| Left stick click | Navigation map |
| Right stick click | Autopilot |
| Left grip + A | Target lock |
| Left grip + B | Stop |
| Left grip + X | Communications (then left grip + Y/B to choose, A to send, X to close) |
| Left grip + Y | Full speed |

## Building

```
quest/tools/fetch_deps.sh                  # SDL2 2.32.10 source + Java glue
cd quest/android && ./gradlew assembleDebug
```
Needs the Android SDK (platform 34, build-tools 34, CMake 3.22.1), NDK
25.1.8937393 and JDK 17. Output: `app/build/outputs/apk/debug/app-debug.apk`.

`quest/tools/quest_run.sh` builds, installs, launches and pulls a headset
screenshot; `quest/tools/quest_cmd.sh` drives the game headless through the
developer command file `/sdcard/WingCommander/vrcmd` (`click X Y`, `move X Y`,
`key NAME`, `type TEXT`, `fake FIELD VALUE MS`, `wait`).

## Code map

* `src/quest/quest_xr.c` — OpenXR thread: EGL context, session, virtual
  screen quad, keyboard panel, laser beam, Touch sampling, haptics, recentre.
* `src/quest/quest_input.c` — game-side glue: data directory, logcat, virtual
  SDL gamepad for the port's 4button-4axis joystick mode, laser → mouse,
  keyboard → key events.
* `#ifdef WC1_QUEST` hooks in `src/sdl/{video,events,joystick,launcher,compat}.c`
  and `src/trainsim.c` (`ReadTextInput` shows the keyboard).
* `quest/CMakeLists.txt`, `quest/android/` — NDK/Gradle build.

## Stage 2: stereo space

In flight, space is rendered per eye: ships, missiles and dust are billboards at
their real 3D position, stars sit on a fixed sphere at infinity, and you can
look around with your head.  The cockpit art is a panel 0.9 m in front of you;
HUD pixels in the canopy window (reticle, target brackets) are on a panel at
8 m.  Tuning goes in `/sdcard/WingCommander/vr.cfg` (`world_scale=`,
`dash_distance=`, `hud_distance=`, `stretch=`, `stereo=0` to disable).

**3D cockpit:** the 2D cockpit art is projected from the pilot's eye onto a
curved shell, so it matches the original pixel for pixel from the default
head position. The console tilts down toward your lap (about 0.75 m to
0.45 m away), the canopy frame arches away overhead (about 1 m), and the sides
curve in. Where the window's bottom edge sits in each column decides what is
console and what is canopy, so this works for every ship's cockpit. Outside the
original art, the edges wrap around you and darken. `cockpit3d=0` brings back
the flat panel; `cockpit_wrap=0` disables the wrap-around. `dash_distance=`
scales the whole shell.
