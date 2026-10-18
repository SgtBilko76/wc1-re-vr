# Compilation-unit order

MSVC emits functions in source order and the linker concatenates objects in
command-line order. Use `src/map` and `make sort`; uncertain core boundaries
remain provisional.

```text
0x00401000  game core and Win32 support (command-line objects)
0x00439C40  raster library (raster.obj, assembled), then killbrd.obj and gr.obj
0x004426A0  ix_log_printf (ixlog.obj, the last command-line object)
0x00442750  ix library members
0x004492C6  DirectSound and DirectDraw call thunks
0x004492E0  static debug CRT, first search pass (not reconstructed)
0x004614B0  RtlUnwind call thunk, second search pass
0x004614C0  LZO decoder, then the ix file module at 0x00461630
0x004617C0  CRT members reached through OLDNAMES, second search pass
```

## Proven `ix` order

| Range | Source | Library |
| --- | --- | --- |
| `0x00442750`–`0x00443B0F` | `src/ix/streamer.cpp` | streamer |
| `0x00443B10`–`0x004446FF` | `src/ix/thread.cpp` | streamer |
| `0x00444700`–`0x0044490F` | `src/ix/streamfile.cpp` | streamer |
| `0x00444910`–`0x0044518F` | `src/ix/dsp.cpp` | dsp |
| `0x00445190`–`0x00445F5F` | `src/ix/dsps.cpp` | dsp |
| `0x00445F60`–`0x0044679F` | `src/ix/mixer.cpp` | dsp |
| `0x004467A0`–`0x00446F73` | `src/ix/dspv.cpp` | dsp |
| `0x00446F74`–`0x004471FF` | `src/ix/dspmix.asm` | dsp |
| `0x00447200`–`0x00447A1F` | `src/ix/system.cpp` | sound |
| `0x00447A20`–`0x004486AF` | `src/ix/sound.cpp` | sound |
| `0x004486B0`–`0x004492C5` | `src/ix/sample.cpp` | sound |
| `0x004614C0`–`0x0046162F` | `src/ix/lzo1x.cpp` | lzo |
| `0x00461630`–`0x004617BF` | `src/ix/ixfile.cpp` | file |

Diagnostic source-file anchors establish function order, but the first
diagnostic in a file need not occur in its first function. Linker INT3 fill
establishes these object boundaries:

| Fill bytes | Following object |
| --- | --- |
| `0x00443B09`–`0x00443B0F` (7) | `thread.cpp`, beginning with `ix_streamer_thread_proc` |
| `0x004446F6`–`0x004446FF` (10) | the six `IxStreamFile` methods in `streamfile.cpp` |
| `0x00447A19`–`0x00447A1F` (7) | `sound.cpp`, beginning with `IxSound::ix_system_sound_init` |
| `0x004486A6`–`0x004486AF` (10) | `sample.cpp`, beginning with `IxSample::ix_sample_construct` |

The stream-file object's original filename is not present in the diagnostic
strings. `streamfile.cpp` describes its contents. The unaligned entries at
`0x00443DA6`, `0x00447CD8`, and `0x0044879C` are interior functions, not object
starts. Keeping them in their preceding groups lets the compiler and linker
produce the observed boundaries without explicit padding.

The sample object's final `RET 4` occupies `0x004492C3`–`0x004492C5`.
Its six functions span 3,094 bytes. The following three six-byte import
stubs belong to DirectSound and DirectDraw, followed by eight INT3 bytes
before the CRT at `0x004492E0`; they are not part of `sample.cpp`.
See `docs/EXPORT.md` for the report's handling of these import boundaries.

The retail image also has INT3 fill at `0x00445181`–`0x0044518F` before the
first initializer pair and at `0x00446795`–`0x0044679F` before the second pair.
The pairs begin `dsps.cpp` and `dspv.cpp`, respectively: a 21-byte `$E2`
calls a 16-byte `$E1`, followed immediately by the first ordinary function.
VC++ 4.20 emits these exact 37 bytes for each global array of non-aggregate
state records, even when no explicit constructor exists. Private fields with
the existing free functions as friends reproduce this property. This is a
matching declaration choice, not evidence of the original access-specifier
spelling. Public aggregate records omit the pair; explicit empty constructors
add calls and loop code absent from the executable.

The compiler also emits the two `.CRT$XCU` entries, at original addresses
`0x00465008` and `0x0046500C`, in stream-then-voice order. The handwritten
stand-ins formerly placed at the ends of `dsp.cpp` and `mixer.cpp` did not
register these initializers. `make verify-array-initializers` compares both
complete pairs with binary-comp and checks their registration order and base
relocations. Their original boundaries remain in the recovered map because
compiler-generated functions have no source annotations.

The voice-control group ends with `ix_dspv_recalc_mix`'s `RET` at
`0x00446F70`, followed by three INT3 bytes. The separately assembled mixer
starts at `0x00446F74` and ends at `0x004471FE`, followed by one INT3 byte.
Its shift encodings independently identify MASM output (see
`docs/COMPILER.md`); it is a separate member after `dspv.obj` in `dsp.lib`.

The DSP stream array starts at `0x00598138`. The voice array immediately
after it starts at `0x005981A8`, leaving 112 bytes: exactly two 56-byte
`IxStream` records. This agrees with the stream count of two set by
`ix_dsp_init`; the former eight-entry reconstruction overallocated 336 bytes.

DSP state also constrains data ownership. The initialized master-volume word
at `0x0047198C` contains `0xFFFF` and precedes the allocator callbacks at
`0x00471990` and `0x00471994` in `dsp.cpp`. Its pan table occupies
`0x00597D28`–`0x00598127`, immediately before the DSP tick and primary-buffer
pointer. The voice array, mixer critical section, voice count, and allocated
voice count occupy `0x005981A8`–`0x00598607` in `dspv.cpp`. These definitions,
along with `dsps.cpp`'s stream state, are included in the global-data audits.

The assert anchors name three source trees, `D:\Rnd\Prj\ix\Src\Streamer\`,
`D:\rnd\prj\ix\win95\dsp\` and `D:\Rnd\prj\ix\src\sound\`; each was a separate
library. `src/ix/ixlog.cpp` is the one ix unit linked as an object.

The logger ends at `0x00442738`, followed by seven compiler padding bytes.
The aligned entry at `0x00442740` returns a 32-bit one, then has ten bytes
of compiler padding before the first streamer function. It has no known
references or naming evidence; `ixlog.obj` is only its provisional owner.
The recovered boundary map keeps this unimplemented function separate from
the logger, whose own bytes match with the core optimizer profile.

## Link line

LINK 4.20 resolves symbols in the order they are first referenced: by each
command-line object in turn, then the entry point, then each library member in
the order it is pulled in. Library members are laid out in pull order, and a
library is searched again on later passes when new references appear. The CRT
member order, the call thunks and the import tables therefore record the link
line. The reference build reproduces all of them:

| Evidence in the original | Link-line choice |
| --- | --- |
| Import descriptors run KERNEL32, USER32, GDI32, ADVAPI32, WINMM, DSOUND, DDRAW | Import libraries in that order |
| DirectSound and DirectDraw thunks follow `sample.cpp`, DSOUND first | `dsound.lib`, `ddraw.lib` after the ix libraries |
| `wincrt0.obj` sits between `strncmp.obj` and the CRT members only ix needs | No `/ENTRY`; LINK adds the entry point after the objects |
| `_CrtDbgReport` and `strncmp`, needed first by `ix_log_printf`, precede `wincrt0.obj` | `ixlog.obj` on the command line, the other ix units in libraries |
| ix members appear streamer, thread, stream-file, dsp, dsps, mixer, dspv, system, sound, sample | Streamer, dsp and sound libraries searched in that order |
| LZO decoder after the second-pass `RtlUnwind` thunk, then INT3 padding and the file module, before the OLDNAMES members | The two in libraries of their own, decoder first, after `kernel32.lib` and before the other ix libraries |
| `strupr`, `xtoa`, `flength`, `cprintf`, `chdir` are pulled on the second pass | The game calls the old names, resolved through OLDNAMES |

With these choices every import table matches exactly, and every CRT member
sits at one constant offset from the original.

## Core placement windows

| Source | Range |
| --- | --- |
| `src/winmain.c` | `0x401000`–`0x402DFF` |
| `src/winsys.c` | `0x402E00`–`0x4034FF` |
| `src/mono.c` | `0x403500`–`0x403E4F` |
| `src/auto.c` | `0x403E50`–`0x40469F` |
| `src/eject.c` | `0x4046A0`–`0x404CCF` |
| `src/cmpgn.c` | `0x404CD0`–`0x40609F`; the face parser's exact owner remains provisional |
| `src/fl.c` | `0x4060A0`–`0x40770F` |
| `src/cutscene.c` | `0x407710`–`0x40975F` |
| `src/brains.c` | `0x409760`–`0x40B66F` |
| `src/mission.c` | `0x40B670`–`0x40D08F`; trailing joystick/navigation helpers remain provisional |
| `src/objdata.c` | Data only; original code ownership is unresolved |
| `src/nav.c` | `0x40D090`–`0x40E88F` |
| `src/nearheap.c` | `0x40E890`–`0x40EFDF` |
| `src/gameflow.c` | `0x40EFE0`–`0x40FA2F` |
| `src/title.c` | `0x40FA30`–`0x4102AF` |
| `src/joystick.c` | `0x4102B0`–`0x41073F` |
| `src/spc.c` | `0x410740`–`0x413A0F` |
| `src/cockpt.c` | `0x413A10`–`0x41807F` |
| `src/version.c` | Data only; original code ownership is unresolved |
| `src/geom.c` | `0x418080`–`0x41A9CF` |
| `src/barracks.c` | `0x41A9D0`–`0x41C50F` |
| `src/fade.c` | `0x41C510`–`0x41C75F` |
| `src/debug.cpp` | `0x41C760`–`0x41D0BF` |
| `src/mathutil.c` | `0x41D0C0`–`0x41D11F` |
| `src/disk.c` | `0x41D120`–`0x41DEDF` |
| `src/ship.c` | `0x41DEE0`–`0x4219BF` |
| `src/init.c` | `0x4219C0`–`0x42200F` |
| `src/logic.c` | `0x422010`–`0x42572F` |
| `src/pilot.cpp` | `0x425730`–`0x425C2F` |
| `src/trainsim.c` | `0x425C30`–`0x4272EF` |
| `src/main.c` | `0x4272F0`–`0x42A8EF`; trailing cursor/palette helpers remain provisional |
| `src/targ.c` | `0x42A8F0`–`0x42ACFF` |
| `src/select.c` | `0x42AD00`–`0x42AF9F` |
| `src/timer.c` | `0x42AFA0`–`0x42B04F`; internal ownership remains provisional |
| `src/pload.c` | `0x42B050`–`0x42B15F` |
| `src/sound.c` | `0x42B160`–`0x42B99F` |
| `src/launch.c` | `0x42B9A0`–`0x42C40F`; boundary with installation remains provisional |
| `src/install.c` | `0x42C410`–`0x42C7FF`; leading empty hook's exact owner remains provisional |
| `src/damage.c` | `0x42C800`–`0x42CDAF` |
| `src/view.c` | `0x42CDB0`–`0x42E31F` |
| `src/music.c` | `0x42E320`–`0x42EFBF` |
| `src/screen.c` | `0x42EFC0`–`0x42FA8F` |
| `src/ending.c` | `0x42FA90`–`0x43070F` |
| `src/input.c` | `0x430710`–`0x430BBF` |
| `src/comm.c` | `0x430BC0`–`0x4318EF` |
| `src/video.c` | `0x4318F0`–`0x431A0F` |
| `src/joycal.c` | `0x431A10`–`0x431EFF` |
| `src/dib.c` | `0x431F00`–`0x43390F` |
| `src/game.c` | `0x433910`–`0x433ABF` |
| `src/smart.c` | `0x433AC0`–`0x434CCF` |
| `src/mathfp.c` | `0x434CD0`–`0x4353FF` |
| `src/strdos.c` | `0x435400`–`0x4355EF` |
| `src/eventmgr.c` | `0x4355F0`–`0x436CFF` |
| `src/screens.c` | `0x436D00`–`0x439C3F` |
| `src/raster.asm` | `0x439C40`–`0x43F635` |
| `src/killbrd.c` | `0x43F640`–`0x44089F` |
| `src/gr.c` | `0x4408A0`–`0x44269F` |

The former system/main/HUD division split the main module's initialized
state from its first literal users. The main window now begins with the
process/memory helpers and includes flight input, banners, and the flight
loop. The Mac `targ` and `select` runs corroborate the following target and
weapon-selection groups. The trailing cursor/palette helpers before `targ`
and the Win32 timer helpers after `select` retain provisional ownership;
their positions alone do not prove their original object boundaries.

`LoadPaletteTripletsFile` at `0x00404610` closes `auto.c`. Its `"rb"`
literal is at `0x00465548`, immediately after the initialized autopilot
formation count at `0x00465544` and before the ejection tables at
`0x00465550`. The formation count has only local original references and is
private in `auto.c`. The packet error word at `0x00465460` similarly precedes
`mono.c`'s data-file error strings; it remains shared with packet consumers.
These initialized globals are kept in their owning units and included in
the distributed global audits.

`ejection_sequence` and `stranded_sequence` occupy `eject.c`. Its three
private animation tables and stranded-message pointer precede the message,
`"View Templates"`, and `"THE END"` literals. The complete section is 156
bytes; four linker alignment bytes precede the mouth-animation table at
`0x004655F0`. That initialized table cannot follow compiler literals within
one CL 4.20 object, establishing a boundary between the two data groups.
The old combined unit represented the literals as oversized named arrays
and owned eight bytes that should not have been part of those declarations.

The remaining campaign/conversation group starts with `ParseFaceAnimation`
at `0x00404CD0`, immediately before `ParseMouthAnimation`. The first parser
has no unit-local data reference, so that precise code boundary remains
provisional. The mouth parser indexes its private 26-word phoneme table
with `character - 'a'`, explaining the original displacement `0x0046552E`.
Its empty local-array initializer produces the literal at `0x00465624`;
the following two `"%d"`, two date, one time, and three conversation formats
remain separate use-site literals. The complete 135-byte campaign section
ends at `0x00465676`, with one linker alignment byte before maneuver data.

The beginning of the constant and initialized-data sections also constrains
ownership and qualifiers:

| Original data | Owner and storage |
| --- | --- |
| `0x00463008`–`0x004630D9` | `barracks.c`: the 21 constant menu regions passed directly by `BarracksScreen` |
| `0x004630E8`–`0x00463167` | `dib.c`: eight DirectDraw class/interface GUIDs, followed by its frame-rate constants |
| `0x004631C8`–`0x004631FF` | `gr.c`: constant registry-key and `Cheater` value-name arrays used by `ReadCheaterFlagFromRegistry` |
| `0x00465040`–`0x00465057` | `winmain.c`: two initialized hazard counters and two writable eight-byte hazard tables |
| `0x00465058`–`0x004650B3` | `winmain.c`: window/audio flags, heap counters, class/title arrays, and message-pump state; two intervening words remain unidentified |
| `0x004650B4`–`0x0046535A` | `winmain.c`: configuration, window, memory, joystick, and heap-corruption literals |
| `0x0046535C`–`0x0046545E` | `winsys.c`: game-data paths, CD paths, and disc prompts |
| `0x00465460`–`0x00465540` | `mono.c`: packet-error word, file diagnostics, and MONODEBG literals |
| `0x00465544`–`0x0046554A` | `auto.c`: formation count and palette-file mode literal |
| `0x00465550`–`0x004655EB` | `eject.c`: animation tables, stranded-message pointer, and three literals |
| `0x004655F0`–`0x00465676` | `cmpgn.c`: phoneme frames and conversation-format literals |
| `0x00465678`–`0x00465763` | `fl.c`: maneuver reroll weights and handler pointers |
| `0x00465768`–`0x00465C78` | `cutscene.c`: scramble, landing, and funeral tables followed by their literals and the animation-demo diagnostic |
| `0x00465C7C`–`0x00465C7D` | `brains.c`: enemy-sighting state |
| `0x00465C80`–`0x0046602A` | `mission.c`: debriefing personality, formation state, packet references, formation/timing tables, and pilot-speech diagnostic |
| `0x00466030`–`0x0046865C` | `objdata.c`: animation streams, object definitions, weapon coordinates, and object-name literals; original object name and associated code remain unknown |
| `0x00468660`–`0x004688BF` | `nav.c`: map state, objective styles, mission-name pointers, and the following navigation literals |
| `0x004688C0`–`0x004688CB` | `nearheap.c`: near-heap state and descriptor limit |
| `0x004688CC`–`0x0046890B` | `gameflow.c`: campaign state and the following viewport-disposal diagnostic; the helper's unit ownership remains provisional |
| `0x00468910`–`0x00468F01` | `title.c`: opening-text pointer, Secret Missions message, credits, title-menu and resource records, and the following cinematic literals |
| `0x00468F04`–`0x00468FF1` | `joystick.c`: five interactive calibration prompts and three separate calibration-file literals |
| `0x00468FF4`–`0x00468FFF` | `spc.c`: capital-ship view distance, landing authorization, and alternate chase-view state |
| `0x00469000`–`0x004693B0` | `cockpt.c`: cockpit state, shared weapon-name array, tables, and following message literals; eight zero bytes at `0x00469010` remain unrecovered |
| `0x004693B4`–`0x004693C0` | `version.c`: version pointer and its following literal; original object name and associated code remain unknown |
| `0x004693C8`–`0x0046963F` | `barracks.c`: bunk origins, menu pointers, modal bounds/state, campaign file numbers, and the following string literals |
| `0x00469640`–`0x00469641` | `fade.c`: palette-transition initialization flag |
| `0x00469644`–`0x00469665` | `debug.cpp`: console count, keyboard state/hook, and font, spinner, and newline literals |
| `0x00469668`–`0x00469872` | `disk.c`: graphics-mode and error-message pointers, disk marker, prompt settings, and the complete following literal pool |
| `0x00469878`–`0x00469995` | `ship.c`: damage-system and debris tables, gun refire delays, and the following four HUD literals |
| `0x00469998`–`0x00469BBD` | `init.c`: graphics defaults, the shared low-memory message, reservation tables, and startup literals |
| `0x00469BC0`–`0x00469D7E` | `logic.c`: resource descriptors, scene state, and the scene text-format literal |
| `0x00469D80`–`0x00469D9C` | `pilot.cpp`: transition subtitle, empty answer input, and answer label; the following word at `0x00469DA0` remains unidentified |
| `0x00469DA8`–`0x00469FA6` | `trainsim.c`: panel state, high-score name pointers, menu regions, simulator state, and the following UI literals |
| `0x00469FA8`–`0x0046A3A1` | `main.c`: legacy overlay setting, game/input state, and process, startup, flight, HUD, and score literals |
| `0x0046A3A4`–`0x0046A436` | `pload.c`: compressed-data and non-pushed-buffer diagnostics; trailing audio helpers retain provisional ownership |
| `0x0046A748`–`0x0046A8CD` | `damage.c`: cockpit damage tables and the following report literals |
| `0x0046A8D0`–`0x0046A9F0` | `view.c`: scripted-view state, constellation tables, packet settings, target armor rectangles, and the following target/display literals |
| `0x0046AD88`–`0x0046AF50` | `ending.c`: allocation tag, campaign-ending pointers and projectile origins, subtitle literals, and four separate text formats |
| `0x0046AF54`–`0x0046AF59` | `input.c`: double-click interval and menu-pointer speed |
| `0x0046AF60`–`0x0046B167` | `comm.c`: menu/video state, ace and menu-label pointers, and the complete communication literal pool |
| `0x0046B168`–`0x0046B171` | `video.c`: video mode and the palette-allocation diagnostic literals |
| `0x0046B178`–`0x0046B1A1` | `joycal.c`: graphics-mode table, two unused driver names, and both calibration-file literals |

The former large brains window is divided into flight maneuvers, cutscenes,
NPC intelligence, and mission setup. The Mac `fl` symbol run corroborates
the maneuver group, including the dispatcher using both private tables.
All 47 dispatch pointers must target the correct one of 42 named function
entries; matching another handler or an interior byte is rejected. Its
236-byte initialized section has four alignment bytes before the scene tables.

All 19 cutscene declarations have only local original references. The four
landing comments are now pointer-initializer literals, followed by the
subtitle formats, commands, and animation-demo diagnostic. Their 289-byte
pool ends at `0x00465C78`, establishing the boundary before the AI state.
The seven funeral particle origins end at `0x00465B33`. Four compiler
alignment bytes precede the scene-selection array at `0x00465B38`; the
original lookup uses `series * 2 + 0x00465B36`, or `array[series - 1]`.
Thirteen selections and two alignment bytes reproduce the following flag
at `0x00465B54`. This declaration matches the observed bytes; the binary
does not distinguish its final padding from an additional zero element.
The former 15-element declaration started inside the preceding alignment.
The SDL build preserves the zero results for series 0 and 14 explicitly.
The complete 1,297-byte section and all eleven stored pointers match.

The surviving WINGLEADER `BRAINS.C` and Mac `brain` run corroborate
`cruise_home` through `FF_missile_intelligence`. Its enemy-sighting word
is shared with mission setup. The following 939-byte mission section has
the shared debriefing personality and packet-reference table, plus private
formation state and formation/timing arrays. SDL adapters retain access to
the last three through SDL-only declarations. The 27-byte pilot-speech
diagnostic ends before object definitions, with five linker alignment bytes.
The division between the AI word and mission definitions remains provisional:
no intervening literal proves that internal boundary. The joystick helpers
beginning at `0x0040CAA0` and navigation helpers at `0x0040CB60` also retain
provisional ownership in `mission.c`. Its final `Build_objective_list` extends
through `0x0040D08F`; the old window boundary at `0x0040D000` cut through it.
Its complete 448-byte span now matches, including the signed sentinel index
and source-type dispatch. This validates the function span, not the provisional
internal unit boundaries. The conversation group's final `CloseLook` likewise
matches all 704 bytes through `0x0040609F`, preserving its four-byte local frame.

The object-data block begins after the pilot-speech diagnostic at
`0x00466010` and ends with its name literals before navigation state at
`0x00468660`. Initialized declarations cannot follow compiler literals in
one CL 4.20 object, so both edges establish data boundaries. `objdata.c`
describes the recovered contents; the original filename and any associated
functions remain unknown. It is linked after the provisional mission group
and before navigation without assigning it invented function ownership.

Its 21 private animation arrays occupy 1,064 bytes, followed by 58 packed
135-byte object records, two compiler alignment bytes, and the child-offset
and weapon-coordinate tables. The 393-byte literal pool begins at
`0x004684D4`. Each non-null display-name initializer owns its literal:
30 nonempty names and 26 distinct empty names. The two all-zero object records
have null names. The former flattened string array included the three
linker alignment bytes before navigation; ordinary separate literals now
reproduce that layout without explicit padding. All 80 relocations stay
inside the block: 56 name pointers and 24 animation pointers. The strict
unit gate checks all 9,773 bytes and all 25 declaration offsets, including
interior animation targets and the separate empty-name addresses.

The object-type records retain their 135-byte stride with one signed
32-bit lifetime/fuel-capacity field at offset `0x10`. The two words formerly
declared there are one value: the first four ship records contain 200000,
250000, 280000, and 300000. The full-width fuel division and projectile
lifetime loads independently establish this layout. Combining the initializer
words preserves every table byte; no field follows at offset `0x12`.
The next field, damage capacity, remains at `0x14`.

The navigation unit's 360-byte literal pool follows its map state and pointer
tables, ending immediately before the initialized near-heap state at
`0x004688C0`. This establishes a boundary before `MergeAdjacentNearHeapBlocks`
at `0x0040E890`: those initialized definitions cannot follow function literals
in one CL 4.20 object. The 34 former named navigation string arrays are now
single-use literals, including the three separate `"err"` mission names.
Six initialized variables and the two uninitialized navigation arrays have
only local original references. The SDL mission-name consumer retains access
through the SDL-only header.

The near-heap routines occupy the next nine-function run, followed by campaign
scoring and mission flow. Their internal division into `nearheap.c` and
`gameflow.c` follows their distinct state and call groups; no intervening
literal pool proves that particular boundary. The adjacent `free_viewport`
helper remains in `gameflow.c` provisionally. Its diagnostic ends before the
title's initialized data at `0x00468910`, proving another object boundary.
The unreferenced constant helper at `0x0040FA30` remains with title provisionally.

The title data includes the previously omitted 279-byte Secret Missions
message at `0x00468918`, including its terminator. No surviving code or data
relocation targets it. A writable named character array reproduces those
bytes and the single alignment byte before the credit count. Its descriptive
name does not claim recovery of the original identifier. The opening-text
pointer and all nineteen non-null credit pointers target the following
1,062-byte literal pool, which retains the original spelling and newlines.

The interactive calibration prompts follow the title literals at `0x00468F04`.
The three `"j.cal"` literals at `0x00468FDC`, `0x00468FE4`, and `0x00468FEC`
belong respectively to calibration failure, file opening, and write failure.
They remain separate use-site literals. Initialized space state follows at
`0x00468FF4`, proving an object boundary after the calibration literal pool.
The three calibration/button-wait functions now occupy `joystick.c`; its
238-byte `.data` section has no named globals. The title/joystick code boundary
follows their distinct behavior and call groups; contiguous literal storage
alone does not prove that particular division.

The following space state occupies twelve initialized bytes. The alternate
chase-view flag has only local original references and is private in the native
build; the SDL camera adapter retains access through the SDL-only header.
`SetFleetOverviewView` remains at the start of `spc.c` provisionally: it has no
unit-local data to establish its exact original ownership. The Mac `spc`
symbol run identifies `rotate_eye_to_goal` through `object_intelligence`;
the next recovered function, `EmitTextString` at `0x00413A10`, begins the
cockpit window. These windows no longer overlap the surviving space functions.

The main camera routine, `set_eye_direction_and_position`, now reproduces its
3,264-byte span and 60-byte local frame exactly. The fleet overview retains
its 752-byte span and 36-byte frame, with only the maximum-range comparison
and branch differing. Its native compiler overlays an uninitialized range
local with an initialized vector component; explicit initialization had
changed both that read and the vector placement. The SDL build preserves the
observed fallback value explicitly. See `docs/PATTERNS.md` for the stack
evidence and the camera branch ordering.

Within this window, `start_dust`, `update_star_field`, and
`house_keep_objects` reproduce their complete 288-, 784-, and 1,008-byte
aligned spans. The housekeeping dispatch changes register allocation in
later routines in the same object. Retaining the animation's promoted index
and scale-command switch restores the collision routine's original 92-byte
frame; its corrected sound arguments, collision stores, and mass arithmetic
leave only two exchanged initial loads in the 1,968-byte span. All nineteen
space routines retain their original aligned sizes. The animation's remaining
scale-arithmetic differences are recorded in `docs/PATTERNS.md`; recovering
the preceding movement dispatch restored its two separate stores. The
movement and AI dispatchers now match all instruction mnemonics in their
816- and 400-byte spans, with four and one differing encodings respectively.
A separate afterburner-speed local made movement exact but enlarged the
collision routine; using the existing velocity-magnitude local preserves all
original spans. No unit boundary or linker-order change is inferred from
these compiler effects.

The unit-data gate checks the complete reconstructed `.data` sections:
system paths (259 bytes), file/developer diagnostics (225), autopilot (7),
ejection (156), conversation/campaign (135), maneuvers (236), cutscenes
(1,297), AI (2), mission setup (939), object data (9,773), navigation (608),
near-heap (12), campaign flow (64), title (1,522), joystick (238), and space
state (12). Together with the verified 723-byte startup tail, these form a
16,248-byte run with 187 pointer relocations and 40 inter-object alignment
bytes. The six referenced colour
variables and the two title resource-record pointers must resolve to their
original globals and field offsets. This proves the data layout independently
of the provisional internal code boundaries.

The cockpit's `"None"` weapon name at `0x00469080` is a named array before
the objective-range state. Both missing-weapon branches in `show_weapon_disp`
reference that same array; the former empty literals changed the text and
added extra storage. The seven bytes after the target-bracket visibility
flag are compiler alignment before the message-origin array. A separately
declared padding array added eight bytes and is removed.

The 293-byte cockpit literal pool begins at `0x0046928C` with the writable
objective-message initializer and missile-lock message. Its remaining labels
are use-site literals. The objective pointer is written through by
`flag_reached`, and the original PE keeps this literal writable. The SDL
adapter instead retains its existing writable array in the SDL cockpit unit.
The normal-arrival branch must precede the waiting branch in source to emit
`"Already Visited"`, `"Objective Reached"`, and `"Wait for %s"` in that order,
even though CL schedules the waiting path first in machine code. The final
empty literal supplies the byte load in `print_message_text`.

Twenty-two initialized cockpit declarations have only local original
references and now have private storage; shared resource pointers, the pending
message, and the navigation-pointer object retain external visibility. The
unit-data gate checks a 16-byte prefix and a 921-byte tail, including every
declared offset and both string pointers. The current COFF section has 937
bytes. The unexplained eight zero bytes at `0x00469010`–`0x00469017` prevent a
complete 945-byte cockpit match; their declaration and alignment split are
unknown, so no storage is invented for them.

The version pointer follows the cockpit literal pool at `0x004693B4`, proving
another object boundary: CL emits initialized declarations before literals.
A pointer initialized directly with `"1.03F-95"` reproduces the complete
13-byte block. `version.c` isolates that data without assigning any geometry
functions to it. Its original filename and whether it shared an object with
some of those functions remain unresolved. Three alignment bytes separate
the cockpit tail from version data, and seven precede the barracks tables.
The tail, version, and eight following units through the pilot literals form
a checked 3,461-byte run with 62 pointer relocations and 24 alignment bytes.
A separate
28-byte check joins space state to the cockpit prefix. Neither bridges the
unrecovered cockpit gap or establishes absolute placement.

The GUID sequence includes `CLSID_DirectDrawClipper`, absent from the bundled
DirectDraw header, between `CLSID_DirectDraw` and `IID_IDirectDraw`. The bytes
in the retail image establish its value and position. Unused SDK identifiers
are real original data, so their definitions must survive the reconstruction.

The window class name at `0x00465088` is a writable character array used for
both `WNDCLASSA` names and `CreateWindowExA`'s class argument. Its separate
title array is at `0x00465098`. Both precede the configuration-file literals,
so they are declarations rather than literals emitted at the call sites.
The surrounding zero-valued flags and heap counters need explicit zero
initializers: tentative C definitions put them in the linker's common storage
instead of this initialized-data range. The words at `0x00465078` (one) and
`0x00465084` (zero) still need identification; do not replace them with padding
declarations.

Three partial-section checks cover the 56-byte prefix, eight-byte middle,
and 723-byte tail around those missing words. Their rebuilt owner contains
787 initialized bytes, versus the original 795. The tail includes the
679-byte literal pool. `GetJoystickDevCaps` owns its diagnostic as a use-site
literal at `0x004652DC`, between the memory report and corruption-severity
strings. `WinMain` passes an empty literal at `0x00465218` to `main`; the
former `"Vj"` spelling had two incorrect nonzero bytes that only the strict
pool check exposed. The checked tail and following sixteen data units are
currently eight bytes early. This is the exact combined deficit of the two
unidentified words, not evidence that their source declarations are known.

The same storage distinction applies to the zero-valued definitions still in
`globals.c`. The 592-byte packet-reference table at `0x00465C88`, the input
sample at `0x0046A020`, and the DirectDraw pointers at `0x0046B1A4` are among
87 definitions restored to initialized data. They retain their provisional
owner until the original compilation units are established.

The default modal-text bounds at `0x00469440`–`0x00469447` form one
`ShortRect` initialized to `{24, 40, 296, 60}`. All five calls to
`InitializeModalTextPanel` pass a rectangle by value, and the initializer
copies those eight bytes as a unit. The bounds and following panel pointer
belong to `barracks.c`, together with the preceding menu tables and following
campaign file-number arrays. Only the campaign file-number arrays have users
outside this unit.

The menu-label literals follow these definitions at `0x00469468`–`0x004694E3`.
The modal renderer references `"%P"` at `0x004694E4`, immediately before
`CreateEmptySaveGameFile`'s `"SAVEGAME.WLD"` at `0x004694E8`. This literal order
places the modal-panel routines and `AnySavedGames` at the beginning of
`barracks.c`, moving its boundary to `0x0041A9D0`. The objects remain adjacent
in `SRCS_ORDERED_CORE`; `get_right_shape` closes `geom.c`.

Writing the six menu labels as literals in their pointer-table initializers
reproduces their original four-byte spacing. Separate named character arrays
introduced eight extra alignment bytes. Both `"Save this campaign  "` literals
must remain distinct. With these declarations, `barracks.obj` contributes
exactly 632 initialized-data bytes through the airlock message. The entire
block matches through binary-comp with one address shift, including its six
internal pointers, literal order, and padding; `make verify-barracks-data`
enforces this. Its absolute address remains displaced until preceding units'
data ownership is recovered.

The palette initialization flag follows that literal pool at `0x00469640`.
Named declarations precede compiler literals in VC4.20, including function-static
declarations, so the flag cannot be another barracks declaration. `fade.c`
begins at its sole user, `StepPaletteTransition`. The adjacent `DosStrcat`
entry remains in this range provisionally; its exact original ownership is
not established. Both retain their observed callee-cleanup conventions. The
five palette scratch variables are private to this unit, but their common-symbol
storage still differs from the original uninitialized-data layout.

The flag occupies two initialized bytes, followed by two linker alignment bytes.
The debug-console unit then has four initialized words and three literals:
`"Courier"`, the spinner sequence, and `"\n"`. Each string has one use site.
Named arrays reproduced their contents but required eight-byte object alignment,
placing the console count four bytes too late. Literals restore four-byte
alignment and preserve the exact constructor instructions, including the original
string copy. The complete `debug.obj` data section is 34 bytes; two alignment
bytes follow before the disk tables. Both keyboard words have external users;
the count and hook are private declarations.

The complete initialized-data run from `0x004693C8` through `0x00469D7E` now
matches with one address shift across barracks, fade, debug, disk, ship, init,
and logic. `verify-unit-data` checks all 2,487 bytes, the seven unit positions,
59 relocated pointers, and 13 inter-object alignment bytes. It does not insert
padding or claim the absolute addresses are restored. The training-simulator
block remains separate because ownership of the intervening scene data is open.

The disk unit begins with the graphics-mode pointer table. The next three
pointers at `0x00469678`, `0x0046967C`, and `0x00469680` are loaded directly
by `ReportPacketLoadError`; they point to the read-operation label,
allocation-operation label, and error format. Direct literals in that
routine omitted both the pointer storage and the original loads. A further
unreferenced sequence at `0x00469698` contains pointers to `"(OVR)"` and
`""`, then zero. Its two HIGHLOW relocations and null word are preserved as
`DAT_00469698[3]`; no purpose is assigned to it without a known user.

The resulting `disk.obj` initialized-data section is exactly 523 bytes.
All values, nine pointer relocations, and the 459-byte literal pool match
with one address shift. Five linker padding bytes precede the ship table at
`0x00469878`. `make verify-unit-data` checks the barracks, debug, and disk
blocks, their object sizes, and every global's relative position. Absolute
placement remains dependent on the preceding units' data recovery.

The ship unit's initialized data begins with the 50-word damage-system
table at `0x00469878`. Four compiler alignment bytes precede the existing
explosion-debris table at `0x004698E0`; the hit-debris types and gun refire
delays follow at `0x00469950` and `0x0046995C`. All four tables are private
to `ship.c`. The four HUD strings follow in function-use order at
`0x00469960`, `0x00469974`, `0x00469984`, and `0x0046998C`. Writing these
as literals at their use sites reproduces the original four-byte spacing;
separate character arrays introduced extra alignment. The complete
286-byte `ship.obj` data section matches literally through binary-comp,
including its padding, with no relocations. Two linker padding bytes
precede the next unit's scene-animation flag at `0x00469998`.
`make verify-unit-data` covers this block as well, including the private
explosion table that is absent from the linker's public-symbol list.

The following startup block establishes a separate initialization unit.
Its declarations end with the expanded-memory reservation table at
`0x00469AB0`; the literals from `LoadGamePaletteFile`, `EMStartUp`, and
`LoadOriginFxDrivers` follow at `0x00469ABC`–`0x00469BBD`. The resource
descriptor tables begin after this literal pool, at `0x00469BC0`.
A controlled VC4.20 core-profile probe confirms that named declarations,
including function-static arrays, precede the unit's literal pool even
when they appear after functions in source. These two data groups therefore
require separate objects. `init.c` contains the twelve startup functions
through `GetFxDriverStatus`; the following ace and mission routines remain
in `logic.c`. Interior boundaries within `logic.c` remain provisional.

The two low-memory branches both push `0x00469A28`. This 100-byte message
precedes the reservation tables, so it is a shared writable array rather
than a pair of compiler literals. `szInsufficientMemory` restores that
storage and both references without string pooling. The complete
550-byte initialized-data section in `init.obj` matches literally, including
the spacing between colour bytes and every startup string.

The next 447-byte block contains five resource tables, their active flags,
constellation indices, scene-file numbers, a scene-wait flag, and the final
`"%X%Y%F%s%P"` literal used by `PlaySceneAnimation`. Its 44 relocated pointers
address shape fields and pointer slots in other units. `verify-unit-data`
checks each against the corresponding rebuilt global and original field
offset; null contents at the destination cannot stand in for the correct
address. All other bytes and relative global positions match exactly.
The initialization and logic checks also enforce each object's complete
data size. Two alignment bytes separate the blocks, and one follows the
scene-format literal before the pilot unit's data at `0x00469D80`.

`ShowMeanwhileTransition` and `PromptForAnswerText` own the next three
literals: `"Meanwhile..."`, the empty answer input, and `"Answer: "`.
Their complete 29-byte section matches, including the empty literal's byte
load. Named arrays previously overallocated these literals and placed them
in the temporary globals unit. Three alignment bytes follow the label;
the unreferenced word `10` at `0x00469DA0` and the four bytes after it remain
outside the recovered section.

The training-simulator block starts at `0x00469DA8`, after the scene
literals and the Win32 console wrappers. Its viewport, bounds, shared title
array, six high-score name pointers, menu regions, and three simulator-state
variables precede the literal pool at `0x00469E38`. The six names are separate
literals in the pointer initializer, not a manually concatenated array.
Empty pilot input, the text-flush token, and every following UI string are
literals at their respective call sites. The title remains a named array:
two calls use the same address before the pointer table.

`trainsim.c` now contains `EraseTextContextBackground` through `RunTrainSim`,
ending before the process-exit routines. These game functions use the C core
profile; the console allocation wrapper and its `/GX` construction cleanup
remain in `pilot.cpp`. That file's internal C/C++ boundary is still provisional.
The complete 511-byte simulator data section and all six name-pointer targets
match with one address shift. The preceding unreferenced word `10` at
`0x00469DA0` remains unidentified.

The next word, `0x1370` at `0x00469FA8`, has stronger evidence. The surviving
WINGLEADER main-source screenshot (`../squadronsourcecode.png`) declares
`extern unsigned _ovrbuffer = 0x1370`, immediately before game mode/status
and the two frame counters. The Win32 bytes preserve that value and the
same neighboring state, despite having no relocation that references the
word. It is a retained legacy overlay setting, not padding. Its original
external identifier is preserved in `main.c`; the earlier analysis's claim
that it had no Win32 counterpart was incorrect.

The 29 initialized declarations occupy 184 bytes. The private volume table,
memory-display flag, last input sample, mouse thresholds, button timestamps,
and input latches have original users only within this main group. Five
remain available to SDL adapters through SDL-only declarations. Shared game
state stays declared in `globals.h`. The following 834-byte literal pool
starts with the process helpers' diagnostics, continues through `main`,
and ends with the HUD score messages. All 1,018 bytes and all declaration
offsets match; no pointer relocations occur in this data block.

The unreferenced diagnostic at `0x0046A15C` describes an `int`/pointer size
check. A controlled VC4.20 probe confirms that `sizeof(int) != sizeof(short *)`
is folded away while its diagnostic literal survives in initialized data.
This source form, inferred from the text, restores the original 36-byte
literal without adding runtime code. `ReportOutOfMemoryAndExit` passes the
shared message at `0x00469A28`; the former shortened use-site message added
incorrect storage. The buffer tags are separate literals, and the two
volume-adjustment cases and volume toggles preserve their original literal
order without string pooling.

`PacketLoad`'s compressed-data case precedes its default case in source,
although CL schedules the default code first. This emits both fatal
diagnostics before the non-pushed-buffer diagnostic, matching all 147 bytes
of its literal section. The following audio routines share state with the
playback and registry routines and now reside together in `sound.c`.

The audio unit starts at `InitializeAudioSystem`, `0x0042B160`, and ends
after `SaveVolumeSettingsToRegistry`. Its five initialized state variables
occupy `0x0046A438`–`0x0046A44B`. The three references to each registry
value name share the arrays at `0x0046A450` and `0x0046A460`; writing a
separate literal at each call would incorrectly duplicate their storage.
By contrast, the two full registry-key literals are distinct. The following
222-byte literal pool preserves both `SNOW` copies, the mismatched `sfx23`
diagnostic, and the SFX-volume message's trailing newline. All 274 bytes,
seven declaration offsets, and the separate eight-byte private `.bss` match.

Six linker alignment bytes separate that literal pool from the launch
tables at `0x0046A550`. Their position proves a new compilation unit before
`DrawLaunchDoorFrame`. The five private tables occupy 172 bytes. The first
table changes screen Y, so it is named `asCarrierLaunchApproachDeltaY`.
The last two tables begin at `0x0046A5C0` and `0x0046A5E0`, each containing
14 words. The previous 16-word declarations included four preceding alignment
bytes apiece. The fighter loop uses frames 9 through 22 and the displacement
`0x0046A5AE`: indexing the real table by `frame - 9` preserves that operand.
The scripted-view call passes `0x0046A5E0` directly. Normal compiler alignment
supplies both gaps; neither belongs to an array.

The configuration and installation literals occupy the next 325 bytes,
`0x0046A5FC`–`0x0046A740`. The disk prompt is a use-site literal, and each
installation-failure message remains separate. These routines and the
launch routines use the C profile. Their internal boundary is provisional:
the data establishes separation from the preceding audio and following
damage units but does not independently distinguish launch from installation.
The empty shutdown hook remains with installation provisionally.

Simulator, main, loader, audio, launch, installation, damage, and view data
form one checked 3,145-byte run, with 20 stored pointers and 19 linker alignment
bytes. That run is currently 24 bytes early: eight bytes each remain missing
in startup state, cockpit state, and the gap before the simulator.

The maneuver proximity threshold at `0x00475E78` and reroll byte at
`0x00475E7C` are private to `fl.c`. All references occur in the threshold
setter, `Mveer_away`, and `perform_maneuver`. Their five-byte `.bss` contains
two compiler alignment bytes between the declarations, followed by three
linker alignment bytes before navigation. The threshold is named for its
use in the proximity-based boost and maneuver-reset decisions.

The navigation label and exclusion arrays at `0x00475E80` and `0x00475F48`
have references only in `nav.c`. Their existing 20 packed ten-byte label
records and 21 eight-byte rectangles now occupy a private 368-byte `.bss`
section, with the rectangle array at offset `0xC8`. The separate 1,024-byte
cockpit cursor background at `0x00475FF0` is used only by
`overlay_head_up_display` and `RestoreTransientCockpitGraphics`; it now
occupies `cockpt.c`'s `.bss`. Together with the preceding maneuver state and
following console pointer, these form a verified 1,404-byte run with three
inter-object alignment bytes.

Three more uninitialized-data blocks reside in their owners:
`pDebugOverlay` at `0x004763F0` belongs to the console wrappers in `pilot.cpp`,
`pSnowStaticWave` and `pSnowStaticSound` occupy `0x0047654C`–`0x00476553`
in `sound.c`, and `nDisplayedComponentDamage` at `0x00476554` belongs to
`damage.c`. Every reference to these four variables comes from its respective
owner. VC++ 4.20 emits four and eight bytes of `.bss` in the C++ owners,
with the two private sound globals at offsets 0 and 4. The private damage variable
uses C `static` storage to produce its four-byte `.bss` section.
Their former tentative definitions in `globals.c` became independently
ordered linker common symbols. The unit-data gate now verifies their owner,
relative symbol offsets, complete object `.bss` sizes, and zero-filled bytes.
Their absolute addresses and the intervening uninitialized storage remain
unrecovered.

Declaration order alone does not recover the remaining private C storage.
A trial moving the twelve flight-timing variables into `main.c` produced
an 88-byte `.bss`, with the simulation timestamp first, instead of the
observed 84-byte run beginning with the total-tick count at `0x004764F8`.
Reversing all twelve declarations left their emitted order unchanged.
Making the two MONODEBG variables private to `mono.c` likewise emitted the
device handle before the installed flag, opposite to their original order
at `0x00475E70` and `0x00475E74`; renaming the handle to describe the debug
channel did not change that result. Removing their shared declarations also
disturbed code generation elsewhere: the timing trial changed 200 function
comparisons, and the MONODEBG trial lost fourteen exact comparisons overall.
These trials are reverted. Ownership evidence is not sufficient to claim a
matching storage block without checking its emitted offsets and the full
function report.
Moving only the timing definitions into `main.c`, retaining their external
common storage and shared declarations, also fails to recover the cached
profiling reads. It merely relocates the common symbols; it does not establish
the original private 84-byte run. That narrower ownership trial is reverted.

The 200-byte `szSfxWavePath` buffer at `0x00476558` now resides in `music.c`.
Its two original references both occur in `PlaySfxWaveFileByNumber`: `sprintf`
formats the filename into it, then `playWAVE` receives the same buffer. Its
end is immediately before the DirectDraw frame-rate text at `0x00476620`.
The private uninitialized definition produces a 200-byte `.bss` section.
The unit-data gate checks its owner and size, and the sound pointers, damage
state, and this buffer as one 212-byte run with no intervening alignment bytes.

The cursor save buffer at `0x004865A8` similarly belongs to `eventmgr.c`.
Its three original references occur in the mouse-background capture and
restore routines in that unit. The 4,096-byte buffer ends immediately before
the graphics workspace at `0x004875A8`; it is distinct from the cockpit's
1,024-byte mouse-background buffer at `0x00475FF0`. Its private definition
produces the original `.bss` storage and size. The gate verifies it together
with the recovered graphics prefix as a 68,120-byte run, with no inter-object
padding. These buffers remain shared only in the SDL build, whose audio and
renderer adapters use them through declarations in `wc1sdl.h`.

The graphics workspace begins at `0x004875A8` with a 20-byte `RasterSurface`,
four alignment bytes, and the 64,000-byte transformation buffer at
`0x004875C0`. Their identified direct references are confined to `gr.c`.
Private static definitions there reproduce this 64,024-byte prefix as genuine
COFF `.bss`, with the buffer at offset `0x18`. The partial-section gate checks
both owner symbols, their offsets, uninitialized storage flags, and all
loader-visible bytes. Their absolute placement and the rest of the graphics
workspace remain unrecovered.

A trial moving the other five private graphics/streaming variables together
put the stream path before the RLE buffer and the raster clip before the
raster surface, contrary to the original. Those definitions remain in
temporary storage; the matching prefix does not establish the later layout.

The damage-display unit starts with `show_damage_disp` at `0x0042C800`.
Its initialized tables start at `0x0046A748`, after the installation
literal pool ending at `0x0046A740` and seven linker alignment bytes.
CL places a unit's initialized definitions before its literals, so these
tables require a separate owner from the preceding installation routines.
The display positions, component-name pointers, frame indices, and severity
pointers precede the literal pool at `0x0046A7C4`. The component names and
severity names are individual literals in their pointer initializers; named
character arrays impose different alignment. The two status-format literals
at `0x0046A8B0` and `0x0046A8C0` remain separate use-site strings.

`verify-unit-data` verifies the entire 390-byte damage `.data` section,
including all 14 pointers, and its separate four-byte `.bss`. The three
private tables and private damage state have no declarations in the shared
header. `WC1.map` gains their actual linked addresses from COFF symbols and
relocations, as described in `docs/EXPORT.md`. Two alignment bytes follow the
last literal before the scripted-view state at `0x0046A8D0`.
The constant-return routine at `0x0042CDA0` provisionally closes this unit.

The following `parse_view_script` at `0x0042CDB0` starts `view.c`, whose
initialized state, constellation tables, packet settings, and target armor
rectangles occupy `0x0046A8D0`–`0x0046A947`. The target/display literal pool
follows immediately and ends with the second `"not MCGA"` at `0x0046A9F0`.
Music's initialized state starts at `0x0046A9F8`, after those literals;
CL's definitions-before-literals layout therefore requires a separate unit
before `FadeMusic` at `0x0042E320`. The packet and viewport helpers remain in
the contiguous view unit; no internal source-file boundary is established.

`verify-unit-data` checks the complete 289-byte view `.data` section and all
nine variable offsets. It also checks damage and view as one 681-byte run,
including the two alignment bytes between them. Seven view variables have
only local references in the recovered original code and are private;
the two scripted-view controls remain shared. The map supplement now resolves
these private symbols from `view.obj` alongside the damage symbols.
The BSS used by the view functions remains in temporary storage until its
original declaration order is recovered.

The music unit owns the initialized state beginning at `0x0046A9F8` and
`aabSoundEffectDescriptors` at `0x0046AA50`. The descriptor table is 37
records of eight bytes, with the last byte increasing from 0 through 36.
The sole surviving caller, `TheEndFireWorks`, passes record 36 at
`0x0046AB70`; the earlier seven-byte definition omitted its final index byte
and all preceding records. The table remains writable, as in the original.
Five state variables have references only within music and are private;
the other state and the table remain shared. The map supplement resolves
private music symbols from COFF in the same way as damage and view.

This does not complete the music unit's layout. Its rebuilt `.data` is 768
bytes versus the original 832-byte run ending before `0x0046AD38`. Six
zero-valued slots at `0x0046AA00`, `0x0046AA08`, `0x0046AA1C`, `0x0046AA20`,
`0x0046AA28`, and `0x0046AA44` lack identified declarations. The literal pool
also contains an unreferenced `" queue_start "` at `0x0046AC98` and two unused
`"sfx%02i.wav"` copies at `0x0046AD04` and `0x0046AD20`. Their combined 40
bytes plus the 24 state bytes account for the remaining 64-byte difference.
Do not fill these gaps with dummy globals or invented dead code.

The linked-data audit checks the complete 296-byte descriptor table and 408
bytes of used music literals in four separate runs. The final run also checks
the following 80 bytes of screen allocation and memory-stack diagnostics.
The run metadata is in `config/data-literals.h`; it deliberately excludes
the unrecovered literals.
These are partial data checks, not the complete-section guarantee provided
for damage and view by `verify-unit-data`.

The next complete data block begins at `0x0046AD88`, after the screen and
memory-stack diagnostics. `ending.c` groups the tagged-allocation helpers
starting at `0x0042FA90` with the campaign-ending scenes through `0x0043070F`.
The allocation tag occupies eight bytes, followed by the victory text table,
retreat text pointers, four projectile origins, and two ending-title pointers.
The original reads both dwords of the tag; retain the full eight-byte array.

All eight variables have references confined to this group and are private.
The text pointer initializers contain their own literals, followed by the
four separate use-site `"%X%Y%s%P"` formats. Keeping named character arrays
instead changes alignment and adds gaps. The full `.data` section is 457
bytes, ending with the last format's terminator at `0x0046AF50`.
`verify-unit-data` checks the complete block, all variable offsets, and all
eight pointer targets with one address shift and no string fallback.

The input state at `0x0046AF54` follows those literals. The two initialized
words occupy six bytes in `input.obj`, with the second at offset four.
The communication block starts at `0x0046AF60`, after six bytes of linker
alignment. A single object's two-word prefix would place the first menu
word only eight bytes after the input state, instead of the observed twelve.
The separate objects reproduce this spacing without inventing a variable
in the gap. `input.c` contains the four functions from
`UpdateInputDeviceTransitions` through `PollMenuInputDevices`; `comm.c`
begins with `get_face` at `0x00430BC0`.

The communication declarations occupy 116 bytes, followed by a 404-byte
literal pool. The ace and menu tables contain 17 pointers to separate
literals. In particular, `"Bakhtosh"` requires nine bytes including its
terminator; the former 32-byte packed ace-name array truncated that zero.
The three identical `"%s: %Fs"` formats are separate literals in
`real_vid_transmit`. The pending menu action and menu-label table are private
to this unit. Other declarations remain shared with their original or SDL
callers. `verify-unit-data` checks the complete 520-byte `.data` section,
all 13 declaration offsets, and all 17 pointer targets with one address shift.
It also checks this block's position within the adjacent run from ending
through joystick calibration, described below.

Video mode state follows the communication formats at `0x0046B168`.
The mode word and `PrintPaletteAllocationMap`'s `"%c"` and newline literals
form a complete ten-byte `.data` section. The next initialized table begins
at `0x0046B178`, after those literals and six bytes of alignment, requiring
a separate object. `video.c` contains the four functions from
`ShutdownVideoHook` at `0x004318F0` through `PrintPaletteAllocationMap`.

The graphics-mode table occupies eight bytes at `0x0046B178`. The strings
`"TM.DRV"` and `"SUPERTM.DRV"` follow at `0x0046B180` and `0x0046B188`;
neither has a relocation or disassembled reference in the original. Private
character arrays retain this real data, and the compiler supplies the one
alignment byte between them. These are matching declarations, not recovered
original variable names. The two independent `"j.cal"` literals follow at
`0x0046B194` and `0x0046B19C`, used by `LoadJoystickCalibrationFile`.
All 42 bytes match as `joycal.obj`'s complete `.data` section. The source
window begins with that function at `0x00431A10` and retains the adjacent
rectangle helper through `0x00431EFF`; its internal boundary is provisional.

`verify-unit-data` checks both complete sections and all four declaration
offsets. Ending, input, communication, video, and calibration data also match
as a single 1,050-byte run with all 15 inter-object alignment bytes included.
The boundary between the allocation helpers and ending scenes remains
provisional. These placement windows do not establish original filenames.

DirectDraw's initialized interface pointers, cascade level, and frame-timing
state belong to `dib.c`. The first seven declarations occupy 28 bytes at
`0x0046B1A4`–`0x0046B1BF`; four further declarations occupy 16 bytes at
`0x0046B1C4`–`0x0046B1D3`. The ten variables used only by this unit are
private, while `nFrameIntervalMs` remains shared with the SDL flight code.
Both observed state runs match after relocation, but the zero words at
`0x0046B1C0` and `0x0046B1D4` have no known references or recovered declaration
form. They remain unresolved rather than being represented by filler variables.

The complete 3,620-byte DirectDraw literal pool follows at `0x0046B1D8`,
ending with `DirectDrawResultToText`'s `"<undefined>"` terminator at
`0x0046BFFB`. `DIBcascade` must spell the surface-lock failure branch before
the success path so their separate diagnostic strings appear in that order.
This pool is checked through `abDIBDiagnosticLiterals` comparison metadata.
The current `dib.obj` `.data` section is 3,664 bytes, eight short of the
observed 3,672-byte span. Its full layout is therefore not included among the
complete-unit gates; its uninitialized storage also remains in `globals.c`.

The shared game state begins at `0x0046C000`, after the DirectDraw literal
pool. Its camera scripts, mission records, and maneuver tables precede
`show_info_disp`'s literal pool at `0x0046D9CC`. This definitions-before-literals
order supports their placement together in `game.c`, renamed from the former
information-display-only `text.c`. This is a descriptive filename; the
original spelling is not recovered. All 59 declarations retain their
original address order. Thirteen canned scripts and five defense-maneuver
lists are private because only the pointer tables in this unit reference
them; shared state and tables keep their public declarations.

The 112-byte state run at `0x0046C000` and the 21-byte run at `0x0046C074`
match independently. The four zero bytes at `0x0046C070` have no known
references or declaration form and remain unrecovered. Their absence also
changes the alignment before the first script, leaving the rebuilt section
eight bytes short: 6,788 bytes versus the observed 6,796-byte span.
`verify-unit-data` explicitly labels these as partial-section checks.

The full 6,652-byte run from `asIntroCameraSequence` at `0x0046C090` through
the final display-string terminator at `0x0046DA8B` matches with one address
shift. It includes 20 navigation records, 48 ship records, both weighted
maneuver tables, all intervening alignment, and the 192-byte display literal
pool. The gate checks all 18 pointer relocations against their positions
within this run, with no string fallback. Array annotations precede their
declarations, and the canned-script lengths are explicit so the ordinary
global-data audit also checks their complete contents.

The briefing and medal unit begins with `LoadBriefingRoom` at `0x00436D00`,
so the `eventmgr.c` placement window ends at `0x00436CFF`. Its initialized
state begins at `0x0046E188`, after the keyboard diagnostic. The character,
portrait, medal, and scene-animation tables precede the medal-name literals
at `0x0046E594`; ten separate text-format literals follow in function order.
The last `"Game Over"` terminator at `0x0046E699` precedes the raster object.
These 27 declarations now reside in `screens.c`, in original address order;
17 are private, while the ten used by other units retain shared declarations.

The character table uses a reconstructed capacity of ten 18-byte records,
with its eight populated initializers unchanged. The original clears ten
activity bytes in `ReturnToBriefingLongShot`, even though its main briefing
loops visit eight characters. Two implicitly zero-initialized records occupy
the 36 bytes from `0x0046E2A8` through `0x0046E2CB`; normal compiler alignment
supplies the four remaining zeros before the medal-position array at
`0x0046E2D0`. No pointer relocation occurs in that tail. This capacity is an
inference from the activity storage and complete data layout, not a uniquely
recovered source declaration. `CloseLook` also retains its original
fourteen-iteration path; the storage reconstruction does not alter that loop.

One complete-section check now covers all 1,298 bytes from `0x0046E188`
through the final string terminator. It verifies all 27 declaration offsets,
all 14 pointer relocations, and the formerly omitted tail with one address
shift. The table-capacity change leaves every function comparison unchanged.
The adjacent raster object has 6,640 initialized bytes, 22 public data
symbols, and no data relocations. Screens, raster, killbrd, and gr now also
match as one 11,486-byte run, including all ten inter-object alignment bytes.
These checks establish relative layout; their absolute addresses still differ.

The debriefing movement table contains 50 signed samples. The original
nonzero bytes at `0x0046E568` and `0x0046E569` are its final two `-2` samples,
followed by two alignment bytes before the first position word. The draw
loop still uses only 48 samples. Keeping ten explicit zero initializers for
`ReturnToBriefingLongShot`'s local flags also removes the extra empty-string
literal produced by `char active[10] = ""`; the complete 262-byte literal
pool now has the original contents, duplicate formats, and alignment.

The earlier shared-data range contains input state at `0x0046DA90`, six
viewport geometry records at `0x0046DAB8`, and direction/cockpit tables from
`0x0046DB28`. Its direct users span several existing source units, and its
position before the text diagnostic at `0x0046E14C` does not establish an
owner. These definitions remain in the temporary `globals.c`, now ordered
by original address. Moving their annotations outside the initializers also
lets the linked-data audit check ten previously omitted tables.

The 1,572-byte run from `0x0046DB28` through `0x0046E14B` matches with one
address shift, including all table alignment. Its final twenty bytes contain
ten signed words initialized to `-400`, now retained as `DAT_0046e138[10]`.
No identified direct reference establishes their name or purpose; the scalar
width follows the repeated two-byte values, without claiming the original
array declaration. The earlier 26-byte input-state and 96-byte geometry runs
also match individually. Twelve unexplained zero bytes at `0x0046DAAC` and
sixteen at `0x0046DB18` prevent a complete-range match. They remain unresolved.

The room/kill-board initialized-data block runs from `0x00470090` through
`0x00470673`, after the raster library. Its 25 declarations now reside in
`killbrd.c`; 22 are private, while the pilot-rank names, initial campaign
state, and room-transition flag remain shared. The complete 1,508-byte
section matches with one address shift, including 24 pointer relocations
and the 328-byte literal pool. The initial pilot records retain their inline
name/callsign arrays. The menu-label pointer arrays instead hold separate
compiler-emitted literals, including two writable pilot-name placeholders.

The adjacent words `{158, 28}` at `0x0047049C` and `0x0047049E` are represented
as `ShortPoint stRecRoomPanelOrigin`. Two independent short definitions insert
alignment between them and move the following menu table by eight bytes.
The paired representation preserves the observed four bytes and the complete
section layout. No inbound reference establishes the original symbol or
declaration form; it is not claimed as recovered source spelling.

`ResetCampaignData` at `0x00440800` copies the unit's initial campaign and
pilot records. The shape helpers beginning with `CheckHeapBlockSignature`
at `0x004408A0` use the following graphics literal pool instead: its first
diagnostic is at `0x00470D18`, after the palette and trigonometric tables.
The six helpers through `SignExtendClipCoord` now open `gr.c`, ahead of
`ValidateViewportBounds`. The literal-free packet-section reader at
`0x00440840` retains its provisional placement at the end of `killbrd.c`;
the exact boundary around that one routine remains unproven.

The graphics unit's complete initialized-data section runs from `0x00470678`
through `0x00470E65`. Its 256-byte palette translation table and two 720-byte
absolute trigonometric tables are private to `gr.c`. The following 334 bytes
contain the shape, raster, streamer, and registry literals in function order.
All 2,030 bytes match with one address shift, and the adjacent room/graphics
gate checks the whole 3,542-byte run, including the four alignment bytes
between the two objects.

The two literal references at `0x00441351` and `0x00441361` establish separate
diagnostic functions at `0x00441350` and `0x00441360`. Each pushes its own
name (`__copyline` or `__wipeline`), calls the existing empty raster hook,
cleans the argument, and returns. Both 16-byte spans now match, including
compiler alignment. These unreferenced routines were missing from the seeded
function inventory; their bytes must not be included in `DrawFontGlyph`'s span.

Private C definitions require separate evidence: a trial moving the two
MONODEBG variables to `static` storage in `mono.c` emitted the device handle
before the installed flag, opposite to the original. Reversing their source
declarations did not change that order. Those definitions remain in
`globals.c`; the C++ result does not justify applying the same layout claim
to all private C variables.

The registry arrays occupy gr's complete 56-byte read-only section, checked
at `0x004631C8` with one address shift and the value-name array at offset
`0x30`. The same routine's `"reg cheating\n"` and `"reg not cheating\n"`
diagnostic literals occupy
writable data. Changing the whole unit's string-storage policy would therefore
be incorrect. The three single-precision constants at `0x004630DC`–`0x004630E7`
encode approximately pi, degrees per radian, and radians per degree. Their
names and owner remain unidentified, and no base-relocated pointer targets
this range. Three `const float` definitions in the temporary `globals.c`
preserve their bytes and four-byte spacing; the read-only gate verifies all
twelve bytes and their storage flags. Their absolute placement is still wrong:
the original precedes the DirectDraw GUIDs, while temporary storage follows
the graphics registry arrays. The complete read-only section now has the
original size, which alone does not establish a section match.

The raster library is the one assembled object, and its edges are proven.
CL pads every optimized function with NOPs to a 16-byte boundary, so the
`mov edi, edi` after `FillRasterBytes` closes `screens.obj` at `0x439C40`. The
library's own functions are unpadded, and LINK fills the gap after it with
INT3, the only such run in the core, up to `killbrd.obj` at `0x43F640`. Its
`.data` (`0x0046E69C`–`0x0047008B`) follows the same order, between the
"Game Over" literal of `screens.c` and the kill board tables.

`src/globals.c` is temporary storage, not an original unit. Move a global only
when ownership and declaration order are proven. When a boundary changes,
update this table and `SRCS_ORDERED_CORE` together, then run `make order` and
`make verify`.

The unreferenced music wait routine at `0x0042E970` reads a word at
`0x0046AA0C` that the seeded global list omitted. Its original value is one.
This is `nMusicWaitForInput` in `music.c`: when nonzero, it keeps polling for
input even when the music status word is zero. The routine also respects the
existing music-mode and wait-enable settings.

Several unreferenced event and graphics routines likewise lie between entries
in the seeded function map. Recovered entries are recorded in `src/map`;
remaining discovery candidates include the rectangle routine at `0x00436AC0`
and legacy event hooks. Do not replace these gaps with padding. The apparent
entry at `0x004393F0` lies in a switch table/padding range and is not a function.

The current boundary supplement has 29 unimplemented entries in addition to
the four reproduced array initializers. Their original aligned spans total
880 bytes: 27 empty or constant-return routines occupy 16 bytes apiece,
the jump-only entry at `0x00436390` occupies another 16-byte span, and the
rectangle routine occupies 432 bytes before `FillGraphicSuffix` at
`0x00436C70`. These are verified code boundaries, not recovered interfaces
or names. In particular, the jump entry targets
`ClearInputKeyStatePreservingModifiers` and must be compiler-generated;
the surrounding empty routines' purposes cannot be established by proximity
alone. The function report does not score these absent implementations, so
its average similarity must not be interpreted as whole-program completion.

At the autopilot/input size checkpoint of 2026-10-18, the 1,521-function
report has 1,235 normalized exact matches, 261 same-size differences, and
25 size differences totaling 166 absolute bytes (net +10). The rebuilt
`.text` is 896 bytes shorter than the original; the missing aligned spans
account for 880 of those bytes. The data virtual-size deficit remains 1,856
bytes. Recovering the three input/autopilot spans removed 48 excess bytes
but shifted downstream code: strict positional matches fell from 122,403
to 53,323 of 518,656 bytes (10.2810%). This is an intermediate function-level
improvement, not a whole-file match; the strict comparison still fails.

The rectangle routine's stack reads need particular care. Relative to ESP
after its four register saves, its two arrays of sixteen 32-bit coordinates
start at offsets `0x24` and `0x64`. The branch at `0x00436B97` compares the
first X slot (`[ESP+0x24]`) with four; that slot was already initialized
from the first rectangle's left coordinate. One selected index likewise
loads `[ESP+0x14]`, where that coordinate was saved. Only the other two
index slots, `[ESP+0x18]` and `[ESP+0x1C]`, have no preceding writes in the
routine. Its four result stores alternate between offsets zero and two,
so the maximum calls overwrite the earlier minimum results.

Probes using two sixteen-element arrays, an uninitialized count, and three
selected indices recovered the 432-byte span but only 89.66% instruction
similarity, with different stack reads. Direct field expressions, LONG
coordinate elements, a two-plane array, and a fourth selected-index element
did not recover the original accesses; the fourth element also enlarged the
frame from 148 to 152 bytes. These probes are not retained. In particular,
do not infer four independent uninitialized scalars merely from the final
branch and index loads: some of those slots already contain coordinates.

A follow-up rectangle probe explicitly tested `x[0]` and initialized the
first selected index from the first rectangle's left coordinate. With three
index slots it emitted a 144-byte frame; four slots restored 148 bytes but
still tested the cached left coordinate at `[ESP+0x10]` instead of the
original `[ESP+0x24]`. Both reached only 85.34% mnemonic similarity. The
compiler also cached a selected coordinate across minimum/maximum calls
where the original reloads it. Sequencing those calls through a real result
temporary, volatile X-array accesses, and volatile selection loads did not
recover the original accesses; the latter enlarged the frame to 152 bytes.
These probes are not retained, and the rectangle remains unimplemented.

The compiler slot reuse proven in `SetFleetOverviewView` makes overlapping
uninitialized locals a possible explanation, but does not establish this
rectangle routine's declarations. A three-scalar index probe coalesces all
three reads into the same X-coordinate slot and shortens the frame to 140
bytes. Volatile scalars read separate slots but enlarge it to 152 bytes.
A four-element selection array with an uninitialized count gives 148 bytes
when loading bottom coordinates before right coordinates, but puts both
the count and the selection at the wrong end of the frame. Initializing
its first element from the left coordinate enlarges the frame to 152 bytes
again. Separating the coordinate temporaries and selection array into
scopes gives the original 148-byte frame and 432-byte span, but still reads
the wrong slots and differs in all 116 compared instruction encodings.
These additional probes are also discarded; matching the span alone does
not reconstruct the observed behavior.

A single 32-element coordinate array produces the same unsuccessful
148-byte frame and 432-byte span as the two-array scoped trial: 89.66%
mnemonic similarity, with all 116 instruction encodings different. Keeping
three volatile selected-index scalars in the later scope and capturing each
once also gives a 148-byte frame, but places the guard and index reads at
the end of that frame and shortens the span by sixteen bytes (88.79%).
Neither reproduces the three explicit index shifts. Both are reverted,
including the exploratory shared prototype, whose insertion also changed
unrelated compilation units.
