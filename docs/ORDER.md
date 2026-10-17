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
| `0x004486B0`–`0x004492DF` | `src/ix/sample.cpp` | sound |
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
| `src/cmpgn.c` | `0x4046A0`–`0x40609F` |
| `src/brains.c` | `0x4060A0`–`0x40CFFF` |
| `src/nav.c` | `0x40D000`–`0x40FFFF` |
| `src/spc.c` | `0x410000`–`0x412FFF` |
| `src/cockpt.c` | `0x413000`–`0x417FFF` |
| `src/geom.c` | `0x418000`–`0x41A9CF` |
| `src/barracks.c` | `0x41A9D0`–`0x41C75F` |
| `src/debug.cpp` | `0x41C760`–`0x41D0BF` |
| `src/mathutil.c` | `0x41D0C0`–`0x41D11F` |
| `src/disk.c` | `0x41D120`–`0x41DEDF` |
| `src/ship.c` | `0x41DEE0`–`0x4219BF` |
| `src/logic.c` | `0x4219C0`–`0x424FFF` |
| `src/pilot.cpp` | `0x425000`–`0x426FFF` |
| `src/system.c` | `0x427000`–`0x4274DF` |
| `src/main.c` | `0x4274E0`–`0x427FFF` |
| `src/hudmsg.c` | `0x428000`–`0x42AFFF` |
| `src/pload.c` | `0x42B000`–`0x42B3FF` |
| `src/sound.c` | `0x42B400`–`0x42CFFF` |
| `src/music.c` | `0x42D000`–`0x42EFFF` |
| `src/screen.c` | `0x42EFC0`–`0x431EFF` |
| `src/dib.c` | `0x431F00`–`0x43390F` |
| `src/text.c` | `0x433910`–`0x433ABF` |
| `src/smart.c` | `0x433AC0`–`0x434CCF` |
| `src/mathfp.c` | `0x434CD0`–`0x4353FF` |
| `src/strdos.c` | `0x435400`–`0x4355EF` |
| `src/eventmgr.c` | `0x4355F0`–`0x436FFF` |
| `src/screens.c` | `0x437000`–`0x439C3F` |
| `src/raster.asm` | `0x439C40`–`0x43F635` |
| `src/killbrd.c` | `0x43F640`–`0x440BFF` |
| `src/gr.c` | `0x440C00`–`0x44269F` |

`LoadPaletteTripletsFile` at `0x00404610` closes `auto.c`. Its `"rb"`
literal is at `0x00465548`, immediately after the initialized autopilot
formation count at `0x00465544` and before the ejection tables at
`0x00465550`. The campaign unit starts with `ejection_sequence` at
`0x004046A0`. The packet error word at `0x00465460` similarly precedes
`mono.c`'s data-file error strings. These initialized globals are kept in
their owning units and included in the distributed global audits.

The beginning of the constant and initialized-data sections also constrains
ownership and qualifiers:

| Original data | Owner and storage |
| --- | --- |
| `0x00463008`–`0x004630D9` | `barracks.c`: the 21 constant menu regions passed directly by `BarracksScreen` |
| `0x004630E8`–`0x00463167` | `dib.c`: eight DirectDraw class/interface GUIDs, followed by its frame-rate constants |
| `0x004631C8`–`0x004631FF` | `gr.c`: constant registry-key and `Cheater` value-name arrays used by `ReadCheaterFlagFromRegistry` |
| `0x00465040`–`0x00465057` | `winmain.c`: two initialized hazard counters and two writable eight-byte hazard tables |
| `0x00465058`–`0x004650B3` | `winmain.c`: window/audio flags, heap counters, class/title arrays, and message-pump state; two intervening words remain unidentified |
| `0x004693C8`–`0x0046963F` | `barracks.c`: bunk origins, menu pointers, modal bounds/state, campaign file numbers, and the following string literals |
| `0x00469644`–`0x00469667` | `debug.cpp`: console count, keyboard state/hook, font name, spinner, and newline arrays |
| `0x00469668`–`0x00469872` | `disk.c`: graphics-mode and error-message pointers, disk marker, prompt settings, and the complete following literal pool |
| `0x004698E0`–`0x0046994F` | `ship.c`: the writable explosion-debris table referenced by `Create_explosion_debris` |

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
data ownership is recovered. The palette flag at `0x00469640` remains in
`globals.c` because its original unit boundary is still uncertain.

The debug-console data is a complete 36-byte block. Both keyboard words have
users outside `debug.cpp`; the count, hook, and character arrays are private
to that unit. Moving these definitions preserves all 36 bytes, including
the padding after the five-byte spinner array.

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

The registry arrays occupy read-only data, whereas the same routine's
`"reg cheating\n"` and `"reg not cheating\n"` diagnostic literals occupy
writable data. Changing the whole unit's string-storage policy would therefore
be incorrect. The read-only layout still has three unresolved float constants
at `0x004630DC`–`0x004630E7`. Their ownership or declaration form must be
recovered from evidence; do not fill these gaps with padding variables.

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
