# Super Wing Commander Mac demo

The GUI recognizes the SWC Mac demo, plays its splash, Lion logo and prologue,
then shows the original title artwork over an animated battle. New Game plays
the four-part campaign opening and opens the Tiger's Claw bar and barracks.
Talk to Shotglass, Paladin and Angel, check pilot rankings or medals, then enter
the briefing room and launch the experimental Enyo 1 flight with five ships,
four objectives, and the original 320x240 cockpit and sprites.
SDL2 handles input and presentation; WC1 supplies mission state, ship setup,
flight dynamics, movement, ship-view selection, navigation, and
player weapons, including projectile collisions, damage, and missile guidance.
Flight now uses the recovered SWC ship/weapon definitions, shared WC1 NPC AI,
autopilot, encounter spawning and asteroid fields. The targeting lead indicator
adapts WC2's matching calculation to SWC's original projection and cockpit art.
The Hornet cockpit now has speed and navigation readouts, fuel/throttle/energy
gauges, player/target shield and armor displays, radar contacts, weapon status,
navigation and hostile-direction markers, warning lamps, and damaged displays.
Cockpit banking, the pilot's hand, native flight effects, warning sounds,
computer speech and animated communications portraits are connected.
Flight projection, target-bracket geometry/artwork and pilot/ace target names
now follow the Mac instructions. Ctrl+E ejects, with the original external
camera, abandoned-ship explosion and rescued/stranded mission outcome.
The original conversations, briefing, debriefing and funeral scenes use the
demo's text, portraits and AIFF speech. Prologue, opening, Hornet launch, landing and
funeral movies have embedded audio. QuickTime music sequences now accompany
the startup, opening, carrier, briefing/debriefing, launch, flight, landing and funeral.
The M-key computer has navigation, communications, guns, weapons and eject tabs.
Communications provide wingman orders, taunts and landing-clearance requests;
returning to the carrier ends Enyo 1 with the original debriefing branches and
Spirit's funeral if she was lost. Player destruction leads through the death
cinematic and funeral to animated THE END. Losing the carrier with no nearby
enemies plays BUC and the narrated stranded ending. Completed flights/endings
return to the title menu; New Game always starts fresh Enyo 1.
The launcher and command line support enhanced OpenGL, joystick modes and axis
layouts, rumble, cockpitless view, and cheat mode (`--cheats`: player
invulnerability and no player collision response). The cheat choice survives
title-screen combat and subsequent New Game starts. EGA dithering remains
WC1-only because its conversion table depends on the DOS palette. See
[SDL2.md](SDL2.md) for the graphics options and controller layouts.
Later missions, campaign progression, save/load, remaining HUD modes and
side/rear views are still pending. These paths have been compiled and inspected
without interactive execution.

`make modern-swc` builds the asset viewer. It loads the demo's CMF files,
decodes sprites, reads the resource-fork palette, and displays the cockpit and
13 ship shape sets at 320x240. Left/right select a ship view, up/down select a
ship, C toggles the cockpit, and Esc exits.

`src/swc/` contains the resource readers. `src/sdl/swc_mission.c` translates
Mac mission records into WC1 state through `SdlLoadSwcMissionData`, called
directly by the SWC flight host. WC1's `LoadMissionData` retains its WC1-only
path. The CMF reader and mission adapter are linked into
`wc1-modern`; the original Win32 reference build does not include them.
`src/sdl/swc_demo.c` remains a separate presentation/inspection entry point.
`src/sdl/swc_flight.c` hosts the experimental flight using the shared core.
`src/sdl/swc_cockpit.c` presents the Hornet instruments. `src/sdl/swc_text.c`
shares Helvetica text on macOS and portable bitmap fallback text. `src/sdl/swc_nav.c`
presents the five-page in-flight computer. `src/sdl/swc_rooms.c`
adapts carrier presentation and scenes; `src/swc/speech.c` reads AIFF speech.
`src/swc/sound.c` decodes the flight `snd ` bank; `src/sdl/swc_sound.c`
supplies a shared stereo SDL mixer for four effect/flight-speech voices,
foreground movie/dialogue PCM and music. `src/swc/music.c` reads QuickTime
music sequences; `src/sdl/swc_music.c` selects and synthesizes them.
`src/swc/movie.c` reads and decodes the Mac movies; `src/sdl/swc_movie.c`
supplies SDL2 presentation, foreground audio, and the startup/opening/launch/landing/funeral
sequences.
The viewer needs only the host C compiler and SDL2. Flight links the existing
native core and needs its normal build dependencies, including LZO.
Flight uses SDL2 2.0.18 or later for the VDU polygon rendering.

## Status review — 2026-10-12

The current scope is an experimental Enyo 1 session. Resource loading and the
initial flight checks passed; a complete interactive mission and presentation
parity with the Mac executable are still unverified. The user has confirmed the
autopilot works. The autopilot and sound/cockpit reconstructions below follow
the initial status review; no game was launched for either implementation.

Fresh Ghidra instruction exports were checked for CODE_01, 02, 03, 04, 05, 06,
09, 12, 13 and 14. All thirteen CODE-resource hashes in the provenance inventory
match the local originals. The GameFlow switch targets at CODE_09 +0x1d68 were
explicitly disassembled; its death/landing cases must not be inferred from a
decompilation that omits those targets. Expanded A5 data resolves the intersegment
calls and strings. Local exports and check logs are under
`out-modern/swc-status-review/`; no SWC binary-similarity score was produced.

The initial resource review decoded all 23 CMFs (1,011 chunks, 267 sprite sets,
3,446 frames), 38 movies (3,521 frames), and 92 AIFF recordings. Enyo 1 loaded
five ships and four objectives. Regenerating `swc_objects.h` from CODE_08 and
expanded DATA/A5 produced byte-identical output. Those earlier automated checks
and their build targets have since been removed. They did not establish a full
mission's gameplay, appearance, audio synchronization or original timing.

The main confirmed differences are:

| Area | Original evidence | Current behavior / implication |
| --- | --- | --- |
| Damage probabilities | CODE_05 `your_internal_damage` +0x1830 calls `random_number(10)`; +0x1862 calls `random_number(9)`. CODE_04 `dp_random` +0x5d44 returns a remainder modulo its argument. | `src/ship.c` still uses inclusive `RandomBelowOrEqual(10)` and `(9)`: severity can be 10 instead of 0..9, and table selection can include index 9 instead of 0..8. The existing SWC cockpit-region correction fixes only that separate draw. |
| Other pilots' post-mission statistics | CODE_09 `PostMission` +0x1794..+0x17b0 calls CODE_04 `Random3DO` +0x5dea with `(0,2)` and `(0,playerKills)`. Its equal-bound case returns zero. | WC1 `PostMission` uses inclusive `RandomInRange`; it can add two missions rather than at most one. When the player made zero kills, its `(0,0)` call can still award another pilot one kill. Common player/wingman statistics and badge control flow agree, but the whole routine is not equivalent. |
| Autopilot eligibility | CODE_13 `auto_pilot_valid` +0x178a..+0x1880 checks an escort-wait flag; A5-0x1ff0 contains `Wait for %s`. | Resolved in the SDL-only SWC adapter: hazard/enemy/escort/distance priority and the objective-arrival wait latch now drive both A and the lamp. |
| Movement timing | CODE_02 `accelerate_and_move_object` +0x43ba clamps elapsed Mac ticks to 0..6, then +0x471e scales velocity before adding it to position. A5-0x377a holds Q8 factors 128,154,205,256,320,384,512. | The host adds velocity once per fixed 50 ms step. That matches the table's factor at three Mac ticks; variable-tick behavior remains different. |
| Campaign and death flow | CODE_09 `GameFlow` calls `PostMission` at +0x1f46 and `UpdateSeries` at +0x1f4a; its death case calls CODE_14 `death_sequence` at +0x1e1c before the player funeral at +0x1e44. | Ejection, the death cinematic, funeral, stranded sequence and animated endings are connected. The supplied demo omits the CKPT/17 ejected-pilot art. Enyo 1 completion returns to the title menu; New Game resets the campaign. Save/load and later progression remain absent. |

Two apparent rendering/HUD gaps need qualification. CODE_01
`rotate_Mac_shape2` +0x10ae constructs a centered quad from frame width/height
and scale (+0x10ec..+0x113c); it does not apply the frame's signed x/y offsets.
The host's centered ship placement therefore has original-code support. Those
offsets belong to the separate unscaled `MacDraw2` path (+0x0f20..+0x0f3c).
Exact rotation rounding, sampling and clipping still require comparison; adding
frame offsets to all ship sprites would not follow this evidence.
CODE_13 `show_navigation_disp` +0x121a, `show_damage_disp` +0x123a and
`show_info_disp` +0x1256 are empty in the original demo. The simulator's
unavailable message is also original, not a missing implementation to fill in.

Flight effects and voices now use the native Mac sound bank through
`SdlPlayGameSoundEffect` and the SDL cockpit adapter. Startup splash/logo/prologue
and the new-campaign opening are connected before the carrier visit. The title
battle, idle intro replay, death cinematic and animated endings are connected.
Movie palette dimming, the native briefing map and room animations are connected.
Exact classic font pixels require an optional Helvetica resource; frame pacing
and interactive visual parity remain unverified.
The most useful next work is correcting the demonstrated RNG differences,
then inspecting a full Enyo 1 route through combat and return before
expanding campaign support. Other AI/collision paths still need individual
instruction comparisons; this review does not establish their equivalence.

### Autopilot reconstruction

The SDL counterparts of CODE_13 `auto_pilot_valid` (+0x1732) and
`flag_reached` (+0x212e) are now implemented in `src/sdl/swc_cockpit.c`.
They preserve the hazard/enemy/escort/distance priority, the four original
`Wait for %s` ship types, and the DATA/A5+0x46ec escort latch. Every arrival
check clears that latch; arriving ahead of the escort sets it again. The
eligibility routine retains the original +0x179a ship-ID indexing, with a host
array-bound guard. Speech IDs 1..7 now use the native cockpit message queue
and sound bank; arrival uses voice ID 0.

`SdlSwcVisitCinema` in `src/sdl/swc_flight.c` reconstructs CODE_14
`visit_the_cinema` (+0x000c). Fresh Ghidra disassembly of CODE_02's view switch
tables confirms the view-12 setup at +0x3474: 500 units to the side and 2,000
ahead, looking at the player. The camera update at +0x29f0 moves ten units
sideways per frame. The host supplies SDL pause/focus/quit handling and plays
the original sound 17 until the sequence ends; its pacing remains the existing
fixed flight interval. It restores the prior camera on exit.

Pressing A now runs this 120-frame external view before the shared formation
travel. The renderer includes the player's ship and omits cockpit/HUD overlays.
The host updates one objective per frame, including unselected objectives, as
CODE_13 +0x0316..+0x034e does. P and focus loss pause the sequence; Escape or
Backspace skips the camera on release and lets travel complete normally.
Window close exits the session. Camera and input state are restored on exit, and a
presentation failure propagates to the mission host. CODE_14 +0x077e's final
simulation frame runs after travel.

All shared-core dispatch is guarded by `SDL_PORT` and `SdlSwcFlightActive()`;
SWC state and presentation live in the SDL sources. Native and GUI builds pass.
`binary-comp` reports identical Win32 scores before and after integration:
`visit_the_cinema` 100%, `auto_pilot_valid` 100%, `auto_pilot_sequence` 96.91%,
and `flag_reached` 85.84%. The user subsequently confirmed autopilot works;
agent validation was limited to source/assembly inspection and compilation.
No SWC binary similarity score is claimed.

### Flight sounds and cockpit additions

Fresh Ghidra exports of CODE_01, 06, 09, 13 and 14 supply the sound-channel,
speech, message and animation behavior. `Sounds.CMF` contains 494 standard
Sound Manager `snd ` resources: unsigned eight-bit mono PCM, with the original
16.16 rate `0x2b7745d1` (about 11,127 samples/second). The afterburner release
is a separate `snd /500` in the executable's resource fork, at rate
`0x56ee8ba3`. The host keeps these rates when resampling to its SDL output.

| Feature | Original evidence | SDL behavior |
| --- | --- | --- |
| Sound channels | CODE_01 `init_sound` +0x6052, `play_sound` +0x626e, `is_sound_playing` +0x5f8e | Four mono channels; an active sound ID is not restarted, and a full mixer does not interrupt another sound. The bank is decoded before flight. |
| Weapons and impacts | CODE_09 `sound_effect` +0x1508; CODE_05 `fire_weapon` +0x379a | Shared weapon/damage calls use native samples and 100-unit distance attenuation. Enemy lasers select sound 26. |
| Afterburner | CODE_01 `play_looped_sound` +0x667e, `stop_afterburner_sound` +0x678a | Sound 12 loops over samples `[0x23f8,0x32f5)`; release replaces it with resource-fork sound 500 in the same channel. |
| Cockpit warnings | CODE_13 `update_lights` +0x04fc, `update_VDUs` +0x0b4e, `malf_sound` +0x19e4 | Native damage alarm 32, malfunction static 23 and impact sound 24. |
| Computer messages | CODE_13 `CockpitMessage` +0x01aa and `UpdateMessage` +0x000c | Sixteen queued, copied messages, 34-character limit, queued duplicate suppression, flash cadence and voice IDs `79+message`. Arrival/autopilot speech is connected. |
| Flight communications | CODE_06 `Vocalize_Message` +0x045c, `real_vid_transmit` +0x049e, `check_message` +0x0640; CODE_13 `npc_communication` +0x3d2e | Player order voices, native response table, rank variants, Kilrathi chatter and thirty-second repeat throttling. Playback completion closes portraits and restores the prior VDU mode. |
| Portrait animation | CODE_13 `update_vid_disp` +0x0d00; DATA/A5 +0x4e6 and +0x2cb4 | WGMN face and mouth frames use the original portrait polygon, with a one-in-four mouth toggle and communication-component malfunction static. |
| Banking and pilot hand | CODE_13 `handle_cockpit_banking` +0x18f2, `determine_pilot_hand` +0x14e2 | Cockpit and instruments approach the pitch/yaw offset by one pixel per tick, capped at ±6 vertically and ±8 horizontally. PC00/6 supplies sixteen hand frames. |
| Flight frame | CODE_01 `DrawBorder` +0x065c, `dump_buffer_to_screen` +0x06dc | A fixed palette-255 frame covers 8 pixels at each side and 6 at the top/bottom after all flight layers. The interior `(8,6)-(312,234)` stays covered throughout cockpit banking; steering no longer exposes moving black strips at the artwork edges. |

SDL replaces Sound Manager playback and QuickDraw rendering. Pause/focus loss
pauses the shared mixer. One device stays open across carrier, movie and flight
transitions and closes before session resources are freed. If it is unavailable, timed text/portraits keep the
message queue progressing. Cockpit text uses Helvetica 9 on macOS and the
portable bitmap fallback elsewhere. Computer messages follow component/weapon
notices in the original status area. Weapon selection lasts fifteen updates;
unvoiced messages use the forty-update countdown and flashing retains its
twenty-update reset across messages. Queue overflow drops a new message.
Communications use base-game speakers for Enyo 1, WC1's RNG with adjusted bounds,
and the host simulation clock; later campaign voice substitutions are absent.

The earlier WC2 findings identify `HasInRangeGunForTargetLead` (0x43ce8f) and
`UpdateTargetLeadIndicator` (0x43cf5a), adapted for ITTS. The targeting pass also
adapts `check_target` (0x43ddfc) and `target_locking` (0x43c048), using the Mac
instructions for acquisition, lock timing, malfunction and display differences.
The matching target-list, sorting and cycling functions remain shared with WC1.
Side/rear cockpit art (PC00/2..4) is
absent from this demo's cockpit CMF.

Validation uses original instructions/data, static review and compilation of
the Win32 reference, native flight, GUI and asset viewer. `binary-comp` scores
for the two new guarded core hooks are unchanged: `message_showing` 100.00%,
`real_vid_transmit` 98.53%. No tests were added or run; no game, GUI or audio
playback was executed. The new sound mix, portrait appearance and timing remain
unverified interactively.

The flight-frame correction was checked against the original Ghidra instructions
and clut/251, then compiled with `make modern modern-gui modern-swc` and reviewed
with compiler static analysis. No tests or game execution were used for this fix.

### QuickTime music

The demo's 27 `MIDI/*.MooV` files contain QuickTime `musi` note sequences,
not sampled audio. Their data forks hold `mdat` events; the `moov/128`
resource supplies instrument requests, the 600-tick time scale, sample sizes,
chunk offsets and sample durations. `SwcReadMusic` bounds these structures and
decodes rests, short/extended notes, controllers and markers. Notes can cross
sample boundaries. Extended pitches below 128 are integer MIDI keys; larger
ones are 8.8 values. Controller 32 is signed 8.8 pitch bend in semitones.
Every instrument header ends with `0x60000000`; twelve retain zero padding
after it. `BRIEFIN0` uses a fixed 840-byte sample in `stsz`, while the other
tracks carry individual sample sizes. Both layouts are accepted.

Preserve each music file's resource fork. The reader accepts a native macOS
fork, a raw `<filename>.rsrc` sidecar, an AppleDouble `._<filename>` sidecar,
or a flattened movie with `moov` in its data fork. Copying only the original
data forks loses the track metadata. Missing or unsupported tracks are silent
and report a warning when selected; effects and speech remain available.

Selection follows Ghidra exports of CODE_09 `gametrack` (+0x1168),
`spacetrack` (+0x0d1e), `servicetrack` (+0x13d0),
`new_space_music_changes` (+0x0e88), and their original scene callers:

| Context | Track |
| --- | --- |
| Startup splash | `INTRO.MooV` |
| Prologue | `SUPRWNG1.MooV` through `SUPRWNG4.MooV`, at the original clip/dialogue boundaries |
| New-campaign opening | `DOCKING.MooV` |
| Bar and pilot rankings | `RECROOM.MooV` |
| Barracks and medals | `BARRACKS.MooV` |
| Briefing | `BRIEFIN0.MooV` |
| Launch / landing | `ARMOR.MooV` / `LANDING.MooV` |
| Initial patrol / quiet navigation sphere | `DULLPATR.MooV` |
| Hostile sphere | `FLOUNDER.MooV` or `REGULARC.MooV` |
| Asteroid/mine sphere | `GOALLINE.MooV` |
| Successful patrol return / incomplete return | `RETURNI0.MooV` / `RETURNIN.MooV` |
| Debriefing above 70% / at most 70% | `DEBRIEFI.MooV` / `DEBRIEF0.MooV` |
| Funeral movies and both speeches | `FUNERAL.MooV`, continuous across the sequence |

The original filename table also references `FLYINGTO.MooV` (initial strike),
`RETURNI1.MooV` (successful non-patrol return), and `DEBRIEF1.MooV`
(zero full mission score). None is supplied in this demo; these names are not
aliased to other tracks. Navigation cues change only when the action sphere
changes. WC1's same-named music function instead takes attacker/victim arguments
and uses different rules. Completed tracks loop, and reselecting an active track
preserves its position. Original scene volume is doubled to a percentage:
opening 30, flight 25, prologue 30 or 50 at the original speech/movie boundaries,
other connected scenes 50.

The return cue also follows CODE_05 `triumph` +0x69a2..+0x69ce: a patrol is
complete only when every flight-path objective was visited. WC1's current
patrol branch returns success on an incomplete route, so the SDL music adapter
performs this check locally. Other mission types share the existing predicate.

TinySoundFont renders the sequences at 48 kHz with the bundled GeneralUser GS
2.0.3 bank. `make modern` and `make modern-gui` copy `swc-music.sf2` beside the
executables; release archives include it and both component licenses.
`WC1_SWC_SOUNDFONT` optionally selects another complete General MIDI bank.
Music has host mix headroom and honors the existing 0..20 music-volume setting.
One SDL device mixes it with foreground mono PCM and four effect voices;
pause/focus loss pauses all of them. The callback uses cached sequences and
preallocated synth channels/voices, with no file I/O or allocation.

This is a host synthesizer substitution: GeneralUser's instrument timbres,
envelopes and volume response differ from QuickTime's instruments. TinySoundFont
does not implement the sequences' reverb/chorus sends; modulation-wheel events
in the supplied data are zero. Exact mix, timing and loop transitions remain
unverified interactively. Validation for this work is original-instruction/data
inspection and native/GUI compilation only. No tests or game/audio playback.
Win32 source selection and reconstructed functions are unchanged.

Format references: Apple's [QuickTime music media specification](https://developer.apple.com/documentation/quicktime-file-format/music_media)
and [QuickTime Music Architecture event definitions](https://leopard-adc.pepas.com/documentation/QuickTime/RM/MusicAndAudio/qtma/B-Chapter/2TheQuickTimeMusicAr.html),
plus Apple's `QuickTimeMusic.h`. Pinned component sources and hashes are in
`third_party/tinysoundfont/README.md` and `third_party/generaluser/README.md`.

## Run

Build the GUI with `make modern-gui`, run `out-modern/wc1-modern-gui`, select the
extracted **SuperWing DEMO** directory, and click **Enter Tiger's Claw**. To
preselect the directory or start the carrier visit from the command line:

```sh
out-modern/wc1-modern-gui --gui --swc-demo "data/swc-demo/SuperWing DEMO"
out-modern/wc1-modern --swc-demo "data/swc-demo/SuperWing DEMO"
```

Click a person or doorway to interact; hover shows its label. Tab/arrows select
hotspots and Enter activates them. From the bar, enter the barracks through the
upper-right door, then the briefing room through its right-hand door. Click,
Space or Enter advances dialogue; Escape or Backspace returns from a conversation
or skips the briefing into launch. P pauses dialogue and speech; focus loss
also pauses both. Escape or Backspace returns from the barracks, rankings or
medals, and leaves the bar open.
Window close exits at any point. The simulator retains the demo's unavailable
message. Save/load bunks explain that SWC saves are not supported yet.

With the archive's startup assets present, the splash, Lion logo and prologue
precede the title menu. New Game plays the four-part opening and first carrier visit.
The Hornet launch follows the briefing. Space, Enter, Escape, Backspace, or a
left click skips the entire current sequence on release; P pauses it.
Splash and logo dismiss separately; skipping the prologue advances
to the title menu. Losing focus pauses video and audio together. Skipping the opening
enters the bar; closing the movie window exits the session. Once in flight,
Escape and P toggle pause. During the autopilot camera sequence, Escape or
Backspace skips the camera while travel completes; P pauses it. C closes an
open communications menu, and Space/Enter/mouse buttons resume their weapon bindings.

Flight uses WC1's DOS/Win32 mouse and keyboard controls through SDL2:

| Control | Action |
| --- | --- |
| Mouse movement | Steer using WC1's dead zones and response curve |
| Hold right button and move | Horizontal motion rolls; vertical motion changes throttle |
| Double-click right button and hold | Afterburner; release stops renewing the boost timer |
| Arrows or keypad 2/4/6/8 | Pitch/yaw with WC1's gradual steering response |
| Home/PgUp/End/PgDn or keypad 7/9/1/3 | Diagonal steering |
| Comma/period, Insert/Delete, or keypad 0/decimal | Roll |
| Shift with a steering key | WC1's fast steering response |
| Keypad 5 | Centre steering and the mouse pointer |
| +/- or keypad +/-, mouse wheel | Change commanded speed; keys repeat while held |
| Backspace | Set commanded speed to zero; the ship decelerates |
| Backslash | Set commanded speed to maximum |
| Tab or keypad * | Afterburner, subject to WC1's fuel and timer checks |
| Space / left mouse button | Fire selected guns; holding repeats while energy and cooldown permit |
| Enter / press both mouse buttons | Release one selected missile; guided weapons require a lock |
| G / W | Cycle gun selection / missile type |
| T | Cycle visible targets in range order, skipping friendlies when hostiles are available |
| L | Toggle target-lock mode; ITTS leads a hostile fighter when guns are in range |
| A | Autopilot toward the selected objective, stopping for enemies or hazards |
| N / M | Next objective / in-flight computer |
| Y / D / F | Cockpit / battle / chase view; D or F again returns to the cockpit |
| C / 1..7 | Open/close WC1 communications / choose a numbered recipient or command |
| Ctrl+E | Eject; a damaged ejector can fail, and a destroyed ejector cannot operate |
| Ctrl+F1 | Toggle cockpit |
| P / Esc | Toggle pause with a visible pause/resume message |
| Cmd+Enter (macOS), Alt+Enter (other platforms) | Toggle fullscreen in flight, menus, and movies |

M opens the original five-page in-flight computer and suspends flight. Up/Down
or clicking a tab changes pages; the last page is remembered. Left/Right select
an objective or communication entry. Tab also cycles the selection; on Guns
and Weapons, Tab/Right or clicking the loadout cycles the installed guns or
missile type. Communications use Enter, a numbered key or a clicked row to
choose a recipient and then send an order. On Eject, Enter or the eject button
attempts ejection with the same damage check as Ctrl+E.

M, Escape or Space returns to flight. Backspace returns from communication
orders to recipients, or closes the computer elsewhere. Enter also closes the
navigation page, where N and clicking a marker/label select an objective.
Controller D-pad navigates tabs/selections, A activates, and B/Back goes back.
Closing preserves the camera and recentres the flight mouse; send, eject and
close actions complete on release so the same press cannot fire on return.

The window title remains **Super Wing Commander** throughout the demo, without
control hints or flight telemetry. The pointer is confined to the flight window;
Escape/P releases it while paused. Losing focus also pauses movement and releases the pointer.
Flight, title, autopilot and death/ejection camera pauses show **GAME PAUSED**
over the frozen picture, with the appropriate resume key. The message also
appears during focus loss and disappears when play resumes.
Keyboard steering takes priority while held; moving the mouse or joystick
selects that steering device. Releasing the
right-button roll/throttle mode recentres the pointer, as in WC1. Holding both
buttons releases one missile per press; a latched right-button afterburner keeps
its existing behavior and allows left-button guns. Q/E are no longer roll bindings.

Joystick controls use the same launcher options as WC1, including all seven
four-axis layouts. The default two-button mode uses button 1 for guns, button 2
as a roll/throttle modifier, a double press and hold of button 2 for afterburner,
and both buttons for a missile. Mapped controllers use A/B for these buttons.
The improved modes use these bindings:

| Controller / joystick button | Action |
| --- | --- |
| A / 1 | Guns |
| B / 2 | Release missile |
| X / 3 | Hold for afterburner |
| Y / 4 | Roll/throttle modifier in two-axis mode; cycle target in four-axis mode |
| LB/RB / 5/6 | Cycle guns / missile type |
| Left/right stick clicks, or raw buttons 7/8 | Next objective / autopilot |
| D-pad Up/Down/Left/Right | Full speed / stop / communications / target lock |
| D-pad while communications are open | Up/Down highlights a choice, Right selects, Left closes |
| Start / Back | Toggle pause |

Controller navigation also works in carrier rooms and scenes: D-pad browses
hotspots, A confirms, B/Back goes back or skips, and Start pauses. Confirmation
uses button release, matching the keyboard behavior so scene transitions do not
reuse a held button. Joysticks can connect or disconnect during the session;
SDL's normalized axes need no `j.cal`. Optional rumble uses the shared heavy-weapon,
damage, collision, and afterburner feedback and stops on pause, focus loss,
scene transitions, disconnect, and shutdown.

To return, select the Tiger's Claw objective with N and travel back with A
when autopilot is available. Use T to target the visible carrier, then C and
the numbered choices to request landing clearance. The shared rules can deny
an immediate return or a return with enemies nearby. With clearance granted,
approach within 700 range units without afterburning to begin landing. Skipping
landing movies continues into debriefing; closing the window exits. If Spirit
was lost, her funeral follows debriefing. Player destruction instead starts the
player's funeral without landing or mission-stat updates. Click, Space or Enter
advances funeral dialogue; Escape or Backspace skips the rest of the funeral.
Skipping a funeral movie also ends the sequence. The player's completed farewell leads to animated
THE END, which waits for input. Death, stranded and completed Enyo 1 paths return
to the title menu. Escape/P pauses the title animation. Window close or the host's
quit command exits the session.

Point `SWC_DATA_DIR` at the extracted **SuperWing DEMO** directory containing
`CMFs/`. For example, from the repository root on macOS:

```sh
unar -o data/swc-demo ../releases/mac/SuperWing_DEMO.sit
make run-modern-swc SWC_DATA_DIR="data/swc-demo/SuperWing DEMO"
```

The viewer and flight host first try `SuperWingCommanderDemo.rsrc` inside that directory,
then the native resource fork of `Super Wing Commander Demo`. The palette is
`clut` ID 251, named `pcshipv00`. A stripped PEF executable does not contain it.
For the GUI or flight host, copy an extracted **raw** resource fork to
`SuperWingCommanderDemo.rsrc` inside the demo directory. The asset viewer also
accepts an explicit path:

```sh
out-modern/swc-demo "/path/to/SuperWing DEMO" \
    --rsrc ../releases/mac/extracted/SuperWingCommanderDemo.rsrc
```

AppleDouble sidecars (`._...`, or `.rsrc` from `unar -forks visible`) are containers,
not raw resource forks, and are not currently supported. On macOS, `unar`'s
default fork-preserving extraction works directly. Paths and filenames currently
use the spelling in the demo archive. Game assets stay outside version control.

The asset viewer's `--snapshot FILE.bmp` saves the decoded 320x240 frame and
exits. This artwork export can use SDL's dummy video driver:

```sh
SDL_VIDEODRIVER=dummy out-modern/swc-demo "/path/to/SuperWing DEMO" \
    --rsrc /path/to/SuperWingCommanderDemo.rsrc \
    --snapshot out-modern/swc-preview.bmp
```

The known demo contains 23 CMFs, 1,011 chunks, 267 compressed sprite sets,
3,446 frames, and 18,315,140 decoded chunk bytes. Development builds retain ASan
and UBSan diagnostics. The asset viewer's snapshot command remains available
for inspecting decoded artwork.

## Recovered format contracts

These contracts come from the original 68000 instructions and actual demo data.
Ghidra's C output alone is insufficient: classic Mac A-line traps truncate some
decompilations, and `GetFramePtr` returns its pointer in A0.

| Data | Contract | Original evidence |
| --- | --- | --- |
| CMF header | `CMF1`; LE32 TOC offset/length at 12/16 | CODE_09 `CMOpenFile` +0x26ae |
| CMF TOC | LE32 relative type-table offset; type/count groups; 16-byte entries | CODE_09 `CMReformatTOC` +0x2fac |
| CMF entry | ID, file offset, flags/size, old handle; length mask `0x00ffffff`; compressed bit `0x10000000` | CODE_09 `CMGetChunk` +0x22f6 |
| Compressed chunk | BE32 output length, then byte RLE | CODE_09 `CMDecompressRLE` +0x2b26 |
| RLE | `(control & 127) + 1` bytes; high bit repeats the next byte, otherwise literal copy | CODE_09 +0x2ba4..+0x2bfe |
| Sprite set | BE32 count; each frame has a 14-byte header plus payload padded to a multiple of four | CODE_01 `GetFramePtr` +0x05f4 |
| Frame | BE16 encoding, signed x/y, width/height; BE32 payload length | CODE_01 `GetFramePtr` and `DrawRLEImage` |
| Pixels | Encoding 1 is raw; 2 is byte RLE; palette index 0 is transparent | CODE_01 `DrawRLEImage` +0x17aa |
| Palette | Classic resource-fork `clut`, 16-bit RGB channels reduced to their high byte | Demo resource fork, clut/251 |
| Mission chunks | `Data.CMF`, `MOD0..2` IDs 1..6; index `series * 4 + mission` for IDs 1..5, series alone for ID 6 | CODE_03 `LoadMissionData` +0x16d2 |
| Mission records | 24-byte header; 16 × 88-byte nav points; 16 × 68-byte objectives; 32 × 76-byte ships; two 40-byte auxiliary blocks | CODE_03 `LoadMissionData`, CODE_04 record consumers |

The host ignores persisted Mac handles and cache state. Compressed allocations
are limited to 64 MiB and impossible compression ratios are rejected. Frame
payloads borrow the decoded set; owned buffers and CMFs have explicit lifetimes.
The viewer centers extracted ship images for inspection. It does not claim to
reproduce flight projection, sprite hotspots, scaling, or original draw order.

## Carrier rooms and first briefing

The demo's original `GameFlow` (CODE_09 +0x1b8e) calls `RecRoom`, then
`BarracksScreen`, `Briefing` and `scramble`. `StartNewCampaign` enables the
room path. These are playable demo sections, rather than unused full-game
assets. Choosing the simulator instead displays the original demo restriction.

The SDL adapter shares these existing WC1 functions without changing them:

| WC1 code | Use in the SWC carrier |
| --- | --- |
| `ResetCampaignData`, `CorrectPointers` | Fresh campaign and pilot records, including callsign, rankings, medals and badges |
| `TitleMenuRegion` | Shared rectangle storage; SDL follows the Mac's half-open hit testing and overlap priority |
| `ConversationSceneRecord`, `ParseTests` | Original 13-byte scene commands and conditional branches |
| `AddPCName` | Player callsign, name, rank and player/wingman kill substitutions in original dialogue |
| `ParseMouthAnimation`, `ParseFaceAnimation` | Original script strings decoded into animation commands |
| `Build_objective_list` | Shared mission objectives; the SDL adapter supplies the Mac map transforms |

WC1's complete `SceneDirector`, `RecRoom` and `BarracksScreen` call DOS packet,
viewport, font and input routines. The SDL adapter uses their reusable logic
and data types, with separate presentation for SWC's Mac CMFs and palettes.
The ranking sort retains the same kills/missions rule as WC1 and SWC. No
reference-build functions were edited for this feature.

SWC initializes the player as `ARMSTRONG` / `MAVERICK` after each shared
campaign reset. CODE_07 `InitCharacters` +0x13fc copies nine 48-byte pilot
records from expanded DATA/0 +0x28f0; the player record at +0x2a70 contains
those two strings. `CorrectPointers` +0x054a selects that ninth record.
Shotglass's second line in `BRF0/5` section 4 reads `You must be $C`:
the callsign substitution previously exposed WC1's `PELLEY` / `GOBLIN`
defaults instead. The SDL initialization now supplies the Mac identity for
subtitles and pilot displays on startup and subsequent New Game selections.
WC1's templates and shared text substitution remain unchanged.

`Data.CMF` contains `CMP0/3`, whose first pair selects Paladin and Angel, and
`BRF0/5`, the Enyo 1 script. Its ten section offsets are little-endian 32-bit
values; scene records contain three signed bytes followed by five little-endian
16-bit fields. Sections 0/1 hold the first briefing, 2/3 the debriefing; 4/5, 8/9 and 6/7 hold
Shotglass, Paladin and Angel respectively. The host validates section bounds,
NUL termination, animation syntax, branch targets and expanded subtitle size
before calling WC1's original parsers. It supports the initial mission's
commands and rejects unsupported script forms explicitly.

| Original presentation evidence | SDL implementation |
| --- | --- |
| CODE_07 `RecRoom` +0x059a | Original bar artwork, three characters, room hotspots, rankings and simulator message |
| CODE_07 `ShowChalkBoard` +0x111a | Original board artwork and row positions with shared WC1 pilot records |
| CODE_03 `BarracksScreen` +0x5c70 | Original barracks background and briefing/bar/medal/bunk hotspots |
| CODE_03 `DrawMedals` +0x5a9c | Original badge/medal frames, offsets and shared campaign awards |
| CODE_03 `SceneDirector` +0x254c | First briefing and bar records, including Angel's interjection in Paladin's conversation |
| CODE_03 `DrawBriefingLongShot` +0x1e54, `DrawPodiumShot` +0x1e02 | Original background/body frames and placement |
| CODE_03 `LoadFace` +0x42cc, `CloseTalk` +0x323c | Original portrait/background selection and mouth/eye overlay offsets |

Expanded DATA/0 supplies the room palettes and filenames at +0x3fd2/+0x4046,
ranking rows at +0x2690, idle programs at +0x27ae/+0x2816/+0x27da, medal
frames at +0x42f2 and briefing origins at +0x4306/+0x430a. The GUI now checks
the carrier/debriefing/funeral CMFs as well as the flight CMFs. Speech and movies remain
optional; missing carrier artwork is an error.

Speech comes from `AIFF/camp.0/04/04SSLL00.AIF`, where `SS` is the script
section and `LL` the original record index, including branch-skipped records.
The format string is used by CODE_03 `SceneDirector` +0x2cb6; the archive adds
the `04` directory. All 92 demo recordings inspected use AIFF with one channel,
16-bit big-endian PCM at 11025 Hz. The reader validates FORM/COMM/SSND sizes,
chunk padding, data offsets and declared sample counts. SDL mixes the PCM with music;
missing/unreadable files or an unavailable device leave subtitles usable.
Malformed present speech is an error. No external audio codec is required.

The original demo files all declare a FORM size four bytes shorter than the
actual container, apparently omitting the `AIFF` type from the count. Their
final SSND chunks reach physical EOF and contain exactly the declared PCM
samples. The reader permits this specific four-byte SSND overrun only when
that chunk ends at physical EOF. Other chunk overruns and truncated samples
remain errors; ordinary AIFF FORM bounds are unchanged. This fixes rejection
of Angel's `04060000.AIF` and the other original recordings.

Scope remains a fresh Enyo 1 visit using WC1's initial pilot template. Save/load,
name entry, later mission rooms and post-debriefing progression are not connected.
Carrier presentation now includes the following recovered behavior:

- The bar's **Barracks and Pilot Ranking hotspots were reversed**. CODE_07
  +0x060e..+0x0638 and the strings at A5-0x2b7d/-0x2b6e establish Barracks
  at `(250,65)-(318,127)` and rankings at `(268,26)-(320,65)`. Hit testing
  follows Mac `PtInRect`: right/bottom edges are excluded, and character
  regions take priority. Paladin and Angel retain their initial sprite bounds,
  as in `GetScreenRect2` +0x1420 and `RecRoom` +0x0b54.
- Hovering characters selects DATA/0 +0x2782/+0x2788/+0x278a frames 3/4/5.
  The Barracks door uses RCRM/1 frame 1; the ranking highlight uses RCRM/12
  at `(245,15)`. The simulator opens/closes RCRM/11 at `(0,85)` through
  frames 0..6. Previously its position incorrectly held the ranking highlight.
  Barracks door/medal hover sequences use RCRM/13 frames 1..8, 9..16 and
  17..21; the wall display and desk light are also drawn.
- Bar idle loops, the barracks wall display, simulator and door/medal hover
  sequences share a 15 Hz room-animation clock, reduced from 30 Hz. Input
  polling remains independent. CODE_07 `RecRoom` +0x0c98..+0x0d38 counts
  animation delays in room updates; CODE_03 `BarracksScreen` +0x606e..+0x6140
  advances hover frames every second update and +0x62d6 chooses the wall frame
  once per draw. Their frame/delay commands and relative pacing are preserved.
  The 15 Hz clock is an SDL presentation choice; these Mac loops do not specify
  a fixed wall-clock rate. Pausing, focus loss, fades and conversations freeze
  the room clock. Speech, portrait scripts, movies and flight retain their
  separate timing.
- Carrier mouths now consume AIFF `MARK` phonemes through the same
  CODE_12 `ParseMarks`/`GetMouthPos` reader used by the intro. Timing comes
  from the audio mixer's consumed sample position, with a one-buffer latency
  allowance; pause/focus loss freezes both audio and animation. Without a
  device, elapsed speech time supplies the clock. Recordings without markers
  keep the mouth closed; missing recordings retain script animation/subtitles.
  `SceneDirector` +0x2e34 chooses `CloseLook` when the displayed portrait
  is listening, so another character's voice no longer moves its mouth.
  Eyes retain their separate scripts, with `GetEyePos`'s minimum 24-update
  delay, on a 30 Hz host presentation clock.
- `PanToScreen` +0x2222 is the commander's turn toward the display, followed
  by the map reveal. BRFG/4 frames 11..16 run at eight Mac ticks per frame;
  CODE_14 `BriefingMap_ShowGrid` +0x0cfe reveals BRFG/27 two pixels per edge
  per update at its DATA/0 origin `(40,20)`. Both maps now share the native
  `BuildMap`, `SetScale`, `ScaleX`/`ScaleY` and `FindTextPos` adaptation.
  Briefing uses DATA/0 +0x272's `(125,124,40,20,54,45)` layout and BRFG/28
  masks, hazard/objective colors, full labels and a thirty-update objective
  blink. The initial reveal draws markers without labels, as `DisplayMap`
  +0x0f88 does; later records use `BriefingMap_UpdateMap` +0x10c4.
- `Dismissed` +0x1eba uses BRFG/29's reverse angle. After speech, BRFG/31
  frames 11..13 turn the commander and BRFG/32 frames 0..4 show the pilots
  standing. BRFG/30 at `(85,5)` and /33 at `(5,0)` then walk out with the
  original independent one-in-five start gates and terminal frames 10/9.
  Pause/focus loss freeze the thirty-update host clock; release input skips.
  Its separate `BRIEFING.MooV` cue is absent from the supplied demo, so the
  existing `BRIEFIN0.MooV` briefing cue continues.
- Bunk occupants represent **saved pilot slots**, not the current wingman
  roster. A fresh original preferences file clears all eight occupancy bytes.
  If a `SuperWing Preferences` data fork is copied into the game-data root,
  SDL reads only those bytes at `10 + slot*0x458` from the original 0x2372-byte
  file. This does not import campaign state or enable save/load. Occupied
  RCRM/15..22 bunks follow the original draw order and DATA/0 +0x40c2/+0x40e2
  frame/delay/jump programs. RCRM/45 supplies hover lamps, including empty
  slots. The desk has its falling palette-8 pixel, six-update flash frames
  and sound 35. These animations use the existing 15 Hz room clock.
- The incidental bar animation is the original **`@` Easter egg**:
  `SetGInputForChar` enables `RecRoom` +0x0e5c's RCRM/49 sprite. Its thirteen
  frames move from x=-75 to x=320 at y=40 behind the room/window artwork.
  It is not a random idle event.
- `print_subtitle` +0x1122 uses **white, centered Helvetica 14**. The record's
  `textColour` also identifies the speaker; it does not directly select the
  Mac subtitle ink. Wrapping uses measured advances and a 275-pixel width,
  with 15-pixel line spacing and the original text-layer position. Long
  expanded captions are kept inside the host picture. An optional classic
  Helvetica resource supplies the original bitmap strikes on every host;
  without it, macOS uses system Helvetica through CoreText and other hosts
  use the bundled bitmap fallback. See the font setup below.
- CODE_15 `dark` +0x03c2 loads the current palette's **clut ID + 10**;
  these resources dim the picture rather than making it black. Room and
  conversation transitions use those actual palette colors. SDL interpolates
  between the normal and dim pictures over ten Mac ticks (167 ms); that
  interpolation is a host presentation choice, while the Mac routines switch
  palettes directly. Captured render targets keep resize, pause and skip safe.

Remaining presentation limits are the external classic-font asset, fixed host
frame pacing and interactive visual verification. Save/load, name entry and
later campaign scenes remain separate work. The optional font reader has not
been checked with an actual classic Helvetica resource.

Mission state is prepared before the carrier for the briefing map; simulation
starts only after launch. Actions complete on release, focus loss pauses
speech and scene time, and textures/audio are released on every exit. Runtime
check automation has been removed. Validation for this change
was original-instruction/data inspection, static review and compilation with
`make modern modern-gui modern-swc`, plus compiler static analysis; no tests,
interactive code, game, GUI or audio playback were run. Visual behavior and
audio timing still need user review.

## Carrier return and debriefing

The SDL flight host reuses WC1's communication recipient/command menus and
`request`, including wingman orders and landing command 12. C and number keys
follow DOS/Win32 controls; SWC's native CODE_06 `Chosen_communicate_option`
instead routes four quick commands. SDL draws the menu and clearance feedback;
native order/response voices and animated WGMN portraits are now connected.

CODE_06 `cleanup_objectives` +0x0738, `can_land` +0x0ab4 and `request` +0x0c0e
match the shared Enyo 1 rules. Clearance needs no enemy within 20,000 and either
patrol progress, a kill, health below 50, or fuel below 1,000. It does not require
completing every objective. CODE_02 `house_keep_objects` +0x414a then checks
range below 700 with collisions enabled, clearance and normal-speed flight.
The SDL SWC branch omits WC1's facing checks. Flight stops before another
collision pass once landing is triggered.

CODE_03 `landing` +0x447e plays `LANDING1.00`, `LANDING2.00`, `LANDING1`,
`LANDING4.00`, `LANDING5.00`, `LANDING6.00`, then `LANDING7.NN`, with
`NN = 2 - evaluate_damage(0)*2/100`. The host uses this order and health variant.
Missing movies leave debriefing available; malformed present movies fail.

WC1 `PostMission` shares CODE_09 +0x1672's player/wingman statistics and
first-mission badge fallthrough. Random awards to other pilots differ, as
documented in the status review above. `FullMissionScore` +0x17e2 and
`PlayersMissionScore` +0x1840 use 16 signed weights from `CMP0/2` at
`(series-1)*128 + mission*24 + 40`. Enyo 1's weights are `2,1,2,1` followed by
zeros. A temporary WC1 campaign score window supplies those weights to its
unchanged score functions and `ParseTests`; the previous pointer is restored
on every exit. This does not load or advance the full campaign.

`BRF0/5` section 2 has 32 records. The bounded reader now accepts its kill,
wingman-status and full/partial score branches plus `$C`, `$K` and `$L`.
Original `DeBriefing.CMF` BRFG/11..14 and `DeBriefingHeads.CMF` BRFG/15 and
TKHD portraits use palettes 190/191. Body origins follow `DrawDebriefingLongShot`
+0x1b8c; close-up backgrounds follow `SceneDirector` +0x2960/+0x2988.
`CloseTalk` +0x3292 selects portrait base frame 21 for debriefing, unlike the
frame 0 base used in other conversations.
Early returns and Spirit's loss select the original dialogue instead of a
fixed success outcome. Spirit's funeral follows if she died on this mission.
Dismissal returns to the title menu after any funeral. Office/award ceremonies
are not reached by the original demo's Enyo 1 path; see the postflight audit
below. Save persistence and later campaign progression remain unimplemented.

Validation: original Ghidra instructions and offline demo-data inspection,
static review, and `make -j4 modern modern-gui`. No tests, interactive code,
game/GUI sessions or audio playback were run. Existing packed-pointer linker
warnings remain; the launcher fixture only gained the required CMF filenames.

## Demo postflight applicability

The original demo returns to its title after one flight. CODE_09 `squadron`
+0x003a..+0x0058 selects Enyo 1 and the Hornet; after `GameFlow` returns zero
at +0x219c, `squadron` +0x0090..+0x00aa returns to `Title_Sequence`.

CODE_09 `GameFlow` +0x2002..+0x200a branches directly to the wingman-funeral
check when the post-series sequence is -1, bypassing every office/award call.
`UpdateSeries` +0x1922..+0x1942 only selects a sequence when the incremented
mission index reaches the series mission count. `Data.CMF` `CMP0/2` describes
Enyo as two missions (BE32 at +4); finishing mission index 0 leaves the sequence
at -1. The office-pending and medal-pending flags alone cannot open those scenes.
The three `Office` calls are at +0x2066, +0x214c and +0x2170; the sole
`MedalSequence` call at +0x2048 additionally requires sequence 64. Enyo 1's
mission medal record is type 0 with threshold 2000 (BE32 at +32/+36), but
queuing a medal still does not bypass the series gate.

The supplied data retains scripts and palettes for these later scenes, while
omitting their presentation assets. CODE_03 `Office` +0x147c and
`MedalSequence` +0x3584 identify the script chunks. `GameMode` +0x6e3c uses
the filename/palette tables at DATA/0 +0x4046/+0x3fd2:

| Scene | Present in the demo | Missing from the demo |
| --- | --- | --- |
| Office | `Data.CMF` `BRF0/2` (4454 bytes), palettes 192/202 | `Office.CMF`, `COMMANDE.MooV` |
| Medal ceremony | `Data.CMF` `BRF0/3` (1760 bytes), palettes 194..196/204..206 | `MedalLongShot.CMF`, `MedalCommander.CMF`, `MedalPinning.CMF`, `MEDALCE0.MooV`, `MEDALCE1.MooV`, `MEDALCER.MooV` |

Office uses GameMode 5; medal establishing, commander and pinning shots use
modes 10, 25 and 26. `ViewMedals.CMF` supplies the existing barracks medal-case
screen, not ceremony artwork. These absent packs must not become required
demo assets or trigger fabricated postflight scenes.

One reachable flow differed: `GameFlow` +0x1f6a..+0x1f70 bypasses promotion
and `DeBriefing` after rescued ejection. The SDL host now runs `PostMission`,
skips landing/debriefing, then plays Spirit's funeral if she was lost and
returns to the title. Ordinary landings retain debriefing and the same funeral
check; death and stranding keep their existing endings. Mission setup also
clears all six pending postflight flags from +0x1ba6..+0x1bc6, so a queued
first-ejection medal cannot leak into another New Game. These flags are outside
the shared `ResetCampaignData` template.

Validation: fresh Ghidra CODE_03/09 exports, A5 jump-table/string resolution,
offline inventory of all 23 demo CMFs and the original resource fork, native
and GUI builds, and Clang static analysis. No tests, game or GUI were run.
Full `UpdateSeries`, office/ceremony presentation and campaign persistence
remain future full-game work; they are not additional scenes missing from
the demo's Enyo 1 ending.

## Funeral scenes

CODE_09 `GameFlow` +0x1b8e routes player death to `funeral_sequence(1)`
(call at +0x1e44), without landing or `PostMission`. Its +0x218c check starts
`funeral_sequence(0)` after any debriefing when the wingman was lost. The SDL host
uses those outcomes and WC1's existing pilot-death state, retaining the flown
mission for the original `ParseTests` command 30 / `wing_status` branches.

CODE_03 `funeral_sequence` +0x4a08 interleaves `FUNSERV0`, `FUNSERVA`,
the eulogy, `FUNSERVB`, then the farewell. The SDL adapter uses this order and
the existing movie player. Each movie group is checked for readable files
before playback; a missing group leaves the dialogue available. Malformed
present movies and speech report errors. Audio ownership passes between the
movie player and dialogue player, and all paths handle skip, quit and cleanup.

`Data.CMF` `BRF0/1` has fourteen LE32 directory entries and the same 13-byte
records as the briefing. For fresh Enyo 1, `funeral_player` +0x46ec selects
section 2 and `farwell_player` +0x47cc selects section 0; `funeral_wingman`
+0x4888 and `farwell_wingman` +0x494a select sections 12 and 10. The latter
scripts branch to Spirit using shared WC1 state. The bounded reader also
permits `$N` and `$R` before WC1's `AddPCName`; score tests 35/36 remain limited
to debriefing, where the temporary campaign score window is available.

`Funeral.CMF` uses palette 193: BRFG/19 is the long shot, BRFG/20 supplies two
close-up backgrounds selected by text color, and TKHD/1 and TKHD/10 supply the
portraits. This follows `DrawFuneralLongShot` +0x1af8, `MountGraphics` +0x2484
and `SceneDirector` +0x29b0. Speech uses `AIFF/camp.0/00/00SSLL00.AIF`; the
demo includes eleven recordings for these player/Spirit scenes. The fresh
pilot's rank is zero, matching the supplied `$R` speech variant.

The host presents the original moving `PLNT/1` background and Helvetica 14
THE END after the player's farewell, retaining `the_end` +0x4b1e's input wait.
Funeral music continues through this screen. Skipping the funeral still bypasses
THE END, as the original skip-latch branch does. Movie skip dimming is connected;
exact scene timing remains unverified. The GUI requires `Funeral.CMF`.

## Startup, opening, launch, landing and funeral movies

The Mac demo stores 38 custom `LMov` files under `Movies/`, with `.dcMov`
extensions. These are not ordinary QuickTime movies. The new decoder follows
CODE_12 `LMovieOpen`, `LMovieTask`, and `LMovieDrawFrameMinRect`; the SDL host
replaces QuickDraw blits, Sound Manager double buffers, and Mac event handling.
WC1's `scramble` constructs its launch from sprites, so it cannot play these
files. Shared WC1 mission state is prepared before the carrier visit; flight
simulation starts after playback.

Startup follows CODE_12 `SetupSWStuff` +0x4ad2/+0x4ad6: `ShowSplash`
(+0x4bb6) displays resource `PICT/129` with `INTRO.MooV`, for at most 1500
Mac ticks (25 seconds), then `MacIntroFilm` (+0x4cdc) plays `Lion` without
music. The splash reader supports the demo's single indexed 320x240
PackBits rectangle, with its own 256-color table; it is not a general PICT
interpreter. No original pictures or recordings are copied into the source tree.

CODE_09 `squadron` +0x5e then calls CODE_07 `IntroFilm` +0x1442:

| Sequence | Music / original level |
| --- | --- |
| `INTRO00`, `INTRO01` | `SUPRWNG1`, 50 |
| `INTRO02` | Continue `SUPRWNG1`, 30 |
| `INTRO03`, `INTRO04` | `SUPRWNG2`, 30 |
| `intro_ready_attack` (+0x1906), speech `intro.03` through `intro.09` | `SUPRWNG3`, 30 |
| `INTRO05`, `INTRO06`, `INTRO07` | `SUPRWNG4`, 50 |
| `intro_ranger_dead` (+0x1adc), speech `intro.10` and `intro.11` | Continue `SUPRWNG4`, 30 |
| `INTRO08`, `INTRO09`, `INTRO10` | Continue `SUPRWNG4`, 50 |

The live dialogue scenes use `IntroFilm1.CMF` / `IntroFilm2.CMF`, palettes
130/131, and `TKHD/13` at (0,10). The first includes two mirrored `ST01/1`
wingmen. Both show the original `PLNT/1` stars, following the stationary
180-degree camera case of CODE_05 `init_3d_shards` / `update_3d_shards`.
Eye frames change every ten host draws. CODE_12 `ParseMarks` (+0x00de) and
`GetMouthPos` (+0x036a), plus the phoneme table at expanded DATA/0 +0x0908,
supply mouth frames from the AIFF `MARK` words: five 60 Hz ticks per phoneme,
including `th`/`ch`, and a closed-mouth event after each word. Recordings
without markers retain the base mouth. The intro and carrier now share the
audio mixer's playback clock for mouth timing. Speech duration remains available
if the audio device is unavailable.

Space, Enter, Escape, Backspace or left click dismiss the splash and logo separately;
during the prologue, they skip the remaining movies and dialogue. This follows
the movie skip latch and CODE_15 `movie_dim_screen` (+0x046c), which clears it
after `Lion`. Input completes on release. P and focus loss pause video,
dialogue animation and the shared audio device. Missing startup movie/dialogue
files skip the group; malformed present assets report an error. Quit ends the
session, and music, PCM, textures and temporary buffers are released on exit.

The host continues into `Title_Sequence` (+0x1c86). Idle replay calls IntroFilm
without repeating the splash or Lion. Saved-game selection remains disabled
because the host has no save loader. Movie palette dimming is connected; exact
Mac draw/RNG timing remains unverified. Live animation uses a 20 Hz host cadence
and shared WC1 RNG.
`intro_shakecockpit` is empty in this demo. `BUC` belongs to CODE_14
`stranded_sequence` +0x0a06 (MovieDisplay call +0x0a40), not startup.

### Title battle, death and stranded endings

`SwcRunFlightScene` presents CODE_07 `Title_Sequence` +0x1c86 using
`Spaceflight.CMF` `RCRM/47`, flight palette 251 and `VICTORY.MooV`. The background
uses the four original DATA/0 mission records 32..35: two Dralthi and two Hornets
at nav 16. The shared simulation supplies combat, damage and debris; an eliminated
side respawns, and fighters beyond 100,000 units explode. The selected New Game
overlay appears on four of every six rendered updates. After 10,001 updates
(about 8 minutes 20 seconds at 20 Hz), the intro replays and the menu returns.
This is the original frame counter, which does not reset on mouse movement.
The title menu releases mouse capture and shows the system arrow on every visit,
including returns from flight and intro replay. Mouse clicks use SDL's scaled
logical coordinates, matching the original menu artwork after resizing.

The Mac-only camera mode 16 is recovered from CODE_02 `new_view` +0x3190 and
`set_eye_direction_and_position` +0x25d8, with CODE_04
`try2point_object_at_object` +0x319e. It turns gradually toward its subject,
follows beyond 5,000 units and selects another ship if the subject disappears.
Its Q8 acceleration uses the Mac shrink limit of 0xf00. The Win32 camera switch
is unchanged. Title combat is discarded before starting a campaign; cockpit,
pilot, mission, input and audio state are reset on each visit.

CODE_14 `death_sequence` +0x362a uses that camera on the player, plays
`FUNERAL.MooV`, advances 40 updates, then calls `Explosion(0)`. Simulation
continues behind the centered, size-10 `Press <sp> to continue` prompt at baseline
200, palette index 129. Dismissal enters the player funeral without PostMission.

CODE_14 `ejection_sequence` +0x07c4 creates object type 56 with a 32,000-update
lifetime, copies the player's frame and position, and adds `up * -0x500` to
its inherited velocity. Camera 16 follows the pilot while `EJECT-IM.MooV`
plays. After 40 simulation updates the abandoned ship explodes; the prompt is
left-aligned at (10,200). An unopposed surviving wingman receives the come-home
order. Escape/Backspace skips the scene on release, still recording the lost ship.
Space/Enter or a click continues after the explosion. P pauses the scene.
Pilot allocation avoids WC1's reserved starfield slots 34..48; if the effects
pool is full, one transient explosion/debris/projectile can be reclaimed.

Ctrl+E maps to CODE_01 `process_player_input` +0x3cbe's eject action. Ejector
damage 4 disables it; lower damage succeeds when the original exclusive-upper-bound
`Random3DO(0, damage)` returns zero, otherwise sound 25 plays. SDL checks this
directly without changing WC1 input or its Win32 executable. Once ejected, damage
to the abandoned ship cannot redirect the host into the player's funeral.

The original requests `CKPT/17`, but it is absent from all 23 supplied CMFs and
the executable resource fork. The host keeps the pilot's motion and camera without
drawing substitute art, and reports the omission once. It loads the thirteen-frame
set if supplied in either flight CMF; malformed present art remains an error.
Thus the ejection behavior is implemented, but the visible pilot still needs
the missing asset. No game data is added to this repository.

CODE_09 `GameFlow` +0x1e62..+0x1ef2 checks for stranding after ejection. A rescued
pilot bypasses the landing movie, records the ejection, loses one promotion point
(minimum zero), increments the eject count and queues medal 3 for the first eject.
`PostMission` runs, but +0x1f6a..+0x1f70 skips debriefing. The series gate at
+0x2002 also bypasses office/medal ceremonies for Enyo 1, even with that pending
medal. Any lost-wingman funeral then precedes the return to the title menu.

CODE_13 `check_stranded` +0x1630 selects arcade state 3 when the carrier record
has state 3 and no enemy is within 30,000 units. This now enters CODE_14's BUC
movie with `LASTRITE.MooV`, then `AIFF/Stranded.aif`. Music ducks to 20 during
narration and returns to 50 afterward. The original two captions use baselines
102/134; THE END replaces them after 127 updates.

`SdlShowSwcEnding` adapts CODE_05 `init_3d_shards` +0x4480,
`generate_new_3d_shard` +0x4362 and `update_3d_shards` +0x4598. Both endings
use 63 particles from `RecRoomScene.CMF` `PLNT/1`, palette 135, the original
15-entry frame table, Q8 yaw/projection and retirement/respawn conditions.
Stranded draws text before the particles; the funeral's THE END draws after them
at baseline 110. Both use Helvetica 14 and palette index 4. Input is consumed on
release; Escape or Backspace skips, and P/focus loss pause animation and audio.
Completed endings return to the menu; window close exits throughout.

Validation: fresh Ghidra instruction exports, original DATA/CMF/clut inspection,
`make -j4 modern modern-gui modern-swc` and Clang static analysis. No tests,
game/GUI execution, playback or audio device were run. Original artwork was
extracted for inspection only. Shared AI/RNG differences, exact classic font
pixels and variable Mac frame pacing remain unverified; movie palette dimming
is described below.

CODE_03 `opening_scene` +0x4374 plays `OPEN0`, `OPEN1`, `OPEN2`, then `OPEN3`.
The names resolve from A5-0x347c, -0x3468, -0x3462 and -0x345c respectively;
A5-0x3475 names the separate `DOCKING.MooV` music. CODE_09 `squadron` +0x84
calls this sequence before `StartNewCampaign(0)` at +0x8a and `GameFlow` at
+0x90. Both SDL launchers now play it before the initial carrier visit, using
the existing movie decoder and sequence player. A missing opening file skips
the group and preserves carrier entry; malformed present data reports an
error. Skipping also enters the carrier, while window close ends the session.
This four-clip group follows the separate startup sequence above.

CODE_03 `scramble` +0x45d0 and its A5-relative strings specify this order:
`ARMOR`, `HALL`, `LAUNCH01`, `LAUNCH02`, `LAUNCH03`, `LAUNCH04.00`,
`LAUNCH06.00`, `LAUNCH07.00`, `LAUNCH08.00`. The `.00` suffix selects the
Hornet. The host checks that all nine files can be opened before playback;
missing/unreadable movies produce a stderr message and leave the existing
direct-flight path available. Invalid movie data reports an error.

| Movie data | Recovered contract |
| --- | --- |
| Header | Big endian; `LMov`, version `0x00010000`; 184 fixed bytes followed by 16 bytes per indexed record |
| Layout | Metadata size at +0x08; record count at +0x0c; integer frame rate at +0x10; audio-present bit at +0x14; height/width at +0x18/+0x1a; initial pixel value at +0x1c |
| Index | Flags, persisted cache pointer, payload length, absolute file offset; flags 2 = video, 1/0x11 = audio |
| Palette | Length at +0xac; 2056-byte Mac color table immediately after the index; 256 colors in table order, ignoring `ColorSpec.value` |
| Audio | Block bytes at +0x9c; 16.16 sample rate at +0xa0; sample bits at +0xa4; valid sample count at +0xa6; audio flags at +0xaa |
| Demo audio | Unsigned 8-bit mono, flags 2, 11128-byte stored blocks, 22254.545... Hz; the sample count trims the last block and excludes padded audio after it |
| Frame commands | Upper three bits select end-row, short/long skip, short/long literal, short/long repeat, or no-op |
| Run lengths | Short: `(command & 31) + 1`; long: `((command & 31) << 8) + nextByte + 1`; skips retain preceding pixels |

The host accepts the demo's 320x240, 256-color layout, checks record and scanline
bounds, and limits loaded movies to 64 MiB. Every video delta is decoded in
order even when presentation must catch up. SDL2 displays each movie's own
palette, mixes its embedded PCM with sequence music, and rounds the original sample rate to an
integer Hz. Playback falls back to silent video if no audio device is available.
The video clock and audio device pause together, and the last frame remains
while longer audio finishes. All movie resources are released on finish, skip,
error, or window close.

The separate `MIDI/DOCKING.MooV`, `ARMOR.MooV`, `LANDING.MooV` and
`FUNERAL.MooV` soundtracks accompany their groups. First briefing, bar,
debriefing and funeral scenes use the carrier adapter described above.
No external video library is needed for this slice.

CODE_15 `movie_dim_screen` +0x046c halves all 256 palette entries immediately.
The SDL player applies that transition after Lion, the final prologue clip,
opening, launch, landing and stranded BUC. The funeral calls it after room/end
presentation, so its intermediate clips retain their palette. Skipping follows
CODE_12 `MovieDisplay` +0x0738 instead: entries 1..254 are halved and endpoints
0/255 are preserved. There is no timed fade in either native routine. SDL
re-presents the indexed frame with the dimmed palette before releasing it.
The separate skip-time zero-index pixel rewrite at +0x07d0 is not reproduced.

Opening integration was checked against fresh Ghidra instructions for its
caller, the original four MovieDisplay calls and expanded A5 strings. Validation
was static source review and `make modern modern-gui`; no tests, game, GUI,
playback or audio device were run for this change.

Startup validation used fresh Ghidra CODE_07/04/05/15 exports, the original
CODE_01/09/12 callers, expanded A5 strings/tables and original asset metadata.
`make -j4 modern modern-gui modern-swc` passes. No tests, game, GUI or audio
playback were run. All changes are confined to SDL sources and headers; Win32
reconstruction and source selection are unchanged. Interactive appearance and
synchronization remain unverified.

Validation for this pass consists of original-instruction review, offline
inspection of all 38 original containers and 3,521 encoded frame streams,
static source review, and `make modern modern-gui`. This is not a playback
test: no game, GUI, interactive check, or audio device was run. Playback and
audio synchronization still need user review.

## WC1 reuse and original-code evidence

The implementation uses WC1 functions wherever their behavior is suitable.
SWC adapters decode resource formats and translate record layouts at the input
boundary. They do not introduce separate mission structures or duplicate the
shared gameplay functions. Multi-byte mission fields are read explicitly as
big endian, and values that do not fit the WC1 fields or supported tables are
rejected before publishing any state. Canned-sequence mission records are also
rejected because a persisted Mac pointer cannot be used as a host pointer.

| Existing WC1 code | Current SWC use | Remaining qualification |
| --- | --- | --- |
| `LoadMissionData` in `src/cmpgn.c` | Unchanged WC1 path; SWC calls its SDL mission adapter directly | The launcher selects the game; later SWC campaign scenes are not connected |
| `set_sphere_point` in `src/brains.c` | Adds the existing nav position and ship-relative position | Validated with synthetic data and Enyo 1 |
| `Set_up_ship_info` / `init_intelligence_data` | Sets the Enyo player's position, orientation, speed, pilot, and mission state | Other mission modes and SWC AI differences need review |
| `Build_objective_list` | Builds Enyo's three nav objectives and carrier-return objective | Other SWC objective behavior is not established |
| `cycle_next_objective` | Cycles shared objectives | Briefing and in-flight maps both use Mac scaling/layout in `src/sdl/swc_nav.c`; WC1 `nav_getxy`/`SetScale` are not used by these maps |
| `PollKeyboardState`, `process_player_input` | WC1 keyboard priorities, diagonals, gradual steering, reversals, and Shift response | Deliberately uses DOS/Win32 controls; SWC's Mac event/device layer differs |
| `player_input`, `QueueInputEvent`, `WarpMouseTo` | Original mouse response, edge limits, right-button roll/throttle, and recentering | SWC supplies only motion events and the right-button modifier; weapon events are excluded |
| `accelerate`, `celerate`, `your_afterburner`, `fire_afterburner` | Throttle bounds, boost activation, and boost timers | SDL supplies the native afterburner loop and resource-fork release tail |
| `players_flight_dynamics`, `rotate_object`, `accelerate_and_move_object` | Steers and moves ships and active weapons using SWC parameters | Fixed WC1 simulation step; native elapsed-tick scaling remains pending |
| `fire_players_lasers`, `fire_fixed_projectile_weapon`, `fire_weapon`, `fire_missile` | Energy, cooldowns, projectile initialization and missile release | Recovered SWC definitions and all 63 weapon hardpoints; SDL-only aiming offset and laser cooldown |
| `house_keep_objects`, `update_objects_in_space`, `object_intelligence` | Lifetimes, collisions, damage, ship AI, missile guidance, recharge and animations | SWC dispatch and common setup verified; complete AI/probability equivalence is not established |
| `ReleaseStaleNavTarget`, `set_up_action_sphere`, `init_ship`, `check_next_wave` | Enter nav spheres and spawn their ships/hazards/waves | CMF textures replace WC1 packet loading; later mission scripts remain unsupported |
| `auto_pilot_sequence`, `check_hazards`, `extra_hazard`, `update_hazards` | Formation travel, encounter stops and hazard lifecycle | Original SWC boundary rollback and SDL presentation of the 120-frame external view; the Mac starfield adapter reuses hazard maintenance |
| `select_new_gun`, `select_new_release_weapon`, `build_your_target_list`, `sort_viable_target_list`, `cycle_onscreen_targets` | Shared weapon selection and distance-ordered visible target list | Target-list and cycling behavior match the Mac; SDL supplies automatic acquisition and native missile locking instead of WC1 `check_target` / `target_locking` |
| `draw_nav_pointer`, `auto_pilot_valid`, `missile_on_tail`, `calculate_damage_level` | Shared objective projection and warning queries | SWC navigation activation and stale-marker reset; autopilot dispatches to the recovered SWC eligibility and escort-wait state |
| `malf`, `your_internal_damage`, `place_damage_on_cockpit` | Shared component damage and impact flow | SDL-only SWC malfunction countdown and three physical damage regions; original Mac static, damage and spark artwork |
| `SetSpaceFlightFrameTiming` | Initializes the shared flight tick interval, 50 ms at WC1's default 20 Hz | Replaces the host's hard-coded 62 ms; SWC's elapsed-tick translation scaling is not reproduced |
| `start_dust`, `get_right_shape` | Dust positioning and selection among 37 decoded ship views | Mac star generation/respawn and star/dust drawing live in SDL; shared RNG and complete visual parity remain qualified |

The SDL version of `nav_getxy` writes coordinates through byte copies because
the existing packed objective records can place them at odd addresses. Its
calculations are unchanged; the Win32 reference implementation is preserved.
During SWC flight, `get_right_shape` returns the selected frame to the SDL
texture cache instead of fetching WC1 capital-ship packets.

The keyboard adapter passes one SDL state sample to the original Win32 key
polling interface for each simulation tick. Held keypad keys use the same
bindings as the existing SDL key events. It clears released axes independently
and samples throttle and boost independently of steering, without relying on
operating system key repeat. The mouse adapter initializes WC1's input viewport
to a 320x200 rectangle centered at SWC's scene origin (160,100), then queues one
motion sample per simulation tick and calls `player_input` directly. This uses
the existing response tables, direction signs, right-button behavior, and
recentering code without duplicating the mouse algorithm. SDL converts window
coordinates and pointer warps through its logical renderer, including resize
and letterboxing; the extra WC1 cockpitless camera offsets are not applied.
Active flight and autopilot explicitly hide the system cursor. Pause, focus loss
and minimization release capture and show it; resuming flight hides it again.
Carrier menus manage their own visible cursor, and session cleanup restores the
previous visibility. This SDL state is separate from WC1's `bMouseCursorVisible`
steering flag.

The SWC event loop owns event pumping. Its calls into `player_input` cannot
consume unfiltered SDL button or window events through WC1's separate loop.
Primary/secondary host button states stay clear, and no weapon button events
are queued. Right-button state is passed only as a motion modifier; two presses
within WC1's 20/60-second interval activate the shared `your_afterburner` routine
while the button is held. This timing-only check tolerates WC1's pointer
recentering between clicks. SDL handles weapon buttons separately and calls the
shared firing routines. Short left clicks and controller gun presses remain
pending until a simulation tick. The joystick adapter samples normalized base
axes and calls the existing enhanced-axis and rumble code once per flight tick.
Discrete controller buttons are translated directly into events for the SWC
loop, including scene navigation, without feeding WC1's separate event queue.
`HandleSpaceFlightControls` is still bypassed because its remaining HUD
and mission commands are not connected. SDL builds zero-initialize the legacy
input event before polling, since motion and empty polls leave some fields
unset. In SDL builds only, `accelerate` multiplies signed throttle steps by 256
instead of left-shifting negative values. The reference build retains the
original expressions and event declaration.

### Escape and quitting in the original Mac demo

Ghidra's original-instruction exports distinguish cancellation from quitting.
CODE_01 `SetGInputForChar` +0x3ec4 routes ASCII Escape (`0x1b`) to +0x43b2,
which sets the input byte at unrelocated address `0xa7c6`; Space sets that same
byte as well as the gun flag. CODE_01 `process_player_input` +0x3cba does not
consume the Escape byte. CODE_07 `Title_Sequence` +0x1c86 and `RecRoom` +0x059a
also have no Escape-to-exit branch. CODE_14 `visit_the_cinema` +0x000c ends on
its frame count or game state, not Escape. Escape therefore does not quit or
return to the title from normal flight, autopilot, the title screen, or the bar.

Other screens do consume this flag: CODE_03 `CheckEscaped` +0x4336 latches a
cinematic skip, and the rankings/medal screens wait for it or the activation
flag. Quitting instead uses a separate flag at `0xbeec`. Original `MENU/201`
item 10 is **Quit**, with the Q command key. The quit path calls `YesNoAlert`
(CODE_03 +0x72f4) with `0x800003`, selecting `STR#/128` item 3:
“Are you sure you want to quit Super Wing Commander?” `DITL/128` supplies OK
and Cancel, and CODE_01 `HAPPYDIALOGFILTER` +0x225a maps Escape to Cancel
(item 2 at +0x231a).

The SDL host maps Escape to P's pause action during gameplay and on the title
menu. During movies, dialogue, death and ending scenes, Escape skips on release;
in the autopilot camera it skips the presentation while travel still completes.
Escape also goes back from carrier panels, and leaves the bar open. Backspace
retains the same back/skip actions, as does controller B/Back outside flight.
P pauses all these presentations. C or communications
D-pad Left closes the communications menu. Window close and the host quit
shortcut remain separate exit actions.

The pause message uses the Mac `FlyingLoop` text layout at CODE_09
+0x0ad2..+0x0b60: Helvetica 10, palette color 0x81, x=10 and baselines
180/200/220. That block redraws the native resume instructions after cancelling
a quit dialog while paused. A5-0x2845/-0x2822/-0x2800 contain the original
Cmd-P / Actions-menu instructions. SDL substitutes its actual resume keys,
adds WC1's `GAME PAUSED` heading (`ShowGamePausedBanner`, 0x4290a0), and draws
an opaque panel over the frozen scene for readability. Title and camera scenes
continue to redraw while paused without advancing their simulation or timers,
so resizing/fullscreen changes retain the message. Escape still skips camera
cutscenes; their pause prompt names P only. Movie and carrier presentation are
unchanged by this flight-host overlay.

Validation: fresh Ghidra CODE_09 export, original instructions/A5 strings,
native and GUI builds, and Clang static analysis. No tests, game or GUI were run;
the Win32 source and build paths are unchanged.

### Movement comparison

Ghidra inspection of the original 68000 instructions supports sharing the
movement algorithms, with these qualifications:

- CODE_09 `players_flight_dynamics` +0x02de computes the same axis-rate times
  input divided by eight, with the same yaw/roll signs and blowing-up branch.
  SWC uses wider fields in its 204-byte object definitions.
- CODE_02 `rotate_object` +0x4306 applies pitch, yaw, and roll, then moves each
  rotation value toward zero. CODE_04 `alter_pitch` +0x2b1c, `alter_yaw` +0x2b6c,
  and `alter_roll` +0x2bba use the same basis-vector rotation and normalization
  sequence as WC1.
- CODE_05 `celerate` +0x09c6 clamps commanded speed to zero and maximum speed
  times 256. `accelerate` +0x428c applies the same malfunction adjustment before
  calling it. `your_afterburner` +0x42e0 and `fire_afterburner` +0x3a38 use the
  same fuel gate, eight/two-tick boost renewal, and velocity limit. Sound
  scheduling is platform-specific.
- CODE_02 `accelerate_and_move_object` +0x439e shares the forward target velocity,
  acceleration toward that target, drift, boost/brake timer, and fuel-drain
  calculations. Its final position update multiplies velocity by a seven-entry
  table indexed by elapsed Mac ticks clamped to 0..6 (+0x43ba..+0x43d2 and
  +0x471e..+0x4762). WC1 adds velocity once per simulation tick. The host retains
  WC1's fixed-step model rather than mixing the two timing systems.
- SWC also differs in CODE_04 `max_acceleration` +0x1a10 (one-third bonus for
  ratings 1..7 as well as ratings above 8) and `drain_fuel` +0x1944 (recalculate
  maximum speed when fuel reaches or passes zero). The shared WC1 routines
  apply those two conditions during SWC flight. WC1's original rating gate
  and retail array-address fuel check remain in place for WC1.
- CODE_01 `get_player_input` +0x4508 and `process_player_input` +0x3cba operate
  on Mac events, key-character state, and action flags. A matching function
  name does not establish identical input handling.

The movement and mouse-control updates were compiled with `make modern modern-gui`
and reviewed statically. No game, GUI, or interactive check was run for those updates.

### Player weapons

The flight host now calls WC1's object lifecycle instead of moving only the
initial ships. Guns use the shared energy gate, hardpoints, projectile creation,
convergence, collision/damage, and recharge code. Missiles use the shared loadout,
lock gate, launch timer, guidance, collision grace, expiration, and explosion code.
WC1 handles shield/armor damage, debris, and ammunition removal. Player destruction
stops player input, runs the live death cinematic, and enters the player funeral.

Original instructions support this reuse:

- CODE_09 `fire_players_lasers` +0x021a has the same cooldown/energy gate and
  selected-gun call. CODE_05 `fire_fixed_projectile_weapon` +0x38fe iterates
  enabled projectile slots; `fire_missile` +0x3856 requires a lock for HS/IR.
- CODE_05 `fire_weapon` +0x3320 shares the allocation, hardpoint, energy,
  lifetime, velocity and release flow. Its player convergence point adds 400
  world units along up for lasers and 300 for neutron/mass driver
  (+0x34c8..+0x3532). Expanded DATA/0 +0x31f2 has refire delays `{4,10,4,0}`,
  compared with WC1's `{6,10,4,0}`. Both differences apply only to SWC.
- CODE_02 `house_keep_objects` +0x3d38 and `update_objects_in_space` +0x418c
  follow the same lifecycle stages. This does not establish complete gameplay
  equivalence: WC1 still supplies collision/damage algorithms, and SWC's
  elapsed-tick movement scaling remains pending. Object parameters now come
  from the original SWC initialization instructions.
- CODE_02 `animate_shape` +0x4790 uses the same frame, loop, scale and removal
  commands. SWC explosion types 48/49/50 use 15/20/22 frames, scale 768/512/1024,
  and one tick per frame. The host supplies the recovered sequences from
  DATA/0 +0x2564/+0x261c/+0x25c0 to WC1's interpreter and restores the original
  definitions on exit. The final visible type-49 frame now triggers the original
  eight-color cockpit glare. Earlier documentation called this screen shake;
  CODE_09 draws a recolored PC00/19 mask without moving the camera. Mac handle
  management is not reproduced.

CODE_08 `get_Mac_mem` +0x0050..+0x022c identifies the projectile/effect OBJT
chunks; CODE_03 supplies debris aliases. The SDL cache loads each used resource
with its own frame count, including intentionally empty frames. Missiles use
MS00..03/1 (22 views). Ship views remain 37-frame sets. Resource data is borrowed
only while CMFs are open, and all textures are freed on normal/error exits.

The renderer now adds projected Y to the scene origin, matching both original
engines. WC1's cockpit-packet drawing is bypassed for messages, missile removal,
VDU changes and damage overlays; shared gameplay state is retained. Win32
reference branches remain unchanged. NPC ship AI and wave spawning are connected;
landing, the first debriefing and native weapon audio are connected; later
progression remains pending.

### SWC flight data, encounters and ITTS

`bin/extractSwcFlightData.py` reads the straight-line constant stores in CODE_08
`init_data_struct1` +0x0286 and `init_data_struct2` +0x47cc. It generates
`src/sdl/swc_objects.h` from the original CODE resource and the expanded DATA/A5
images. The generated header records all three input hashes. It translates the
58 object IDs shared with WC1 into host fields: ship motion, fuel, shields,
armor, weapons, projectile damage/lifetime and animation commands. The native
83-record, 204-byte table is never copied directly into WC1's packed records.
Ten-byte Mac weapon slots become seven-byte WC1 slots, with the quantity byte
preserved in SDL-only arrays alongside them; 32-bit ship fuel is
stored in WC1's 32-bit lifetime/fuel-capacity field. Mac pointers
are replaced by host strings/animation arrays or SDL-owned resources. Definitions
are installed for SWC flight and restored on exit. IDs 58..82 remain unsupported.

The generator requires Python Capstone and the analysis images, not a Mac compiler:

```sh
python3 bin/extractSwcFlightData.py \
  ../releases/mac/extracted/code/CODE_08.bin \
  out-modern/swc-cockpit/globals.bin \
  out-modern/swc-cockpit/a5-negative.bin src/sdl/swc_objects.h
```

CODE_04 `position_child` +0x367a indexes 63 vectors at DATA/0 +0x2efe.
Those offsets replace WC1's 56-entry table during SWC flight, including the
carrier's mounts 56..62. Pilot turn intervals at DATA/0 +0x3f82 match WC1's
18 entries, so its turn scheduler is shared directly.

The initial five ships are friendly. Enyo 1's original mission records place
three Dralthi at Nav 1, asteroids at Nav 2, and two Salthi plus asteroids at
Nav 3. WC1's nav-sphere and wave routines now instantiate these records;
team ships retain the original `spawnNav = -1` behavior across transitions.
CODE_04 `load_ship` loads asteroid OBJT/17 and /18, each with 12 frames.
The SDL renderer caches those sets instead of loading WC1 disk packets.

`A` calls WC1's `auto_pilot_sequence`. CODE_14 +0x0202 preserves the same
travel/formation algorithm; the SWC branch also resets the player's gun timer
and retains the arrival position when undoing the final step would change the
active nav sphere. SDL presents the original 120-frame external camera sequence
before travel, then restores the cockpit and advances the final simulation frame. NPC dispatch uses WC1's combat routines, with SWC's actual
ship-side test for patrols (CODE_02 +0x1c2c). Hazard updates and render-dependent
gameplay counters advance once per simulation tick. Communications, carrier
landing and the first debriefing are described above; later progression is pending.

Automatic acquisition enables target lock for a hostile; `L` toggles it.
`SwcIttsRangeCheck` and
`SwcUpdateItts` adapt WC2 `cockpt.c`'s `HasInRangeGunForTargetLead` (0x43ce8f)
and `UpdateTargetLeadIndicator` (0x43cf5a), checked against SWC CODE_13
+0x2bce/+0x2c48. They use enabled guns, projectile lifetime/range, the player's
forward velocity and the target's predicted position. SWC's cone threshold
`0x94`, 160-pixel projection, frame-5 marker, clipping and frame-4 reticle
overlap replace WC2's display-specific behavior. Missing/friendly targets,
destroyed targeting hardware, unlocked mode and out-of-range guns suppress ITTS.

Validation for this slice: original instruction/data inspection, static source
review and `make modern modern-gui`. No tests, game, GUI or interactive checks
were run. AI edge cases, combat balance and presentation need runtime review;
this is not a claim of complete SWC gameplay equivalence.

### Target acquisition and missile lock

`SwcCheckTarget` adapts WC2 `cockpt.c`'s `check_target` (0x43ddfc), checked
against Mac CODE_13 +0x3aa4. Visible ships within 12,000 units are sorted by
distance. The Mac keeps the current listed target or selects the nearest entry,
then enables lock mode for a hostile; a locked target can leave the screen
without being discarded. Friendly targets never retain lock mode. `T` uses the
matching shared cycle routine, which skips friendly ships while visible enemies
are available. Neutralized, removed and invalid targets are cleared safely.
The Mac has neither WC2's special target-camera selection nor its periodic
lock-mode malfunction check.

`SwcUpdateTargetLock` uses WC2 `target_locking` (0x43c048) as a structural
reference and Mac CODE_13 +0x36b2..+0x38de for behavior. Heat seekers take
18 countdown updates and require target facing <= -65; image-recognition
missiles take 32. Acquisition requires a projected radius <= 60 pixels.
Targeting malfunctions stall the countdown; destroyed targeting hardware,
friendly/missing targets, an unsuitable weapon or leaving the acquisition
circle turn missile locking off. An offscreen target preserves the countdown
without advancing it. There is no WC2 torpedo-lock branch or WC1 random marker
angle, extra lock-completion malfunction or text banner. The native snd/21
search pulse and snd/22 acquired tone use the SDL mixer. The original passes
signed projected X times 2/5 as search volume; SDL clamps it to 0..255.

CODE_13 `draw_target_box` +0x3114 uses CKPT/36 for normal friendly/hostile
corners and a separate blue highlight for the transmitting ship. Four CKPT/5
sprites converge from the countdown distance around projected center minus
(14,14), then blink on odd simulation frames after acquisition. DATA/0
+0x43f2 binds CKPT/5 to the original lock-shape handle. The right VDU starts
in target mode, matching CODE_03 `init_vdus` +0x0914; cycling targets preserves
an active communication portrait. Projection is refreshed before each targeting
update, including catch-up ticks. Pause does not advance the countdown or blink.

This pass changes only SDL source and its SDL-only header. Fresh Ghidra
CODE_05/13 exports, expanded resource tables and CKPT/5 frame headers were
checked against the WC2 source and original disassembly. `make -j4 modern
modern-gui modern-swc` passed; Clang static analysis of the three changed SDL
units reported no findings. No tests were added or run, and no game, GUI or
audio playback was launched. Interactive visual/audio parity remains unverified.

### Hornet cockpit

The cockpit presents shared WC1 state through the Mac demo's artwork.
WC1's `update_digital_readouts` supplies actual and commanded speed; the SDL
readout adapter applies SWC's four-digit zero padding and original positions.
`get_color` classifies contacts, `rotational_pos_to_scanner_pos` supplies the
radar math, and `set_objective_range` / `objective_name` supply navigation data.
The shared scanner routine selects the Hornet geometry and SWC's extra one-pixel
X offset only during SWC flight. The Win32 reference branches are preserved.

| Instrument | Original evidence | Current presentation |
| --- | --- | --- |
| Actual / commanded speed | CODE_13 `update_digital_readouts` +0x0660; expanded DATA/0 +0x666 / +0x696 | Baselines (96, 32) / (205, 32), using the shared WC1 speed calculations |
| Weapon energy / fuel / throttle | CODE_13 `update_bars` +0x0520; DATA/0 +0x420 / +0x78c / +0x786 | PC00 chunks 15 / 16 / 17; Hornet frame maxima 10 / 13 / 13 |
| Hull, armor and shields | CODE_13 `show_weapon_disp` +0x0e54 | PC00/10: hull, armor flashing below half strength, fore/aft shield presence |
| Radar | CODE_13 `draw_3d_scanner` +0x285a; DATA/0 +0x300 / +0x390 | CKPT/31 contact sprites, 30,000-unit cutoff, nav marker, then PC00/18 grid at (139, 151) |
| Destination / range | CODE_13 `update_digital_readouts`; DATA/0 +0x6f6 | Baseline (200, 94), 11-pixel line spacing, `FAR` at 32,000 or above |
| Target / range | CODE_13 `update_digital_readouts` +0x0660; DATA/0 +0x6c6 / +0x2ca4 | Baseline (200, 110), pilot callsigns for ratings 0..7, ace names for 9..12, otherwise SWC ship names; shared surface range |
| Target brackets | CODE_13 `draw_target_box` +0x3114; CODE_01 `get_Mac_boundaries` +0x0ae2 | Original CKPT/36 corner masks, Q16 bounds, friendly/hostile colors and distinct blue speaker offsets |
| Missile lock | CODE_13 +0x3336..+0x340a, `target_locking` +0x37c0; DATA/0 +0x43f2 | Four CKPT/5 markers close in over 18/32 countdown updates, then blink after acquisition |
| Target hull / armor / shields | CODE_13 `show_target_disp` +0x1270; DATA/0 +0x756 | STnn/2 or SHnn/38 at (230,136), with PC00/9 shields |
| Gun / missile selection | CODE_13 `update_status_text` +0x102e; DATA/0 +0x726 | Original short names at (51,84), fifteen-update selection notices, followed by computer messages |
| Reticle | CODE_13 `overlay_head_up_display` +0x2f56; DATA/0 +0x792 | CKPT/1 frame 0 at (144,96), including frame and cockpit banking offsets |
| Forward navigation marker | CODE_13 `draw_nav_pointer` +0x3460 | Shared WC1 projection, CKPT/1 frame 3; visible in target mode too, with stale offscreen positions cleared |
| Hostile direction | CODE_13 `draw_3d_scanner` +0x285a | CKPT/1 frames 6..13; tracks the last hostile target or first enemy contact within scanner range |
| Autopilot / missile / severe damage lamps | CODE_13 `update_lights` +0x0408; DATA/0 +0x3d8 | PC00/12, /14, /13; shared WC1 warning queries refreshed every seven simulation ticks, damage alarm at (156,45) |
| Malfunctioning VDUs | CODE_13 `malf` +0x1a28, `update_VDUs` +0x0aa8, `update_dead_disp` +0x0dda; CODE_03 `init_vdus` +0x0914 | VDUS/1 and /2 static mapped onto the original Hornet polygons, replacing live displays |
| Component status | CODE_13 `show_weapon_disp` +0x0fc8, `update_status_text` +0x102e; DATA/0 +0x886 / +0x89a / +0x8ae | Cycles damaged components every 30 healthy weapon-display updates, with original names, severity labels and colors |
| Damaged panels / sparks | CODE_13 `place_damage_on_cockpit` +0x41ae, `cockpit_explosion` +0x4222, `explosion_draw` +0x40a0; DATA/0 +0x7f2 | PC00/7 overlays disable radar/left VDU/right VDU; CKPT/6 supplies the eight-frame spark sequence |

The layout constants come from DATA/0 expanded using the original CODE_01
startup decoder. Offsets above refer to the expanded A5-relative data, not
compressed resource-file offsets. Resource lists at +0x433e and +0x43e6 identify
the shape sets. The SDL cache verifies their frame counts and places each frame
using its signed x/y offsets, following CODE_01 `MacDraw2` +0x0ef4. Radar contacts
are transformed from their current world positions, as in SWC; this avoids the
scene projection cache omitting objects close to the player.

Spatial markers and target brackets are drawn before the cockpit artwork,
following CODE_13 `update_cockpit` +0x02d4, so solid panels mask them. Text remains
on its own overlay, and panel damage/sparks are drawn last. Target bounds include
the same fixed-point corners used to rasterize the ship.
The hostile arrow is suppressed outside scanner range to avoid stale coordinates.

Display state, random static frames, component cycling and sparks advance only
on simulation ticks, so render rate and pausing do not change their duration.
The shared `malf` entry point uses SWC's countdown during SWC flight, including
when WC1 targeting or damage logic calls it. CODE_04 `dp_random` +0x5d44,
`random_number` +0x5d8c and `Random3DO` +0x5dea exclude the upper bound for
these small positive ranges: SWC's malfunction test samples 0..14 and its
countdown samples 5..9, while dead VDUs
select frames 0..2 from the four-frame resource. The SDL adapter adjusts WC1's
inclusive random bounds without replacing its RNG. Likewise CODE_05
`your_internal_damage` +0x184a selects three cockpit regions, not WC1's four.

VDU rendering respects the shared mode stack and SWC's physical-damage flags.
`show_navigation_disp` +0x121a, `show_damage_disp` +0x123a and `show_info_disp`
+0x1256 are empty functions in the Mac demo. Destination/range text is produced
by the separate readout layer, not a missing navigation-page implementation.
CODE_01 `MacDrawcel2` +0x14a4 maps static over the quad from DATA/0 +0x426/+0x5a6;
the default SDL path uses the recovered fixed-point polygon rasterizer and ignores
frame-origin offsets for this operation. Communications use their separate +0x4e6 polygon and WGMN
frames. Cockpit damage, malfunction and communication state reset on a new flight.

This pass supports the Enyo 1 Hornet cockpit. Ship capacities use the recovered
SWC definitions. Shield, armor and weapon-energy displays now respond to shared
combat state. Target brackets use the original `get_Mac_boundaries` calculation
and CKPT/36 corner masks. Flight sprites and VDU polygons use the recovered
rasterizer by default; enhanced OpenGL maps the same corners with GPU triangles.
Target names use pilot callsigns or the original Bhurak/Dakhath/Khajja/Bakhtosh
ace strings for named fighters; other targets retain SWC ship names. Series 6's
Valkyrie override is retained even though later missions are not connected.
The WC1-only bottom-edge ITTS, Need Lock and MISSILE LOCKED banners are removed
from SWC. CODE_13 `set_global_message` +0x3f4e writes DATA/0 +0x4712/+0x4716,
but neither value is read by this demo executable. CODE_05 `fire_missile`
+0x38b0 silently rejects an unlocked shot; the Mac provides reticle and sound
feedback. Shared damage notices and host communications acknowledgments now use
the native status queue and its lifetime. Target readouts remain at `(200,110)`
and `(200,121)`, with navigation at `(200,94)` and `(200,105)`.
The cockpit toggle retains the spatial HUD, weapon/component status,
target/navigation text, and messages. ITTS and the L lock control are connected.
Banking, pilot animation, communication portraits, computer-message timing,
damage sounds and alarms are connected as described above. Automatic target
acquisition, missile-lock animation and separate speaker brackets are connected.
Side/rear views and other cockpitless instruments remain pending. The autopilot
lamp uses SWC's eligibility and escort-wait gate; missile/damage lamps reuse
WC1's queries.

#### Text font

The inspected Mac cockpit layers request QuickDraw font ID 21 (Helvetica) at
size 9: CODE_06 `AllocTextLayer` +0x1d9a and CODE_14 `init_text` +0x1fec.
CODE_06 `tprintf` +0x1ef4 advances a newline by font size plus two pixels.
Cockpit, map, pause and carrier text share the font adapter at sizes 9, 10 and
14. Colored label/value segments share a pen position instead of fixed columns.
For classic pixels, supply a resource fork containing Helvetica (FOND ID 21)
as `Fonts/Helvetica.rsrc` under the game-data root, or point the
`SWC_MAC_FONT_RESOURCE` environment variable at it. Raw resource forks and
AppleDouble files are accepted; on macOS the path can name a font suitcase's
`..namedfork/rsrc`. No font data is redistributed.

The reader follows FOND's plain-style associations to exact-size NFNT/FONT
strikes and preserves their one-bit pixels, glyph bearings, advances and ascent.
For old FONT-only resources it uses family ID * 128 + point size. Resource
layout follows Apple's [NFNT documentation](https://developer.apple.com/library/archive/documentation/mac/Text/Text-250.html)
and [FOND associations](https://developer.apple.com/library/archive/documentation/mac/Text/Text-271.html).
The demo contains no such fonts and no local classic Helvetica resource was
available for validation. Unsupported/missing strikes fall back to modern
system Helvetica on macOS (without antialiasing/smoothing), whose pixels are
not claimed identical to QuickDraw.

Other hosts retain the generated **SWC HUD Bitmap**, a monochrome ASCII atlas
with a five-pixel advance. The TTF, source hash and SIL Open Font License remain
in [`third_party/anonymouspro`](../third_party/anonymouspro/README.md); the optional
[`bin/buildSwcHudFont.py`](../bin/buildSwcHudFont.py) generator records rasterizer
versions. Release archives include the license. This remains the portable fallback
when no classic strike is supplied. Bytes outside printable ASCII and QuickDraw
font synthesis remain pending.

`update_status_text` +0x102e starts at (51,84), draws a damaged-component line,
then any active gun/weapon selection notices, then the computer message.
CODE_15 `select_new_gun` +0x0222 and `select_new_release_weapon` +0x0398 reset
fifteen-update notices, including automatic weapon changes after firing.
The SDL hooks preserve the Win32 reference branches. `UpdateMessage` +0x000c
retains its flash countdown across messages and lets the lifetime/flash clocks
advance during communications, deferring the next message until speech ends.
Voices start when dequeued; missing audio falls back to timed text.

#### Sprite rasterization and cockpit glare

`src/sdl/swc_raster.c` adapts CODE_01 `rotate_Mac_shape2` +0x10ae,
`MacDrawcel2` +0x14a4, `adjust` +0x5a28, `span` +0x5a5a, `edge` +0x5d00
and `poly` +0x5e3a. Fresh Ghidra instructions establish Q8 corners/UV, the
scale-before-dimensions truncation, width/height-minus-one UV endpoints,
first-extrema edge chains, exclusive bottom row, and unrounded horizontal
clip interpolation. CODE_09 +0x38d8 draws `max(2, span-1)` texels, skipping
palette zero. The adapter preserves these rules, using explicit source/output
bounds to avoid the original unchecked memory accesses. SDL presents the
completed pixels, replacing rotated rectangles and textured VDU triangles.

Optional `--enhanced` rendering keeps those Q8 transforms but uses SDL's OpenGL
backend to draw textured triangles at the window's output resolution. Each
sprite caches its fixed flight palette as a texture, with palette zero
transparent and nearest sampling. This deliberately uses GPU coverage instead
of the software rasterizer's exact span rules. Menu and movie rendering remain
within the same SDL texture/target API; the separate WC1 indexed-frame shader
does not process SWC frames.

CODE_02 `Cos` +0x557c and `Sin` +0x5608 fold angles into DATA/0 +0x4550's
91-word Q8 quadrant table. Their three-letter MacsBug names were omitted by
the older symbol extraction, which incorrectly extended `Magnitude` over them;
the fresh instruction export resolves their actual entry points.
CODE_01 `get_Mac_boundaries` +0x0ae2 is now separate from the drawing quad:
it retains the low scale bit, calculates corners in Q16 and floors all extrema.
CODE_13 `draw_target_box` +0x3114 colors four 6x6 CKPT/36 masks with MacDrawColor2
semantics. Normal corners start at left/top minus 3 and right/bottom minus 3;
speaker-highlight corners use minus 5 and minus 1 respectively. Both renderers use these
original bounds, including the software rasterizer's deliberate Q8 difference.

The SDL projector follows CODE_04 `transform_objects_to_your_view` +0x4150:
160-pixel projection divides the full 64-bit XY product; sprite scaling retains
the wide result until its 0..8192 clamp. Depth sorting preserves 32-bit distances,
while shared WC1 targeting receives a saturated 16-bit compatibility cache.
Mac dust cutoff/size constants (5,000 units and 0x2100) and sky-depth sentinels
are recovered. CODE_02's class-2 stars now draw the original PLNT/1 frames
32..41 with unscaled `MacDraw2` origins; class-4 dust is one palette-128 pixel.
Both participate in depth sorting after glare. Native `generate_stars` +0x38cc
and `update_star_field` +0x39f2 replace the WC1 initialization/respawn ranges:
eight dust slots use a 5,000-unit initial volume, seven stars use the native
exclusive frame choices, and respawning retains the Mac camera offset and
velocity-dependent distances. This also removes WC1's possible frame-42 choice.
Shared `start_dust`, hazard maintenance and WC1 RNG remain; the adapter does
not claim the same random sequence as the Mac. Automatic acquisition uses the
refreshed projection cache; the separate nearest-ship camera candidate remains pending.
Pixel parity has not been checked against a running original.

Ejection and cockpit validation: fresh Ghidra instruction/data inspection,
read-only CMF/resource-fork inventories, `make -j4 modern modern-gui`, and Clang
static analysis of the three SDL files. Builds passed with the existing packed-data
linker alignment warnings; static analysis reported no findings. No tests were
added or run, and neither the game nor the GUI was launched. All implementation
changes are in SDL-only files; the Win32 reconstruction is unchanged.

The effect previously described as explosion screen shake is cockpit **glare**.
CODE_02 `animate_shape` +0x4832 triggers it at animation index 19 of a visible
type-49 explosion. CODE_05 `ShipExplosion` +0x2302 uses type 50 for fighters
and type 49 for capital ships; the SDL hook now preserves that distinction.
CODE_09 `handle_space_glare` +0x0128 calls `MacDrawColor2`
with the PC00/19 mask before sorted objects and cockpit art. DATA/0 +0x1cba
supplies eight colors, traversed 129,131,140,144,161,176,192,237. Player hits
use the four-color sequence 191,212,236,253 at +0x1cb0 and cannot interrupt
explosion glare. The mask includes its (0,2) frame origin and cockpit banking;
there is no random camera displacement in this path. Action-sphere changes
clear it. The SDL host advances these effects on simulation updates rather
than render calls, so pause and redraw frequency do not change the duration.

Validation: fresh Ghidra exports and offline original resource inspection,
native/GUI/viewer compilation, Clang static analysis, portable-font syntax
compilation and reference `binary-comp` checks. No tests, game, GUI, audio
playback or interactive checks were run. All six changed core functions retain
their previous Win32 `binary-comp` scores, recorded in the provenance inventory.
Appearance and timing need user review.

SWC binary similarity measurement is no longer part of this work: the original
Mac compiler/toolchain is unavailable. Validation uses original instructions,
actual data, source inspection and compilation. WC1's reference-build
verification remains in place. These checks do not establish equivalence for
every SWC gameplay path.

[`config/swc-provenance.json`](../config/swc-provenance.json) records reference
hashes, CODE-resource offsets, source symbols, and differences in the host
adaptations. It is an evidence inventory, not a guessed tool configuration.
Keep CODE-resource offsets distinct from WC1 virtual addresses and Ghidra's
temporary import base. Ordinary `Function start:` annotations are reserved for
the WC1 executable so its export and verification tools do not mix binaries.

The current resource readers are host adaptations with extra bounds checks and
different memory APIs. `SwcDecodeFrame` only uses `DrawRLEImage` as format
evidence; it is not a reconstruction of that whole drawing routine. SDL window,
resource-fork, allocation, and input plumbing have no one-to-one original game
function mapping. Do not infer code reuse percentages from matching names;
many WC1 names were already derived from this Mac executable's MacsBug strings.

## In-flight computer and selectable cameras

The SDL flight host exposes the original MENU/206 bindings: Y returns to the
cockpit, D toggles battle view (14), and F toggles chase view (4).
CODE_01 `DoViewsMenu` +0x3052 and `process_player_input` supply the toggles.
The camera updates follow CODE_02 `new_view` +0x3190,
`set_eye_direction_and_position` +0x25d8 and `battle_view` +0x58f6.
Chase starts 1,200 units behind the player, approaches a 700-unit trailing
position with the original 1/25 response, and follows the player's roll.
Battle view frames the active fleet, switching to player-relative framing
when ships are widely separated. Both render the player's ship without cockpit
layers. Map visits preserve the view, and autopilot repositions it on arrival.
The shared WC1 camera and Win32 reconstruction are unchanged.

`src/sdl/swc_nav.c` presents all five pages of CODE_14 `InflightComputer`
+0x2dd2. It loads `ComputerInflight.CMF` CKPT/2 (frame), /3 (backgrounds),
/4 (selected tabs), /9 (weapons), /10 (markers), /12 (ship silhouettes) and
/13 (eject guard), using GameMode(11)'s clut/250. The comm/guns/weapons
background frames are intentionally empty. Mac `BuildMap`
+0x14f8, `FindTextPos` +0x142c and `SetScale` +0x1c8a supply the 137x138
map at (41,43), 10,000-unit margin, marker types/colors, objective visibility,
visited state, player position, hazard fields and label placement.
`UpdateInflightNavText` +0x273a supplies the sector, system, flight, mission
role and selected-objective note, with size-9 text coordinates and 11-pixel
line spacing. Map labels use size 10. Original classic-Mac font pixels remain
subject to the existing Helvetica/fallback limitation.

CODE_14 `UpdateCommStatus` +0x290e and `print_message_text` +0x289a supply
recipient/order lists, conditions, numbering, colors and text positions.
Recipients include the wingman, friendly carrier and targeted enemy fighter.
Orders include formation/attack, radio silence, assistance, taunts and landing
clearance. The computer queues the native player voice and calls shared
`request` with the command ID directly; WC1's separate C-key menu remains usable.

The gun and weapon pages draw the installed mounts over the ship silhouette,
using the forty x/y/Q8-scale triples at DATA/0 +0x0030. Selected mounts blink,
and original short names and ammunition quantities appear alongside them.
Tab/Right calls the existing gun/missile selectors. CODE_14 +0x2ef2 supplies
the eject guard's five-update animation; the warning background alternates
every ten updates. Activation uses +0x34e6's ejector damage/random check and
failure sound 25 before the existing ejection cinematic.

The Mac's slot quantity at byte +9 is now retained by the flight-data generator
and copied during SWC ship setup. CODE_05 `remove_weapon` +0x019a decrements
quantities above one before removing/compacting a mount. The Hornet therefore
starts with four Dumbfires and two Heatseekers, rather than losing each entire
mount after one shot. The native full-loadout bound also protects the spare-slot
write. These additions are guarded by SDL_PORT and SWC flight state; WC1 slot
layout and the original Win32 function bodies are preserved.

Flight and audio pause while the computer is open. SDL supplies mouse input,
controller navigation, release-to-activate actions, focus/cursor restoration
and a bounded objective search. Animations use a 50 ms host clock. The briefing
map shares the native map builder; the optional font resource supplies classic
bitmap text as described above.

Validation uses fresh Ghidra instruction exports, offline CMF/resource-fork
inspection, native and GUI builds, and Clang static analysis. The computer and
flight units have no analyzer findings; `disk.c` retains an existing unrelated
uninitialized error-report argument. binary-comp's Win32 scores are unchanged:
`remove_weapon` 90.79%, `set_objects_data` 93.94%. No tests were added or run,
and neither the game nor the GUI was launched. Interactive appearance and
controls still need user review.

## Next support slices

1. Continue comparing shared AI/collision behavior with SWC instructions,
   including RNG bounds, elapsed-tick motion and later mission modes. The
   common 58 object definitions are recovered; Mac IDs 58..82 remain unsupported.
2. Continue remaining 320x240 HUD work. Mac projection, target brackets and
   pilot/ace names and native star/dust rendering are implemented. Exact font
   pixels require a supplied Helvetica resource; interactive visual parity remains
   unverified. Automatic targeting, missile-lock feedback and speaker brackets
   are implemented; the separate nearest-ship camera candidate remains pending.
3. Continue later campaign communications, remaining HUD modes and
   post-debriefing progression. Landing and the first debriefing are connected;
   the demo disables its simulator and ends after Enyo 1. Office/award ceremonies
   are later campaign work, not missing demo visits (see the postflight audit).
4. Connect later scripted scenes and save/load. Bunk art now follows optional
   original preferences occupancy, but saved campaign state is not imported.
   The native briefing map, walk-out, incidental room animations and movie
   palette dimming are connected. The first bar, barracks and briefing, AIFF speech and Hornet
   startup/opening/launch/landing playback, first debriefing and player/Spirit funerals are
   present, along with the title battle, idle replay, death cinematic, stranded
   BUC sequence and animated end background. Other scene entry points remain pending.
