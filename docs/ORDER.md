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
| `0x00442750`–`0x00443DA5` | `src/ix/streamer.cpp` | streamer |
| `0x00443DA6`–`0x0044490F` | `src/ix/thread.cpp` | streamer |
| `0x00444910`–`0x004451B4` | `src/ix/dsp.cpp` | dsp |
| `0x004451B5`–`0x00445F5F` | `src/ix/dsps.cpp` | dsp |
| `0x00445F60`–`0x004467C4` | `src/ix/mixer.cpp` | dsp |
| `0x004467C5`–`0x00446F73` | `src/ix/dspv.cpp` | dsp |
| `0x00446F74`–`0x004471FF` | `src/ix/dspmix.asm` | dsp |
| `0x00447200`–`0x00447CD7` | `src/ix/system.cpp` | sound |
| `0x00447CD8`–`0x0044879B` | `src/ix/sound.cpp` | sound |
| `0x0044879C`–`0x004492DF` | `src/ix/sample.cpp` | sound |
| `0x004614C0`–`0x0046162F` | `src/ix/lzo1x.cpp` | lzo |
| `0x00461630`–`0x004617BF` | `src/ix/ixfile.cpp` | file |

These source-file anchors establish function order, but do not establish every
object boundary. The retail image has INT3 fill at `0x004446F6`–`0x004446FF`
before the stream-file methods, at `0x00445181`–`0x0044518F` before the first
initializer pair, and at `0x00446795`–`0x0044679F` before the second pair.
Conversely, the first thread, stream-DSP, and voice-DSP functions follow their
predecessors without alignment. The initializer pairs may belong to the
following source groups; their current placement at the ends of `dsp.cpp`
and `mixer.cpp` is not proof of ownership. Recover the original declarations
and object grouping rather than inserting padding or registering hand-written
initializer wrappers.

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
| ix members appear streamer, thread, dsp, dsps, mixer, dspv, system, sound, sample | Streamer, dsp and sound libraries searched in that order |
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
| `src/geom.c` | `0x418000`–`0x41AD4F` |
| `src/barracks.c` | `0x41ADA0`–`0x41C75F` |
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
| `0x004698E0`–`0x0046994F` | `ship.c`: the writable explosion-debris table referenced by `Create_explosion_debris` |

The GUID sequence includes `CLSID_DirectDrawClipper`, absent from the bundled
DirectDraw header, between `CLSID_DirectDraw` and `IID_IDirectDraw`. The bytes
in the retail image establish its value and position. Unused SDK identifiers
are real original data, so their definitions must survive the reconstruction.

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
