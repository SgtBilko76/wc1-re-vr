# Super Wing Commander Mac demo

The GUI recognizes the SWC Mac demo and opens the Tiger's Claw bar and barracks.
Talk to Shotglass, Paladin and Angel, check pilot rankings or medals, then enter
the briefing room and launch the experimental Enyo 1 flight with five ships,
four objectives, and the original 320x240 cockpit and sprites.
SDL2 handles input and presentation; WC1 supplies mission state, ship setup,
flight dynamics, movement, projection, ship-view selection, navigation, and
player weapons, including projectile collisions, damage, and missile guidance.
Flight now uses the recovered SWC ship/weapon definitions, shared WC1 NPC AI,
autopilot, encounter spawning and asteroid fields. The targeting lead indicator
adapts WC2's matching calculation to SWC's original projection and cockpit art.
The Hornet cockpit now has speed and navigation readouts, fuel/throttle/energy
gauges, player/target shield and armor displays, radar contacts, weapon status,
navigation and hostile-direction markers, warning lamps, and damaged displays.
The original conversations, briefing, debriefing and funeral scenes use the
demo's text, portraits and AIFF speech. Hornet launch, landing and funeral
movies have embedded audio.
WC1 communications now provide wingman orders and landing-clearance requests;
returning to the carrier ends Enyo 1 with the original debriefing branches and
Spirit's funeral if she was lost. Player destruction leads to the player's
funeral and an end card.
Later missions, campaign progression, remaining HUD modes, voiced flight
communications, the separate death cinematic, flight effects and music are
still pending. Landing and funeral paths have been compiled and inspected,
without interactive execution.

`make modern-swc` builds the asset viewer. It loads the demo's CMF files,
decodes sprites, reads the resource-fork palette, and displays the cockpit and
13 ship shape sets at 320x240. Left/right select a ship view, up/down select a
ship, C toggles the cockpit, and Esc exits.

`src/swc/` contains the resource readers. `src/sdl/swc_mission.c` translates
Mac mission records into WC1 state through the existing `LoadMissionData`
entry point in SDL builds. The CMF reader and mission adapter are linked into
`wc1-modern`; the original Win32 reference build does not include them.
`src/sdl/swc_demo.c` remains a separate presentation/inspection entry point.
`src/sdl/swc_flight.c` hosts the experimental flight using the shared core.
`src/sdl/swc_cockpit.c` presents the Hornet instruments. `src/sdl/swc_text.c`
shares bitmap text between the cockpit and carrier. `src/sdl/swc_rooms.c`
adapts carrier presentation and scenes; `src/swc/speech.c` reads AIFF speech.
`src/swc/movie.c` reads and decodes the Mac movies; `src/sdl/swc_movie.c`
supplies SDL2 presentation, queued audio, and the launch/landing/funeral sequences.
The viewer needs only the host C compiler and SDL2. Mission tests also link
the existing native core and need its normal build dependencies, including LZO.
Flight uses SDL2 2.0.18 or later for the VDU polygon rendering.

## Status review — 2026-10-12

The current scope is an experimental Enyo 1 session. Resource loading and the
initial flight checks passed; a complete interactive mission and presentation
parity with the Mac executable are still unverified. The autopilot reconstruction
below follows the initial status review.

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
| Campaign and death flow | CODE_09 `GameFlow` calls `PostMission` at +0x1f46 and `UpdateSeries` at +0x1f4a; its death case calls CODE_14 `death_sequence` at +0x1e1c before the player funeral at +0x1e44. | The host always starts series 1 / mission 0, ends after debriefing/funeral, and enters the player's funeral directly on destruction. Save/load, later progression and the separate death presentation remain absent. |

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

Flight sound effects remain deliberately suppressed in `SdlPlayGameSoundEffect`.
Music, inflight voices/portraits, opening/death presentation, exact text and
palette transitions remain incomplete despite functioning speech/movie decoders.
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
array-bound guard. Speech IDs 1..7 remain disconnected.

`SdlSwcVisitCinema` in `src/sdl/swc_flight.c` reconstructs CODE_14
`visit_the_cinema` (+0x000c). Fresh Ghidra disassembly of CODE_02's view switch
tables confirms the view-12 setup at +0x3474: 500 units to the side and 2,000
ahead, looking at the player. The camera update at +0x29f0 moves ten units
sideways per frame. The host replaces Mac event handling and sound with SDL
pause/focus/quit handling and silent presentation; its pacing remains the
existing fixed flight interval. It restores the prior camera on exit.

Pressing A now runs this 120-frame external view before the shared formation
travel. The renderer includes the player's ship and omits cockpit/HUD overlays.
The host updates one objective per frame, including unselected objectives, as
CODE_13 +0x0316..+0x034e does. P and focus loss pause the sequence; Esc or window
close exits the session. Camera and input state are restored on exit, and a
presentation failure propagates to the mission host. CODE_14 +0x077e's final
simulation frame runs after travel.

All shared-core dispatch is guarded by `SDL_PORT` and `SdlSwcFlightActive()`;
SWC state and presentation live in the SDL sources. Native and GUI builds pass.
`binary-comp` reports identical Win32 scores before and after integration:
`visit_the_cinema` 100%, `auto_pilot_valid` 100%, `auto_pilot_sequence` 96.91%,
and `flag_reached` 85.84%. Integrated gameplay remains unverified; validation
for this change is limited to source/assembly inspection and compilation. No
SWC binary similarity score is claimed.

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
Space or Enter advances dialogue; Esc returns from a conversation or skips the
briefing into launch. P pauses dialogue and speech; focus loss also pauses both.
Esc returns from the barracks, rankings or medals, and exits from the bar.
Window close exits at any point. The simulator retains the demo's unavailable
message. Save/load bunks explain that SWC saves are not supported yet.

With the archive's `Movies/` directory present, the Hornet launch follows the briefing.
Space, Enter, Esc, or a left click skips the entire sequence on release; P
pauses it. Losing focus pauses video and audio together. Closing the movie
window exits without starting flight. Once in flight, Esc resumes its normal
exit behavior and Space/Enter/mouse buttons resume their weapon bindings.

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
| +/- or keypad +/- | Change commanded speed while held |
| Backspace | Set commanded speed to zero; the ship decelerates |
| Tab or keypad * | Afterburner, subject to WC1's fuel and timer checks |
| Space / left mouse button | Fire selected guns; holding repeats while energy and cooldown permit |
| Enter / press both mouse buttons | Release one selected missile; guided weapons require a lock |
| G / W | Cycle gun selection / missile type |
| T | Cycle visible targets using WC1's range ordering and enemy preference |
| L | Toggle target-lock mode; ITTS leads a hostile fighter when guns are in range |
| A | Autopilot toward the selected objective, stopping for enemies or hazards |
| N / M | Next objective / nav map |
| C / 1..7 | Open/close WC1 communications / choose a numbered recipient or command |
| Ctrl+F1 | Toggle cockpit |
| P / Esc | Pause / close communications, otherwise exit |

The window title shows the selected objective, actual and commanded speed,
and afterburner state. The pointer is confined to the flight window; P releases
it while paused. Losing focus also pauses movement and releases the pointer.
Keyboard steering takes over until the next mouse movement. Releasing the
right-button roll/throttle mode recentres the pointer, as in WC1. Holding both
buttons releases one missile per press; a latched right-button afterburner keeps
its existing behavior and allows left-button guns. Joystick flight is still
unsupported; the GUI disables its options. Q/E are no longer roll bindings.

To return, select the Tiger's Claw objective with N and travel back with A
when autopilot is available. Use T to target the visible carrier, then C and
the numbered choices to request landing clearance. The shared rules can deny
an immediate return or a return with enemies nearby. With clearance granted,
approach within 700 range units without afterburning to begin landing. Skipping
landing movies continues into debriefing; closing the window exits. If Spirit
was lost, her funeral follows debriefing. Player destruction instead starts the
player's funeral without landing or mission-stat updates. Click, Space or Enter
advances funeral dialogue; Esc skips the rest of the funeral. Skipping a funeral
movie also ends the sequence. The player's completed farewell leads to a static
THE END card that waits for input. These paths end the current demo session,
returning to the launcher when started there.

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

The asset viewer's `--check` presents one frame and exits. `--snapshot FILE.bmp` also saves that
320x240 frame and exits. The check runs with SDL's dummy video driver:

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
| `FindMenuRegionAtPoint`, `IsPointInRect` | Hotspot hit testing, with coordinates recovered from the Mac instructions |
| `ConversationSceneRecord`, `ParseTests` | Original 13-byte scene commands and conditional branches |
| `AddPCName` | Player callsign, name, rank and player/wingman kill substitutions in original dialogue |
| `ParseMouthAnimation`, `ParseFaceAnimation` | Original script strings decoded into animation commands |
| `Build_objective_list`, `SetScale`, `nav_getxy` | Shared mission objectives and briefing-map coordinates |

WC1's complete `SceneDirector`, `RecRoom` and `BarracksScreen` call DOS packet,
viewport, font and input routines. The SDL adapter uses their reusable logic
and data types, with separate presentation for SWC's Mac CMFs and palettes.
The ranking sort retains the same kills/missions rule as WC1 and SWC. No
reference-build functions were edited for this feature.

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
chunk padding, data offsets and declared sample counts. SDL queues the PCM;
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
The scene adapter preserves original dialogue and artwork but does not claim
exact Mac presentation: audio-marker lip synchronization, scene-specific text
colors and typography, camera pans, palette fades, music, hover/decorative
animations and exact idle timing are pending. Mouth/face commands currently run
on a host clock; the map uses WC1 coordinates with simple SDL markers. The
Anonymous Pro bitmap and readable subtitle panel are shared with the cockpit.

Mission state is prepared before the carrier for the briefing map; simulation
starts only after launch. Actions complete on release, focus loss pauses
speech and scene time, and textures/audio are released on every exit. The
finite `--check` path bypasses both rooms and movies. Validation for this change
was original-instruction/data inspection, static review and compilation with
`make modern modern-gui`; no tests, interactive code, game, GUI or audio
playback were run. The existing launcher fixture was updated for the required
CMFs but was not executed. Visual behavior and audio timing still need user review.

## Carrier return and debriefing

The SDL flight host reuses WC1's communication recipient/command menus and
`request`, including wingman orders and landing command 12. C and number keys
follow DOS/Win32 controls; SWC's native CODE_06 `Chosen_communicate_option`
instead routes four quick commands. SDL draws the menu and clearance feedback;
original inflight voice/portraits are not yet presented.

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
Office/award ceremonies, save persistence and advancing to Enyo 2 remain
pending; dismissal ends this first-mission host after any funeral.

Validation: original Ghidra instructions and offline demo-data inspection,
static review, and `make -j4 modern modern-gui`. No tests, interactive code,
game/GUI sessions or audio playback were run. Existing packed-pointer linker
warnings remain; the launcher fixture only gained the required CMF filenames.

## Funeral scenes

CODE_09 `GameFlow` +0x1b8e routes player death to `funeral_sequence(1)`
(call at +0x1e44), without landing or `PostMission`. Its +0x218c check starts
`funeral_sequence(0)` after debriefing when the wingman was lost. The SDL host
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

The host presents a static THE END card after the player's farewell, retaining
`the_end` +0x4b1e's input wait. The original animated end background, separate
death cinematic, `FUNERAL.MooV` music, fades and exact scene timing remain
pending. Validation was original-instruction/data inspection, static review
and `make -j4 modern modern-gui`; no tests, interactive sessions or playback
were run. The GUI requires `Funeral.CMF`; its existing fixture was updated only.

## Launch, landing and funeral movies

The Mac demo stores 38 custom `LMov` files under `Movies/`, with `.dcMov`
extensions. These are not ordinary QuickTime movies. The new decoder follows
CODE_12 `LMovieOpen`, `LMovieTask`, and `LMovieDrawFrameMinRect`; the SDL host
replaces QuickDraw blits, Sound Manager double buffers, and Mac event handling.
WC1's `scramble` constructs its launch from sprites, so it cannot play these
files. Shared WC1 mission state is prepared before the carrier visit; flight
simulation starts after playback.

CODE_03 `scramble` +0x45d0 and its A5-relative strings specify this order:
`ARMOR`, `HALL`, `LAUNCH01`, `LAUNCH02`, `LAUNCH03`, `LAUNCH04.00`,
`LAUNCH06.00`, `LAUNCH07.00`, `LAUNCH08.00`. The `.00` suffix selects the
Hornet. The host checks that all nine files can be opened before playback;
missing/unreadable movies produce a stderr message and leave the existing
direct-flight path available. Invalid movie data reports an error. The finite
`--check` flight path bypasses movies.

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
palette, queues its embedded PCM, and rounds the original sample rate to an
integer Hz. Playback falls back to silent video if no audio device is available.
The video clock and audio device pause together, and the last frame remains
while longer audio finishes. All movie resources are released on finish, skip,
error, or window close.

The separate `MIDI/ARMOR.MooV`, `LANDING.MooV` and `FUNERAL.MooV` soundtracks,
palette fades and opening integration remain pending. First briefing, bar,
debriefing and funeral scenes use the carrier adapter described above. No
external video library is needed for this slice.

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
| `LoadMissionData` in `src/cmpgn.c` | Detects `CMFs/Data.CMF` and invokes the SDL adapter | The direct SDL flight host also calls this adapter; later campaign scenes are not connected |
| `set_sphere_point` in `src/brains.c` | Adds the existing nav position and ship-relative position | Validated with synthetic data and Enyo 1 |
| `Set_up_ship_info` / `init_intelligence_data` | Sets the Enyo player's position, orientation, speed, pilot, and mission state | Other mission modes and SWC AI differences need review |
| `Build_objective_list` | Builds Enyo's three nav objectives and carrier-return objective | Other SWC objective behavior is not established |
| `cycle_next_objective`, `nav_getxy`, `SetScale` | Cycles and projects the shared objective state | This uses WC1 map units/layout; SWC's `nav_getxy` uses wider coordinates and different scaling |
| `PollKeyboardState`, `process_player_input` | WC1 keyboard priorities, diagonals, gradual steering, reversals, and Shift response | Deliberately uses DOS/Win32 controls; SWC's Mac event/device layer differs |
| `player_input`, `QueueInputEvent`, `WarpMouseTo` | Original mouse response, edge limits, right-button roll/throttle, and recentering | SWC supplies only motion events and the right-button modifier; weapon events are excluded |
| `accelerate`, `celerate`, `your_afterburner`, `fire_afterburner` | Throttle bounds, boost activation, and boost timers | Audio calls stop at the SDL boundary until SWC sound is implemented |
| `players_flight_dynamics`, `rotate_object`, `accelerate_and_move_object` | Steers and moves ships and active weapons using SWC parameters | Fixed WC1 simulation step; native elapsed-tick scaling remains pending |
| `fire_players_lasers`, `fire_fixed_projectile_weapon`, `fire_weapon`, `fire_missile` | Energy, cooldowns, projectile initialization and missile release | Recovered SWC definitions and all 63 weapon hardpoints; SDL-only aiming offset and laser cooldown |
| `house_keep_objects`, `update_objects_in_space`, `object_intelligence` | Lifetimes, collisions, damage, ship AI, missile guidance, recharge and animations | SWC dispatch and common setup verified; complete AI/probability equivalence is not established |
| `ReleaseStaleNavTarget`, `set_up_action_sphere`, `init_ship`, `check_next_wave` | Enter nav spheres and spawn their ships/hazards/waves | CMF textures replace WC1 packet loading; later mission scripts remain unsupported |
| `auto_pilot_sequence`, `check_hazards`, `update_star_field` | Formation travel, encounter stops and hazard lifecycle | Original SWC boundary rollback and SDL presentation of the 120-frame external view |
| `select_new_gun`, `select_new_release_weapon`, `cycle_onscreen_targets`, `target_locking` | Shared selection and missile lock state | SDL presents SWC art and text instead of WC1's cockpit packets |
| `draw_nav_pointer`, `auto_pilot_valid`, `missile_on_tail`, `calculate_damage_level` | Shared objective projection and warning queries | SWC navigation activation and stale-marker reset; autopilot dispatches to the recovered SWC eligibility and escort-wait state |
| `malf`, `your_internal_damage`, `place_damage_on_cockpit` | Shared component damage and impact flow | SDL-only SWC malfunction countdown and three physical damage regions; original Mac static, damage and spark artwork |
| `SetSpaceFlightFrameTiming` | Initializes the shared flight tick interval, 50 ms at WC1's default 20 Hz | Replaces the host's hard-coded 62 ms; SWC's elapsed-tick translation scaling is not reproduced |
| `generate_stars`, `update_star_field`, `transform_objects_to_your_view`, `get_right_shape` | Projects the scene and chooses among 37 decoded ship views | Centered ship quads have original-code support; projection differences and exact rotation/scaling rasterization remain to be checked |

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

The SWC event loop owns event pumping. Its calls into `player_input` cannot
consume unfiltered SDL button or window events through WC1's separate loop.
Primary/secondary host button states stay clear, and no weapon button events
are queued. Right-button state is passed only as a motion modifier; two presses
within WC1's 20/60-second interval activate the shared `your_afterburner` routine
while the button is held. This timing-only check tolerates WC1's pointer
recentering between clicks. SDL handles weapon buttons separately and calls the
shared firing routines. Short left clicks remain pending until a simulation
tick. `HandleSpaceFlightControls` is still bypassed because its remaining HUD
and mission commands are not connected. SDL builds zero-initialize the legacy
input event before polling, since motion and empty polls leave some fields
unset. In SDL builds only, `accelerate` multiplies signed throttle steps by 256
instead of left-shifting negative values. The reference build retains the
original expressions and event declaration.

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
stops simulation/input and enters the player funeral; the preceding death
cinematic is pending.

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
  definitions on exit. Mac handle management and explosion screen shake are absent.

CODE_08 `get_Mac_mem` +0x0050..+0x022c identifies the projectile/effect OBJT
chunks; CODE_03 supplies debris aliases. The SDL cache loads each used resource
with its own frame count, including intentionally empty frames. Missiles use
MS00..03/1 (22 views). Ship views remain 37-frame sets. Resource data is borrowed
only while CMFs are open, and all textures are freed on normal/error exits.

The renderer now adds projected Y to the scene origin, matching both original
engines. WC1's cockpit-packet drawing is bypassed for messages, missile removal,
VDU changes and damage overlays; shared gameplay state is retained. Win32
reference branches remain unchanged. NPC ship AI and wave spawning are connected;
landing and the first debriefing are connected; later progression and weapon
audio are still pending.

### SWC flight data, encounters and ITTS

`bin/extractSwcFlightData.py` reads the straight-line constant stores in CODE_08
`init_data_struct1` +0x0286 and `init_data_struct2` +0x47cc. It generates
`src/sdl/swc_objects.h` from the original CODE resource and the expanded DATA/A5
images. The generated header records all three input hashes. It translates the
58 object IDs shared with WC1 into host fields: ship motion, fuel, shields,
armor, weapons, projectile damage/lifetime and animation commands. The native
83-record, 204-byte table is never copied directly into WC1's packed records.
Ten-byte Mac weapon slots become seven-byte WC1 slots; 32-bit ship fuel is
preserved across WC1's combined lifetime/weaponDamage storage. Mac pointers
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

`L` enables ITTS for a selected hostile fighter. `SwcIttsRangeCheck` and
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
| Target / range | CODE_13 `update_digital_readouts`; DATA/0 +0x6c6 | Baseline (200, 110), using WC1's object name and surface range |
| Target hull / armor / shields | CODE_13 `show_target_disp` +0x1270; DATA/0 +0x756 | STnn/2 or SHnn/38 at (230,136), with PC00/9 shields |
| Gun / missile selection | CODE_13 `update_status_text` +0x102e; DATA/0 +0x726 | Original short names at (51,84); host keeps them visible and adds ammunition/lock state |
| Reticle | CODE_13 `overlay_head_up_display` +0x2f56; DATA/0 +0x792 | CKPT/1 frame 0 at (144,96), including the frame's offsets |
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
the SDL sprite rotation around the same explicit pivot used to draw the ship.
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
the SDL replacement uses `SDL_RenderGeometry` and ignores frame-origin offsets
for this operation. Cockpit damage and malfunction state reset on a new flight.

This pass supports the Enyo 1 Hornet cockpit. Ship capacities use the recovered
SWC definitions. Shield, armor and weapon-energy displays now respond to shared
combat state. Target brackets use SDL corners around the rotated sprite bounds;
the original MacScale1 rasterizer and CKPT/36 corner artwork remain unimplemented.
Target names use SWC object names without the original pilot/ace-name substitutions.
Component messages and missile-lock notices use WC1 lifetimes at host positions.
The cockpit toggle retains the spatial HUD, weapon/component status,
target/navigation text, and messages. ITTS and the L lock control are connected.
Automatic target acquisition, lock animation, cockpit banking and pilot animation, side/rear views,
communication portraits, original message timing and other cockpitless instruments
remain pending. The autopilot lamp uses SWC's eligibility and escort-wait gate;
the missile and damage lamps reuse WC1's queries. Damage sounds and alarms remain
silent until SWC audio is implemented.

#### Text font

The inspected Mac cockpit layers request QuickDraw font ID 21 (Helvetica) at
size 9: CODE_06 `AllocTextLayer` +0x1d9a and CODE_14 `init_text` +0x1fec.
CODE_06 `tprintf` +0x1ef4 draws QuickDraw text, advancing a newline by font size
plus two pixels. SDL2 has no built-in font. The current substitute is a
monochrome, nine-pixel ASCII atlas derived from Anonymous Pro Regular, with a
five-pixel character advance. This is a provisional display substitute and does
not claim identical Monaco metrics or original SWC typography.

The generated **SWC HUD Bitmap** is compiled into the executable, so normal
builds and installations need neither SDL2_ttf nor a system font. The original
TTF, source hash and SIL Open Font License are in
[`third_party/anonymouspro`](../third_party/anonymouspro/README.md). The optional
[`bin/buildSwcHudFont.py`](../bin/buildSwcHudFont.py) generator uses Pillow and
records its rasterizer versions in the output. Release archives include the
font license. Bytes outside printable ASCII currently display as `?`.

Cockpit and weapon validation consists of original-instruction and resource inspection,
static source review, and `make modern modern-gui`. No game, GUI, or interactive
check was run for this pass; its appearance still needs user review in flight.

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

## Next support slices

1. Continue comparing shared AI/collision behavior with SWC instructions,
   including RNG bounds, elapsed-tick motion and later mission modes. The
   common 58 object definitions are recovered; Mac IDs 58..82 remain unsupported.
2. Compare SWC projection, rotation/scaling rasterization and the remaining
   320x240 HUD. Instrument sprites use their original offsets; centered ship
   quads agree with `rotate_Mac_shape2`, but exact sampling and clipping remain
   unverified.
3. Continue flight communications (voices/portraits), remaining HUD modes and
   post-debriefing progression. Landing and the first debriefing are connected;
   the demo disables its simulator.
4. Connect opening movies, the death cinematic and later scripted scenes;
   add save/load, audio-marker lip synchronization, effects, music and palette
   transitions. The first bar, barracks and briefing, AIFF speech and Hornet
   launch/landing playback, first debriefing and player/Spirit funerals are
   present; the animated end background and other scene entry points are pending.
