# Matching patterns

The original disassembly and extracted strings are authoritative. Decompiled
code, operational labels, and other releases are only leads. Use `binary-comp`
for similarity decisions.

## Source choices that affect matching

- Biased array operands can precede the actual array. The carrier-launch
  fighter loop loads from `0x0046A5AE + frame * 2` for frames 9 through 22.
  Its real 14-word table starts at `0x0046A5C0`; `frame - 9` reproduces the
  operand. The four bytes before the table are normal alignment, as are
  the four before the following view script. Including these bytes as
  array elements changes compiler layout and invents unused source data.
- Confirm arguments at call sites, return width from register use, and calling
  convention from the terminating `RET`. Core functions are normally
  `__cdecl`; `ix` methods receive the implicit object in ECX.
- Prefer `short` when the original emits 16-bit operations.
- The word array at `0x0059C790` is unsigned object mass, copied from
  `ObjectTypeData.mass` at offset `0x0A`. Its only readers are the two
  force-application routines and `object_collision`: they zero-extend it
  before dividing force or computing mass ratios. The former `radarRadius`
  name and signed type were misleading. `awObjectMass` preserves the original
  storage while expressing those reads directly.
- The runtime scale array at `0x0059DE40` is also unsigned. The original
  `house_keep_objects` halves it with a word `SHR`, and `animate_shape`
  uses `SHR` in both scale-command calculations. `transform_objects_to_your_view`,
  `ShowCarrierLaunchSequence`, `ShipExplosion`, and `reposition_fixed_child_objects`
  zero-extend their scale reads. Declaring `awObjectScale` as unsigned short
  reproduces those logical shifts without per-use casts; a signed array
  incorrectly preserves the high bit while shifting. Its storage size and
  original address annotation stay unchanged. The full function report
  improves only the two shift users and preserves every other result.
- Preserve visible local-variable order, branch direction, fall-through, and
  epilogue placement. Equivalent C often emits different instructions.
- `FormatTextTokens` uses the VC4.20 `va_list` and `va_arg` macros. Its native
  short and unsigned-short arguments consume four-byte slots while loading
  only the original halfwords. Treating the cursor as an `unsigned long *`
  instead changes argument-load order and width throughout the formatter.
  The outer character dispatch is a `switch`: CL promotes the character
  before comparing it with `%`, whereas an `if` keeps a short comparison.
  Together these choices produce an exact function comparison, including
  both dispatch tables and the original 64-byte stack frame. SDL continues
  to use its platform's `va_list` with default argument promotions.
- `Title_Sequence` dispatches input events with a switch. Mouse activation
  enters the shared selection path directly; the keyboard cases initialize
  their activation flag, handle the key, and fall through only when requested.
  An if-chain instead extends the flag's lifetime and spills the alternate
  menu shape. The switch restores the original 28-byte frame and 1,856-byte
  aligned span. A real integer local for the selected menu index also restores
  its signed-byte promotion before the bounds tests and the keyboard-switch
  registers. Together these changes raise similarity from 92.39% to 97.25%,
  leaving 74 differing instructions out of 472. Selection branch placement
  and mouse-coordinate argument loads still differ. The original title call
  sign-extends both coordinates, whereas the original barracks, training-menu,
  and recreation-room calls pass dwords whose low words hold the coordinates;
  do not widen the shared short-argument prototype from the title call alone.
  Reversing the bounds condition and spelling its invalid path with a shared
  label leave the same output, so those probes are not retained.
- `InitializeConstellationField` saves the viewport height in its four-byte
  frame, at `[ESP+0x12]` after the register saves. The star loop loads that
  word into DI once; the particle loop reloads it for each Y-position call.
  A volatile saved height and a separate function-scope `starHeight` recover
  those lifetimes, including BX for the star counter and DI for the particle
  counter. Retain a byte offset into the particle records, while accessing
  their members through `ConstellationParticle` fields. An element index
  instead omits the original `LEA EBX,[EAX*8]` and changes the stores.
  Promote density into one integer `population`, use it for the star count,
  then multiply that same local by sixteen for the particle count. This
  schedules `SHL ECX,4` before the star-count division, as in the original;
  independent product locals and a register hint do not. The 368-byte span
  is unchanged, but similarity rises from 84.54% to 98.97% and differing
  instructions fall from 89 to two of 97. Only the closing star-loop
  comparison and branch remain reversed. These source choices reproduce
  allocation; the original qualifiers and local declarations are unknown.
  Keeping `starHeight` inside the guarded loop exchanges BX and DI. Direct
  array indexing removes the explicit offset but exchanges the particle
  loop's registers. Neither variant is retained.
  The unit change also exchanges the height and width calculations in
  `CalcRectangleArea`, changing its previously exact result to four differing
  instructions out of fourteen. Its source, size, and arithmetic are unchanged.
  Reordering its product or assignments, splitting the arithmetic into
  compound assignments, returning the dimension expressions directly,
  renaming its parameter, and making width unsigned do not restore the
  original allocation. Volatile bounds reads preserve height-first ordering
  but exchange AX and DX, increasing the differences to six. Those area
  probes are not retained; the initializer removes 87 differences while the
  area helper gains four.
- `DrawConstellationField` retains a byte offset into its parallel velocity
  and frame word tables across the random-position calls. The original
  sign-extends the random short and saves twice that value with `LEA`; retaining
  an element index instead omits both scaled-offset instructions. Expressing
  the shared byte offset recovers the original 560-byte span and raises
  similarity from 96.38% to 99.28%, with only the star loop's closing comparison
  and conditional branch differing. Reversing that source comparison,
  spelling the loop as a guarded do/while, and using a volatile count read
  all leave those two instructions unchanged; these probes are not retained.
- `CalibrateJoystickInteractive` sets text alignment before testing the loaded
  font. Each sampled axis supplies both its 32-bit global and its 16-bit saved
  calibration word; chained assignments preserve the original load/store order.
  Its range divisors and four scale factors remain in registers until clamped.
  The zero-scale clamps increment in left, up, right, down order, followed by
  the global stores. Real locals for these values reproduce that sequence and
  retain the original 36-byte stack frame. Register allocation and the final
  bound calculations still differ; the aligned span remains sixteen bytes short.
- In `/Od` code, keep explicit pointer locals when the original stores them.
- Repeat identical literals at each use site; string pooling was disabled.
  Source switch-case order also determines literal order independently of
  the final code layout. In `process_player_input`, the SFX up/down cases
  precede music up/down. The flight-control music toggle precedes the SFX
  toggle. In `PacketLoad`, the compressed-data case precedes default even
  though CL schedules the default code first. These arrangements recover
  all separate literals without changing the existing function scores.
  CL also retains a diagnostic inside a folded-away `sizeof` check. The
  unreferenced main-module size diagnostic is reproduced this way, as a
  controlled compiler probe confirms; a named filler string is unnecessary.
  In `DIBcascade`, write the surface-lock failure branch before the success
  path. CL emits its second `" failed\n"` before `" successful\n"`, matching
  the original addresses. Reversing those source paths can preserve the
  normalized function match while swapping the literal order. The complete
  3,620-byte DirectDraw literal-pool check catches this difference.
  Distinguish literals from named arrays: `CreateMainWindow` references the
  same writable class-name array three times, plus a separate title array.
  Four independent literals reproduce the text but not the original storage.
  `WinMain` passes an empty literal to `main`, and `GetJoystickDevCaps` emits
  its failure diagnostic between the memory report and corruption strings.
  The complete 679-byte startup literal check exposed a former `"Vj"`
  spelling that preserved the function's normalized instruction match while
  changing the pointed-to bytes. Use the original image to check even short
  literals omitted by the extracted-string listing.
  Both low-memory branches in `LoadOriginFxDrivers` likewise reference one
  100-byte writable message array before its memory-reservation tables.
  Repeating the message at the call sites adds a duplicate to the literal
  pool and loses the original declaration order.
  `ReportOutOfMemoryAndExit` also uses this shared message; its former shorter
  literal both changed the original text and added unwanted literal storage.
- `flag_reached` writes its normal-arrival branch before its waiting branch
  in source. CL still schedules the waiting path first, but emits the arrival
  strings before the wait format, matching the cockpit literal pool. The
  final empty HUD string remains a use-site literal whose first byte is read;
  replacing it with an immediate zero omits original storage and a relocation.
- The target display's empty string, three separate `" %s"` formats, and
  other labels are use-site literals. Keeping named arrays introduces extra
  eight-byte alignment and separates them from their following display-mode
  messages. `DrawTargetRangeReadout` emits `"TOO FAR"`, `" m"`, and `"----- m"`
  in that order: test the visible-target case first, then the excessive range.
  Its numeric path scans the result of `itoa` with `strlen` and copies the
  suffix to the scratch-buffer base plus that length. The original `NOT ECX`
  and base-relative `LEA` distinguish this from `strcat`, which uses the
  scanner's ending pointer directly. Ordinary `strcpy` calls produce the
  shared eight-byte readout copies. This restores the original function size;
  the placement of the excessive-range branch remains different.
- Preserve diagnostic whitespace. The music wrappers' original literals
  include newlines, and `StopMusic` prints a trailing space before its stream
  stop and a separate newline afterwards. Normalized function comparisons
  alone did not expose these different strings; linked literal-span checks do.
- `RecRoom`'s two pilot-name formats each have a separate empty-string
  literal in the dead-pilot branch. They are not offsets into padded format
  arrays. `InitializeRoomMenu` reads the first byte of its own empty literal;
  spelling that expression directly preserves the original byte load under
  CL 4.20. The chalk-board KIA argument is emitted before its format string,
  as the original data order requires. The complete room-data gate checks
  these short literals and every repeated format without string fallback.
- `PrepareShapeRLEData` loads the first four bytes of the `"1.00"` literal
  into the shape buffer as a dword. Preserve that memory load and the literal's
  own terminator; an immediate version value would omit its storage and
  relocation. The graphics unit gate checks its position between the viewport
  and overflow diagnostics.
- `ReturnToBriefingLongShot` clears ten local flag bytes with ten explicit
  zero initializers. An empty-string initializer instead emits a separate
  literal and clears the trailing bytes through a different sequence. Its
  retained character pointer addresses the animation-phase field, after the
  random activation test, rather than the whole character record. Test the
  frame limit against 40 to retain the original `CMP`/`JL` pair. These forms
  restore the function's size and mnemonic sequence; some registers differ.
- `DebriefingEstablishingShot` reads the frame-skip counter into a short local,
  optionally replaces it with one on the last frame, then decrements and
  stores that local before testing it. Separate operations on the global
  keep an unnecessary constant in a saved register and add sixteen bytes to
  the aligned span. The retained short value reproduces the original sequence.
- `DrawMedals` keeps the medal row's base coordinate separately from the
  badge row coordinate, and wraps badges at `x >= 232`. Keeping that base
  in a short local restores the aligned function size. Its addition/load
  order and register allocation still differ; a separate increment followed
  by a copy lets CL coalesce the locals and loses the recovered size again.
- `ProcessMusicScriptCommand` returns zero when the track is disabled or
  commands are suppressed, and one after dispatch. Its stop-command and
  repeated-track exits leave EAX unspecified, so retain their bare `return`
  statements in the integer-returning C function. The only caller discards
  the result. Declaring the function `void` omitted the two explicit result
  instructions and shortened its aligned span by 16 bytes. The restored
  return type and paths produce an exact function comparison.
- `MapMusicTrackToStreamerCommand` retains separate return blocks for cases
  27/28 and 29/30 even though each pair returns the same value. Its default
  label enters the return after case 19's diagnostic. Grouping those cases
  or moving default to the end changes both the branch encoding and the
  switch table. Restoring these blocks recovers sixteen bytes and gives an
  exact comparison. In `gametrack`, place the damage-at-least-two branch
  before the conditional choice between tracks five and zero; its complete
  function then matches too.
- `IsPointWithinEyeViewCone` returns zero for a projection below `0x94`, then
  returns one. Separate returns produce the original `MOV CX,0`; a result
  variable initialized to zero instead produces `XOR CX,CX`. That one-byte
  difference crosses an alignment boundary. The return form recovers the
  original 160-byte span and gives an exact comparison. A direct Boolean
  return and an explicit result-variable `if`/`else` do not match.
- `set_background_objects_rotation` clears the projected vector's Z field
  before calculating X and Y. Its short angle takes a remainder by 360
  immediately after `ArcCos`, before the X-sign test. The original `CWD` and
  `IDIV CX` establish this operation even though the usual angle range makes
  it redundant. Both changes recover the missing sixteen bytes and produce
  an exact comparison.
- `LoadShapeSet` calls `FetchDiskPacketRetrying` separately in its default-file
  and descriptor-file branches. CL shares their call tail but preserves the
  distinct argument pushes, including the branch's section-word reload.
  Replacing the descriptor file in a local before one common call loses
  those instructions. Separate source calls restore the original 112-byte
  span and match exactly.
- `sort_viable_target_list` still lacks the original saved next-outer index
  on its stack. Natural nested `for` loops, an explicit candidate-distance
  local, and reordered local declarations did not restore that spill and
  reduced similarity. Those trials are reverted; extra stack storage must
  follow from the real variables rather than padding or dummy locals.
- Check neighboring functions after these small source changes too. The five
  exact recoveries above preserve every previously exact function, but
  `transform_objects_to_your_view` and `servicetrack` each gain one differing
  instruction at unchanged sizes and mnemonic scores;
  `new_space_music_changes` loses one. Restoring the view-cone result local
  recovers its neighbor's old allocation but loses the view-cone match, so
  that trial is not retained.
- Single-use console strings are literals: the constructor's font and spinner,
  and the formatted logger's newline. Their offsets alone also fit named arrays,
  but arrays impose eight-byte object alignment. Literal storage reproduces the
  four-byte alignment required immediately after the palette flag.
- `StepPaletteTransition` computes one quarter of the maximum component delta
  before filling its accumulator array, then divides the maximum again for the
  countdown. Keep the first result in a real local; division inside the fill
  loop repeats work absent from the original. Making the maximum private static
  storage does not reproduce this sequence with VC4.20.
- Initialize structure fields individually when the original does so.
  `CreateMainWindow` assigns every `WNDCLASSA` field; a preliminary `memset`
  adds a `REP STOSD` absent from the original. The field order also controls
  how CL schedules stores around `LoadIconA` and `GetStockObject`.
- Use typed fields and arrays. Keep offset-based field names until their intent
  is proven.
- Use the matching CRT routine when the binary calls or inlines it.
- Check the original section before adding `const` to a table. The hazard and
  explosion-debris tables occupy writable data even though their reconstructed
  users only read them; the barracks menu regions occupy read-only data.
- Preserve initialized storage even when every byte is zero. In CL 4.20 C,
  a tentative definition becomes a linker common symbol, while `= 0` or
  `= {0}` puts the variable in initialized data. The packet-reference table,
  input state, music flags, and DirectDraw pointers occupy initialized blocks
  in the original. Checking their values alone does not catch wrong storage.
  Do not infer this distinction from raw section size alone: trailing file
  padding can overlap the start of uninitialized storage.
- Uninitialized C++ definitions have different storage from C tentative
  definitions. The overlay pointer in `pilot.cpp` and the two private wave
  variables in `sound.c` naturally produce `.bss` in
  declaration order. Keeping them in the temporary C globals unit instead
  produces linker common symbols with different positions. Restore proven
  ownership and verify object storage as well as zero values; zero-filled
  memory by itself cannot establish a matching layout. The following private
  C damage-display variable needs `static` to retain its `.bss` storage,
  as do the music filename buffer and event-manager cursor save buffer.
  The latter two remain externally visible only in the SDL build, whose
  adapters share them through the SDL-only header. The original build keeps
  their definitions private and uninitialized in the owning C units.
- Put a global's address comment before its declaration or after its final
  semicolon. A comment inside a multiline initializer is recognized by the
  layout audit but missed by the linked-data comparison's simpler parser.
  Put it before the declaration when interior comments contain semicolons,
  as in `aObjectTypeData`; those can terminate the simpler parser's scan.
- Object display names are separate literals in the packed record initializers,
  including 26 distinct empty strings. The compiler emits the complete pool
  after the coordinate tables with four-byte literal alignment. The last empty
  literal ends the object; the following three zero bytes are linker alignment
  before navigation. A flattened character array incorrectly owns those bytes.
- Recover biased array references from storage alignment and indexing together.
  The funeral scene lookup's displacement is two bytes before its aligned
  array. `asFuneralSceneBySeries[currentSeries - 1]` emits the original word
  lookup while allowing the preceding particle array's four alignment bytes
  and the following flag's alignment to come from the compiler.
- `perform_maneuver` uses a switch for the three inactive-target cases, placing
  their comparisons before the case bodies. Its proximity test compares the
  global threshold against the saved range. Let the reroll byte promote to
  `int` when comparing the random result: a cast to `short` produces a word
  comparison instead of the original sign extension and dword comparison.
  These forms improve the routine from 83.65% to 97.12%; scheduling and
  registers in the radius calculation still differ.
- `DIBupdate` keeps `top` in its argument stack slot while width and height
  occupy EBX and EBP. A volatile qualification on the definition's `top`
  parameter preserves the original memory comparisons, clipped store, and
  repeated accesses. It retains the 116-byte frame and restores the original
  384-byte span, raising similarity from 84.11% to 100% of mnemonics without
  changing other function comparisons. The qualification reproduces observed
  accesses; it does not establish the original source spelling or change
  the function's calling convention or parameter type compatibility.
  The two row-offset products still load their operands in reverse order.
  One extra body byte shifts subsequent relative operands, leaving 45 of 107
  instructions different in the byte comparison. Reversing multiplication
  operands and splitting the products through a real offset local did not
  change that output. Qualifying the source pointer instead preserved the
  size but displaced the left bound and reduced similarity to 86.92%.
  Earlier register hints, reordered extent declarations, an explicitly
  guarded do/while loop, and reusing the right/bottom parameters as extents
  also failed to recover the original allocation. These trials are not retained.
- `DIBsetPalette` still reads the RGB pointer before calculating its cache
  offset, unlike the original. An explicit byte-offset local did not change
  the generated sequence. Reading a volatile index parameter once into an
  int local and using that local throughout likewise left its 85.33% result
  unchanged. Both probes are reverted.
- Keep direct array-field accesses when the original repeatedly uses an index
  plus an absolute field address. A cached structure pointer can produce the
  same mnemonics with shorter displacements, changing the function's size.
  `ConfigureScrambleActor`, `NavMapPointInsideReservedArea`, and
  `ReserveNavMapArea` recover their original sizes with direct field accesses.
- `InitializeCockpitResources` copies each VDU and pilot-hand rectangle as
  an eight-byte record. Four separate halfword assignments add instructions.
  Index the readout origins and hand bounds directly: cached pointers add
  address calculations absent from the original. Keep the scanner sizing
  call nested inside its allocation so the outer zero argument is pushed
  first. The final damage-background loop also pushes an extra zero before
  sizing, then cleans twelve bytes after `MaxShort`; its preceding sizing
  loops clean eight. `MaxShort` reads only its first two short arguments.
  A variadic declaration preserves both observed call forms without changing
  its exact body or calling convention; it does not establish the original
  header's spelling. These changes recover the original function size and
  mnemonic sequence. Seven instructions still differ in comparison operand
  order or weapon-loop registers. Reusing the frame index, moving the weapon
  local declaration, and reversing the source comparison did not resolve them.
- `draw_3d_scanner` retains one grid value across all four pixel calls but
  reloads the current cockpit's scanner coordinates at each call. Its grid
  counter is 32-bit, unlike the short object index. Initialize the counter
  before testing the first grid entry, and initialize the grid pointer only
  inside that guard. The target blink uses a short signed remainder by two
  and selects its colour in the draw-call argument; assigning a replacement
  colour first omits the original sign extension and branch sequence. This
  restores the function's size and mnemonics, with register and comparison
  operand differences still remaining.
- `vdu_polygon` indexes the cockpit-bar table by view and bar directly.
  Initialize its retained length word within the extent expression; loading
  it beforehand or caching the view index shortened the routine in earlier
  trials. The frame swap saves the empty frame first. Direct indexing
  recovers sixteen missing bytes, but the viewport bound reads and register
  allocation remain unresolved. Chained field assignments and reusing the
  percentage argument did not improve the match; copying the frame bytes
  with `memcpy` also enlarged the local frame and is not retained. Removing
  the length local leaves this function's instructions unchanged but alters
  register allocation in later mission functions. Check the entire unit
  when changing locals, not just the edited function.
  Declaring bottom before right improves the current match to 88.52%, but
  its aligned span remains 32 bytes short. Moving the right-bound store
  before the bottom-bound load does not change the generated instructions.
- `fire_computer_graphic_missile` retains a signed-byte frame across the
  coordinate calculations. Refresh it after the VDU-mode query and after
  restoring the old background; use that value for background capture, then
  reload the global for drawing. These lifetimes reproduce the original
  frame reads and recover sixteen bytes, restoring the 336-byte span at
  85.87%. Explicit shape/background pointer locals and a promoted Y local
  did not improve the match and are not retained. Coordinate caching and
  an extra saved register remain different.
- `target_locking` dispatches through a weapon-type switch, with the
  heat-seeking body before image recognition. Keep the countdown in a
  short local, increment it, and store it before returning. Accumulate the
  squared screen distance in a real int local in two steps, and declare
  the short Y coordinate before X. Together these choices recover the
  original branch order, register allocation, and complete 256-byte span
  exactly. Reversing a single sum expression or moving the countdown
  declaration alone did not reproduce the allocation; promoting Y into
  the distance local before squaring it was worse.
- `place_damage_on_cockpit` retains the selected damage-state pointer after
  its two mode guards. Each explosion branch assigns both the pending
  damage and its state. CL shares those assignments after testing the
  shape pointer, preserving the original flags through the two stores.
  This restores its exact match after changes earlier in the unit.
  Hoisting the assignments in source delays the shape test; saving a
  Boolean result instead adds arithmetic instructions and sixteen bytes.
- The cockpit changes leave `cycle_onscreen_targets` with the equivalent
  reversed comparison and branch, two differing instructions at unchanged
  size. Reversing its source comparison, moving the target test into the
  loop body, caching the target count, and reordering local declarations
  do not recover the original order; those trials are reverted.
  `draw_target_box` improves to 97.62% at unchanged size and differing-byte
  count, while `print_message_text` returns to 95.83% with its existing
  sixteen-byte excess. Neither function's source changed in this batch.
- `print_message_text` keeps its character limit as a short and initializes
  the two wrapping pointers after measuring the viewport. Test the character
  just written to the output buffer so CL retains the loaded byte instead
  of rereading the input after a potentially aliasing store. Preserve the
  original forward newline offset from the current output cursor. Index the
  message origins directly with `cCockpitView` in each draw call. Caching
  that view in an int changes how CL shares the argument setup between the
  switch cases and the normal cockpit path. The direct accesses
  recover 95.83% mnemonic similarity, but pointer/index allocation still
  leaves two extra body bytes and therefore a 16-byte larger aligned span.
  Explicit break/index locals and alternate pointer initialization did not
  remove that difference. The full-unit check also recovers the exact
  `build_your_target_list` without changing that function's source.
  Qualifying the read cursor as `const char *` likewise leaves the generated
  instructions unchanged and is not retained.
- `vid_transmit` tests the cockpitless-view flag both before and after its
  frame-parity test. Keeping that repeated condition preserves EBX and the
  second `TEST EBX,EBX`, restoring the missing sixteen-byte aligned block.
  Its background-selection branches each contain their own sprite draw call.
  CL merges those calls, placing the common zero argument before the branch
  and matching the subsequent coordinate loads and portrait draw. Assigning
  a temporary background pointer or using a conditional expression in one
  shared call instead changes argument scheduling. The repeated guard and
  separate source calls reproduce the entire 432-byte span exactly, up from
  91.06% similarity, without changing other function comparisons.
- `fire_afterburner` measures velocity before querying the maximum velocity.
  Save the measured speed in a real int local before the comparison: CL 4.20
  evaluates the two calls in the opposite order when both appear directly
  in the comparison, even if its operands are reversed. The explicit speed
  restores 30 of the original 32 instructions at the byte-comparison level.
  Only the equivalent reversed comparison and branch remain different.
  An early return, a named limit, and a long speed local did not resolve them.
- `get_right_shape` retains object class and type as short values and the
  selected frame as a signed byte. Read the type only in the non-missile
  branch. Its pitch rounding clamps the promoted increment or decrement
  before narrowing back to short, and tests remainders against 15. Direction
  selection uses seven switch cases; the out-of-range path leaves its local
  unset, as the original stack load shows. Do not add a default direction or
  an extra yaw-sector correction. Zero the projected vector's Z component
  before the dot products, and reload the saved capital-ship frame after the
  screen update before indexing packet references. These choices reproduce
  the original 84-byte stack frame and complete 960-byte function span,
  including four relocated switch targets, seven selector bytes, and padding.
- `object_collision` reloads ownership, object classes, and its own mass
  after calls; caching their values across those calls removes original reads.
  Five vector workspaces suffice: one holds the projectile force, separation,
  object component, and successive impulses; a second holds each object's
  tangent in turn. The other three retain relative velocity, the partner's
  component, and their difference. Distinct projectile and collision damage
  words reproduce the observed local lifetimes. Keep the no-collision update
  in the final `else`, and nest the response limits as `MinInt(MaxInt(...), ...)`.
  These choices recover most of the routine. Its retained frame is 92 bytes,
  matching the original; other unit contexts increase it to 96 bytes because
  the compiler spills the mass pointer.
  An explicit mass pointer, a `register` qualifier, and a shared scale local
  did not recover the original allocation and are not retained.
- Check argument widths at callers even when the callee ignores an argument.
  `GetTransformedShapeBounds` takes a short flip value: widening it to `int`
  adds a sign extension in `easy2see` that the original does not contain.
  The solid-colour sprite functions likewise take a short colour value even
  though `SetSolidColourTranslation` uses only its low byte. Their callers
  extend byte colours into AX before pushing them.
- All ten direct callers of `FlushSoundEffectsAndLog` pass two arguments
  and clean eight stack bytes. Keep its int sound handle and short fade
  argument in the declaration, even though the Win32 body ignores both
  and tail-calls `FlushSoundEffects`. Ordinary typed calls replace the
  former incompatible function-pointer casts. The afterburner-expiry path
  passes its current global value and zero; an argumentless call omits
  original instructions.
  `PlayScrambleHangarScene` and `scramble` read an unset four-byte local
  handle at the flush calls. Preserve that native stack read, with zero
  initialization only for SDL. Their fade arguments are ten and zero.
  The hangar scene also passes `0x1b` to the empty `SceneLeaveHook`.
  Restoring these arguments recovers the original four-byte frames and
  the complete 720-byte and 1,104-byte function spans, respectively.
- `AnimateScrambleWalk` retains the escape byte after decrementing the
  frame-skip counter and refreshes it after the draw calls, alongside the
  left walk-frame increment. Use that value for the exit test after the
  coordinate updates. Its lifetime makes CL update the right walker's X
  directly in memory, rather than keeping a third coordinate in a register.
  This removes sixteen excess bytes and restores the 624-byte span at
  98.74%. Load scheduling and register allocation still differ. A compound
  assignment or reordered coordinate updates did not help; an explicit
  left-coordinate local, background-coordinate local, or separate escape
  assignments in the draw/skip branches was worse and is not retained.
- `auto_position` tests the formation slot's low bit directly in its
  `650 : -650` conditional. Comparing the masked value with one instead
  produces a branch and byte operations. The direct condition emits the
  original word mask, `SBB`, mask, and addition, recovering sixteen bytes
  and the complete mnemonic sequence. Only the formation-count comparison
  has reversed operands. Reversing that source comparison or retaining a
  separate slot local did not change it. Selecting a sign and multiplying
  by 650 restores the size but leaves an extra multiplication.
- `InitializeModalTextPanel` takes an eight-byte `ShortRect` by value and
  three short colour arguments. The adjacent default-bound words are one
  rectangle, not two independent integer arguments. Its chained rectangle
  assignment produces an eight-byte compiler temporary; separate `memcpy`
  calls omit that temporary and shorten the function. The reconstructed
  panel and viewport retain flat fields, accessed as a `ShortRect` for this
  aggregate copy. `ShowModalTextPanel` saves the default rectangle before
  allocating the panel but passes the global rectangle to its first
  initialization, reloading its bounds after allocation.
- Write signed arithmetic at its source level when the instructions implement
  division. `ShowModalTextPanel`'s `width * 8 / 16` generates the original
  shift, `CDQ`, bias, and arithmetic shift. Manually expressing the bias lets
  CL simplify the sequence differently and changes register allocation.
- The disk-prompt bounds at `0x005A7D80` are likewise one `ShortRect`.
  `PromptInsertNumberedDisk` copies them into both viewport rectangles with
  two DWORD loads and four stores, then reloads them after selecting or
  allocating the background viewport. Splitting them into independent
  halfword stores adds code absent from the original. The initializer in
  `sound.c` writes the same rectangle's top, left, bottom, and right fields.
- Empty compatibility functions can still take arguments. The text-mode
  disk prompt pushes `2` and `10` for `ResetDiskPromptTimerHook`, then cleans
  eight stack bytes. Preserve these arguments even though the hook is a
  bare `RET`.
- Distinguish a message pointer from the literal it addresses.
  `ReportPacketLoadError` reloads three initialized pointers in `disk.c`.
  It restores `nPacketError` immediately after `LogMemoryUsage`, before
  selecting an operation label. Keeping its packet-size local before the
  error word also reproduces the original EDI/BP register assignment.
- `FetchDiskPacketRetrying` retains the file-table index across retries,
  but reloads `pDiskFileRecords` after every `FreePacketAndClear` call.
  Use the indexed field at each `PacketLoad` call; caching the name pointer
  removes those loads and shortens the function. Its initial packet-size
  comparison is signed (`JLE`), and retry exhaustion is `retries <= 0`
  (`TEST`/`JLE`), rather than a comparison against one. The first error call
  passes the flags word already held in a register, even though that path
  has established that its value is zero.
- `OpenPacketSection` keeps the directory-entry offset across its two reads,
  adding four before the second. Recomputing it from the section index omits
  a stack slot and changes the register lifetimes. Mask the entry offset
  once before the last-section branch, retain the compression word's `0xff`
  mask, and clear the next entry's high byte through its byte representation:
  the original uses `MOV BYTE`, not a full-width `AND`. These choices recover
  its 24-byte frame and 320-byte span at 99.05% similarity, with eighteen
  differing instructions out of 105. The file-size and section-end slots
  remain exchanged, and the section guard and final stores still differ.
  Reversing the guard's source branches added sixteen bytes and reduced the
  match to 80%; that trial is not retained.
- `DecompressPacketSection` tests the nonzero format-flags path first and
  keeps its fallback-allocation flag as a byte, matching `TEST BL,BL`.
  The caller-supplied-workspace path reads its size local independently into
  EDI and EBP after applying the input-size override. A volatile size local
  preserves both original reads; an ordinary local or chained assignment
  merges them into a load and register copy, shortening the aligned span by
  sixteen bytes. This is a matching declaration, not evidence of the original
  source spelling. The complete 752-byte span now has matching mnemonics,
  with twelve differing instructions out of 222: its packet and size slots
  remain exchanged, and two initialization stores are scheduled differently.
  Making the packet pointer or `OpenPacketSection`'s file size volatile did
  not change their code and is not retained. Reordering declarations, using
  DWORD header locals, and changing the scratch pointer's element type also
  failed to recover the remaining layout.
- The constant-zero callback at `0x004362B0` still takes the packet pointer:
  its sole caller pushes that value and cleans four stack bytes afterwards.
  `GetPacketDecompressionResult` describes its observed use, and the following
  empty callback is `FinishPacketDecompressionHook`; their former video-release
  names were unsupported. Both bodies remain exact. Keep the new callback's
  shared prototype argument unnamed: adding a parameter identifier changes
  CL 4.20 register allocation in unrelated compilation units. The declaration
  still supplies the complete pointer type, and its definition names the
  argument. Whole-project reports caught this otherwise invisible regression.
- Keyboard cursor movement passes signed coordinates to `QueueInputEvent`.
  The original sign-extends both short fields; unsigned casts generate
  extra instructions and change argument preparation. Keep the observed
  clamp of 320 for both coordinates, including the vertical coordinate.
- Preserve switch dispatch and shared action paths. `BarracksScreen` has
  jump tables for event types and menu regions, plus a key switch. Keyboard
  activation and mouse clicks reach the same flag assignment; cursor events
  and unhandled events go directly to the common redraw. Its frame-timer
  result is tested in AX even though the timer function returns a full int.
- `RecRoom` likewise uses switches for event types, keyboard activation, and
  menu regions. Keep its byte keyboard-click flag separate from the short
  room-refresh flag. Only the two pilot regions test their shape pointers;
  Shotglass enters the shared conversation directly. Recompute the campaign
  roster index after the first pilot's loading and formatting calls, and
  write through the base-label pointers. The second pilot's sentinel test
  deliberately still reads the first pilot's animation ID, as in the binary.
  Preserve the separate character-mask tests and the shape-pointer recheck
  after obtaining the first pilot's bounds; copy the resulting viewport
  rectangle unconditionally. Restart Shotglass's animation through a byte
  local, and advance each animation pointer inside its draw-call argument.
  Saving the frame first changes register scheduling. These source forms
  reproduce the complete 2,976-byte function span, including both switch
  tables, under the relocated function comparison.
- `ShowChalkBoard` initializes only the text context's two colour bytes,
  assigns its existing modal viewport, and installs the text buffer after
  loading the font. It neither clears the whole context nor copies the screen
  viewport, and its rank lookup has no clamp. Its exit flag is a byte set to
  zero before sorting. During sorting, retain the candidate pilot's short
  index but reload the current slot when swapping; retaining both indices
  extends register lifetimes and adds a spill. These changes recover the
  original function size and 98.51% mnemonic similarity. The sorting address
  calculations and context-initialization scheduling still differ; cached
  score expressions, reversed comparisons, and extra cached indices did not
  resolve them. Removing the viewport copy also removes the extra `stScreen`
  read reported by the global-access audit.
- `IsRoomMenuLabelEmpty` returns a byte: the original uses `SBB AL,AL`
  and `NEG AL`, and `RefreshRoomMenuLabel` tests AL after its call. Declare
  the predicate as returning `char`; its zero/one result does not establish
  signedness. Read `pszCurrentRoomMenuLabel` through a volatile-qualified
  pointer lvalue to produce the original direct memory comparison against
  one; the ordinary expression first loads EAX and compares that register.
  The predicate's complete 16-byte span and its caller's 48-byte span now
  match. This qualified access is a matching choice, not proof of original
  source spelling or asynchronous updates. Plain and unsigned byte results,
  explicit return branches, a named byte result, and logical negation did not
  remove the extra load without that access qualification.
  Qualifying the global pointer declaration also matches those functions,
  but binary-comp's declaration parser omits a pointer qualified after its
  asterisk, reducing the linked-data audit from 1,097 globals to 1,096.
  That declaration form is not retained. Qualifying only the predicate's
  read preserves the global's ordinary declaration and verification coverage.
- `DecodeShapeFrame` receives a full-width height argument and narrows its
  maximum coordinate locally. Its caller sign-extends the height before
  pushing it. Read the two signed coordinate words into short locals before
  promoting them, advancing the command pointer after each read. The row
  command is an ordinary unsigned short, not volatile; the original tests
  the loaded register without an intervening memory reload. Clipped copy
  lengths are signed shorts. Adjust the length before calculating the left
  skip, and advance the destination and X coordinate inside each encoded-run
  branch. CL merges the common tail while preserving separate run-length
  promotions. Initialize both maximum coordinates before the command pointer.
  Keep the encoded runs' clipped length and right edge in their shared
  encoded-row block. The raw-row branch has its own right edge and an inner
  clipped-length word. These independent lifetimes recover the original
  destination-offset register copy, the 640-byte span, and the encoded-run
  clipping registers. The current result is 97.96% mnemonic similarity with
  eight differing instructions out of 196, all in raw-row clipping. Giving
  each of the three copy paths separate length, right-edge, and skip locals
  instead reaches 98.98% similarity but differs in 93 instructions; the byte
  comparison, not that higher mnemonic score, decides which form to retain.
- `PrepareShapeRLEData` serializes the four short extents and four integer
  bounds with individual copies through an advancing byte pointer. Taking
  the bounds' addresses preserves their four stack slots. Copy the frame
  count and final prepared-shape pointer from their locals as well: direct
  typed stores lose the pointer's stack slot and change the offset-table
  clear from the original `REP STOSD` to a sequence with a byte remainder.
  The copies restore the 44-byte local frame and 656-byte function span at
  97.70% mnemonic similarity. Local slot ordering and register scheduling
  remain different. Reordering the prepared pointer's declaration did not
  help; assigning the initial cached-shape lookup to it adds an absent store.
  Removing the unused header pointer also changes allocation in the later
  `RestoreSpriteBackground` and `GetShapeFrameCorners` routines. Keep checking
  the complete unit rather than retaining an unused declaration to influence
  the compiler.
- `DrawFontGlyph` reloads the font through its text context after writing the
  palette translation entries and again when advancing the cursor. Retaining
  the initial font pointer across those writes omits original memory reads.
  On the viewport's top row, load the row-offset word once, test its high bit,
  and add its unsigned value only when that bit is clear. A direct memory-bit
  test followed by another offset read produces a different branch sequence.
  Keep the row and background initialization before the font-colour reads.
  Load the viewport pointer before those font-colour bytes as well. With the
  two following diagnostic functions now delimited separately, the original
  span is 512 bytes; the current rebuilt span is 528 bytes.
  Initialize the existing character-data pointer from the font before loading
  the viewport, and use it for the two initial font-colour reads. Reload it
  after writing the palette entries, as before. This retains the second
  font-colour byte in a register for the translation-table store.
  Each row loop now reads the viewport's row-offset table into a local pointer
  before its top-row test, matching the original shared field load. Together
  with the decoder's corrected scopes, this reduces the previous 544-byte
  span by sixteen bytes and preserves the palette-marker's exact match.
  Mnemonic similarity falls from 91.62% to 89.02%, but the byte comparator's
  differing-instruction count also falls, from 115 of 167 to 111 of 164.
  A single function-scope row-table pointer retains the excess 32 bytes;
  separate column locals worsen allocation. A volatile background byte
  restores the previous mnemonic score but introduces a different stack slot
  and more differing instructions. Those alternatives are not retained.
  Register allocation and scheduling remain different. Keep the font's
  split-byte offset in the source-pointer expression; a separate offset
  local also perturbs later functions. Reversing pointer-addition operands
  or adding a row register hint emitted the same code. Using the argument
  directly as the row reduced the frame below the original 24 bytes;
  reusing the source pointer for the character lookup introduced a spill.
- `GetTransformedShapeBounds` returns a short through an integer intersection
  flag. Its null-shape branch assigns the point-in-viewport comparison to that
  integer before returning; this emits full-width constant results, whereas
  direct short returns change the instruction width. Read all four signed
  frame extents before scaling. The first zero-coefficient corrections use
  increments; the second pair assigns one. Keep the transformed left edge as
  an int until the short comparison and output store: narrowing the whole
  calculation early replaces the original 32-bit subtraction sequence with
  word operations. These forms recover 87.25% mnemonic similarity at the
  original 432-byte span and eight-byte local frame. The remaining coefficient
  scheduling and temporary allocation did not improve with separate scaling
  statements, in-place span sums, accumulated products, or reuse of the Y
  argument for the top edge.
- `CaptureSpriteBackground` and `RestoreSpriteBackground` also read both
  coordinate words into short locals before promotion. Keep separate command
  pointer advances for the two words. Restoration calculates the run end
  inside the second horizontal-bound test; capture retains its preceding
  assignment. These forms preserve the original aligned function sizes.
  The original
  spills the run length while retaining clipping values in registers; this
  allocation remains unresolved. Narrowing the run-length declaration's
  scope, adding a register hint to the skip, caching promoted bounds, and
  duplicating the cursor updates did not improve the match.
  Separating screen-pointer initialization from its offset addition also
  emits the same bytes, as do register hints on both clipping locals.
  Computing the run length before the command-type branch adds a byte copy
  and moves the row count into a register, worsening both functions. These
  trials are not retained.
- `rotate_about_j` multiplies the forward component by sine before multiplying
  the saved right component by cosine. After correcting graphics-workspace
  visibility, CL evaluates these addition terms in the opposite order.
  Swapping the source terms does not change its output. An explicit product
  local restores call order but reduces the stack frame from eight bytes to
  four and worsens the function-size mismatch, so it is not retained.
- `ReadTextInput` dispatches both the input byte and the input mode with
  switches. This retains the zero-extended key for character classification
  and places Enter/Escape and uppercase/numeric branches after their defaults.
  Save the text pointer before the viewport pointer, then copy the viewport
  through `pCurrentTextContext` after installing the input buffer. Using the
  cached viewport pointer omits an original reload. The key itself holds the
  translated character; no separate character temporary is needed.
- `UpdateSceneAnimationObject` returns a short and reads frame commands as
  signed bytes, promoting them only for the draw call. Cache the object's
  layer across each drawing loop, increment the byte stop flag, and let the
  `J` command fall through to `G`'s label search. Keeping an opcode local for
  a later `J` test adds a branch and changes the switch layout. Advance the
  label pointer separately over its command and value. The object-copy command
  narrows its scene-count product before adding the object index at full width;
  narrowing the final sum is different from the original. OR every goal result
  into the completion word. These choices restore the 12-byte local frame and
  1,552-byte span at 98.96% mnemonic similarity. Register allocation, initial
  scheduling, and one comparison operand order still differ. A register hint,
  moving stop initialization past cursor selection, and reversing the label
  comparison did not improve the match.
- `PlaySceneAnimation` writes the packet base into each object's script pointer
  before adding the signed script offset. Combining those writes omits an
  original store. Its three object loops test the object count before loading
  their countdown locals; guarded `do` loops reproduce this. Decrement a
  nonzero wait count before the completion branch, and test the timer result
  as a short. The complete 560-byte span now has the original instruction
  sequence; 15 instructions still differ in loop-register allocation.
- `SceneDirector` uses one record loop. When a conditional test selects another
  record, restart that loop before checking Escape; a nested source loop
  instead makes CL retain the shot and spill the duration, adding repeated
  loads. Initialize the record and prior-state locals before assigning the
  scene-type global. Use `abs` for the objective index, save the selected shot
  before clearing the face in the common cases, and update the cached text
  colour before looking up its palette word. Keep constellation initialization's
  shot assignment after its two global stores so that branch retains its own
  face reset. These choices restore the 1,136-byte span and all mnemonics;
  only two equality comparisons retain reversed operands. Register hints,
  declaration reordering, unsigned shot storage, and a label-based restart
  did not improve the result. The unit change also reverses two comparison/
  branch pairs in the previously exact `PanToScreen` (four of 183 instructions).
  Its source is unchanged: reversed source conditions and guarded palette loops
  did not restore the earlier encodings. Continue checking the entire unit.
  Binary-comp separately verifies all six switch tables in this director and
  the animation interpreter: 60 relocated targets, their HIGHLOW entries, and
  210 literal selector/padding bytes match at one address shift per function.
- Keep the observed form of record copies. In `LoadGame`, a byte source
  pointer and direct destination array fields retain the four original
  counted loops. Caching the destination base adds instructions; naming
  both arrays directly lets CL replace the loops with bulk copies. A byte
  pointer for the 17-byte description copy also preserves the interleaved
  loads and stores in the loader and both save writers. In the writers,
  advance the runtime source pointer before the disk destination pointer:
  reversing that order moves the destination increment ahead of its store
  and introduces an extra displacement byte.
- Calculate the bunk row before `DrawBarracksBunks` prepares its draw-call
  arguments. Keeping the division inside the argument expression occupies
  AX with the frame number and changes both coordinate indexing and push
  order. A short row local restores CX for the frame and the original size.
- A shared call in the executable can come from calls in both source branches.
  In `intro_drawbackgroundships`, separate sprite calls let CL 4.20 prepare
  the common colour and frame arguments before selecting the shape. Selecting
  the shape first, including with a conditional argument, moves those pushes
  after the branch and does not reproduce the original instruction order.
- `your_internal_damage` likewise uses direct component-damage calls in
  each switch branch. CL merges their common call and stack cleanup while
  retaining the original constant pushes; assigning component and amount
  locals before a shared source label introduces register moves instead.
  Reject the wrong quadrant before handling drive damage, and use the
  `quadrant == 1 ? 3 : 1` selection for the alternate damage table. The
  first hull-damage event decrements the event count unconditionally before
  calling `pilot_hit`. The random weapon index is narrowed to signed char
  and then sign-extended to the short argument passed to `remove_weapon`.
- A typed loop filling a short array can compile to `REP STOSD` followed by
  `REP STOSW`. `Title_Sequence` uses this to mark unused menu options as -1;
  a byte-oriented `memset` produces a different remainder sequence.
- Preserve a shared sentinel write after a variable-length list is built.
  `process_player_input` writes one or two keys through an advancing pointer,
  then appends -1. Writing a separate sentinel in each switch case changes
  the original common tail.
- `player_input` saves the polled modifiers to their global before copying
  them to a local. Its later keyboard-event cases do not overwrite that
  global. Sign-extend the key for its switch, and retain the original nesting
  of the mouse-button fire checks, including the apparently contradictory
  tests after the previous-key check. The queued button case updates its
  timestamp after that conditional, on both paths. Calculate both mouse
  magnitudes once as signed shorts before their threshold loops; comparing
  promoted `abs` results instead emits different arithmetic and increases
  the frame. Handle the cockpitless branch first, switch on the signed view
  byte, and clamp at >=9/<=-9. These changes recover the 28-byte frame and
  raise similarity from 82.32% to 94.09%; sixteen excess bytes remain.
  Global-load scheduling, store merging, and register allocation still
  differ. The unchanged next-view selector also becomes exact under the
  unit's resulting allocation; no previously exact function loses its match.
- `init_intelligence_data` takes its canned-sequence pointer from the
  existing 32-bit pilot-level slot before replacing that slot with 2. Reading
  it again from the mission record adds an index calculation absent from the
  original. The native build retains the full pointer from the mission record
  because its pointer cannot fit in the legacy slot. Test pilot levels >4
  before the default rating branch. The resulting 336-byte span matches the
  original length at 93.33% similarity; register choices and vector-copy
  scheduling still differ.
- `draw_nav_pointer` stores its object index and clears the old screen X and
  distance after the creation branch, including when the object already
  exists. The original thus discards the previous projection before every
  visibility check. Reload the global object index for the projected X, Y,
  and distance stores, and narrow the masked screen width to a short before
  shifting it. This restores its 512-byte span at 96.27% similarity. Creation
  scheduling and a missing saved register still change stack offsets and
  encodings; equal size is not an exact match.
- Signedness matters even for 32-bit fields. In `IxStream`, `pending` and
  `lockLen` are signed counts, while the consumed-byte delta is unsigned.
  The original tests `pending` with a signed comparison and updates it
  through a load/subtract/store sequence. Treating all these values as
  unsigned lets CL 4.20 replace that sequence with different memory arithmetic.
- An `/Od` format check can retain a jump through an empty default branch.
  `ix_dsps_alloc` has two such branches for its default eight-bit and mono
  formats. Omitting the `else` bodies removes ten original code bytes.
- Local names affect CL 4.20's `/Od` stack allocation. In
  `ix_mixer_thread_proc`, declaration reordering alone did not recover the
  frame. The descriptive names `primaryCaps`, `resultCode`, `bufferDescriptor`,
  `outputFormat`, and `soundCaps` place the real locals at EBP offsets -20,
  -24, -44, -60, and -156 respectively, matching the original. These names
  reproduce the allocation; they are not recovered original spellings.
  The former layout needed longer displacement encodings, adding 27 bytes.
  Binary-comp now reports the complete 1,180-byte function as exact, without
  changes to types, fields, flags, or instructions written by hand.
- `ix_mixer_service` has separate initial-fill and refill-loop locals. Keep
  the initial buffer, byte count, result, and delay outside the loop, with
  the two refill buffers, counts, cursor positions, buffered-byte count,
  result, and requested-byte count inside it. The request is assigned `0x16f8`
  before the lock call. The two success-result writes before and after the
  loop are absent from the original. The refill locals need separate scopes
  too: second-byte count and play cursor at EBP -20/-24; result, hardware
  write cursor, and buffered-byte count at -28/-32/-36; first-byte count and
  first-buffer pointer at -40/-44; and requested-byte count and second-buffer
  pointer at -48/-52. In that innermost scope, declare the pointer before
  the requested count. Keep the cursor query and delay calculation before
  the buffer scopes, and write the cursor-wrap test as
  `playCursor > dwMixerWriteOffset`. Reversing that test introduces an extra
  instruction byte and changes subsequent branch offsets. The scopes recover
  allocation; they do not prove the original block structure.
  Binary-comp now finds one differing instruction out of 262 instead of 35,
  preserving the original 52-byte frame and 932-byte span at 99.62%
  similarity. The remaining final wrap check loads the write offset before
  comparing the buffer size and uses `JA`; the original loads the size,
  compares the offset, and uses `JB`. Relocation-aware comparison masks the
  two global addresses, so it counts only the branch opcode as different.
  Reversing or negating that final comparison did not change its encoding.
  Alternative descriptive local names and flattening the refill scopes into
  one declaration block also failed; those probes are not retained.
- Preserve the observed order of calls inside comparisons. The audio-service
  loops call `ix_thread_get_audio_chunk_size` before `ix_dsps_get_buffer_free`;
  putting the chunk-size call on the left of `<` reproduces the original
  call order and unsigned branch direction under the library profile.
- The message pump retains a `POINT` cursor position alongside its `MSG` and
  `RECT`, giving a 52-byte local frame. Two independent cursor integers let
  CL overlay their uninitialized slots and shorten the frame. Its minimized
  flag is read again immediately after being set; `volatile int` preserves
  those accesses and the original loop branch instead of sinking the store.
  The minimized-state test follows the `IsIconic` branch, and the minimized
  process priority is `IDLE_PRIORITY_CLASS` (`0x40`).
- Keep the full-width arguments passed by `MainWindowProc` to
  `QueueInputEvent`. Its key value is the original `WPARAM`; its mouse
  coordinates have already been extracted from the message's low/high words.
  Additional short casts introduce zero extensions absent from the original.
  A separate `HANDLE` result from `GetCurrentProcess` also preserves the
  original call-before-priority-push order in the window procedure and pump.
- Both joystick pollers shift the full 32-bit button sample before narrowing
  the second button's mask to a word. Narrowing first emits `SHR AX` instead
  of `SHR EAX`. Store the host secondary-button byte before the primary byte.
  `PollMenuInputDevices` also writes each selected coordinate bound on both
  paths: conditional expressions assigned back to the fields reproduce these
  stores; conditional stores do not. Its change flag is sign-extended from a
  byte for the queued event. These forms recover its 672-byte span at 97.35%
  mnemonic similarity; `PollJoystickButtonEvents` reaches 96.08%. The menu's
  initial zero register, movement temporary, and cursor-copy scheduling remain
  different. A separate horizontal-step local and reordered cursor assignments
  did not improve them and are not retained.
- `LoadJoystickCalibrationFile` keeps both promoted input ranges in 32-bit
  locals through the scale divisions and coordinate bounds. This recovers the
  original register pressure and scale reloads. Choose the adjusted calibration
  bounds in an `if`/`else`, rather than writing the centres first, and increment
  each zero scale. Together these choices restore the 784-byte span at 98.35%
  similarity. The only call to `SampleBothJoysticks` pushes signed -1, while
  the callee reads its fallback as a word. A signed-short fallback parameter
  reproduces both; the callee remains exact. Its final conversion to the
  unsigned fallback of `GetJoystickPosition` preserves the low 16 bits.
- `ReadCalibratedJoystick` reads Y inside the short-circuit failure condition,
  after X passes its check. Chain the three sample-field clears to retain the
  original shared zero register. Compare the minimum X bound with the sampled
  field so CL loads the bound before storing the raw coordinates. These changes
  reach 87.60% and remove sixteen excess bytes; sixteen remain. Explicit cached
  failure/centre locals, long raw-value locals, reversed failure comparisons,
  and folding the device assignment into its test did not recover the remaining
  register-held comparisons and divisors.
- Check neighbouring functions after input changes. The unit's allocation
  changes also affect `ReserveContiguousPaletteEntries`. Reusing its count for
  the fill loop restores the original branch direction and all mnemonics, but
  nine of 39 instructions now differ at byte level, versus three before these
  input changes. Initial zeroing and fill-loop registers remain different;
  restoring the menu's cached index did not resolve them. No previously exact
  function loses its exact match in this change.

- Startup's animation-demo index and mission index occupy distinct word
  slots. The `w` option writes the former; the `m` option writes the latter.
  Keep an argument-slot pointer through the option switch: the original
  reloads the string pointer after `strcmp` and the version-print call.
  The normal campaign loop falls through before the direct-mission path.
  These choices recover the 20-byte frame and raise `main` from 82.95% to
  96.62%; sixteen excess bytes and allocation differences remain.
- `UpdateDamageDisplay` saves the prior component count in a word local
  before counting damaged components. Comparing the promoted new byte count
  with that saved word reproduces the original `MOVSX`/word comparison and
  ESI save. Comparing the two byte values directly loses those instructions.
  Store the new count before the comparison, then store it again on the
  unchanged-count path. Putting the first store inside the changed-count
  branch lets CL merge the two stores and loses sixteen bytes. The counting
  loop has its own signed-byte index, separate from the component selected
  for display later. This recovers the initial BL index and ECX constant
  without changing the later selection loop. The complete 1,072-byte span
  now compares exactly.
- `AllocateNearHeapBlockByFlags` reuses its descriptor pointer when examining
  the last block; those pointer lifetimes do not overlap. A separate last-block
  local changes common-subexpression handling, adds an alignment spill, and
  enlarges the local frame from four bytes to eight. Keep the masked block
  size signed and retain the raw size-and-flags value separately for the
  allocation-bit test. Declare the masked size before the raw value to recover
  their ECX and EDX allocation. These forms match the entire 464-byte span,
  including alignment. Reversing the heap-base/size sum, introducing explicit
  loaded-value locals in `GetNearHeapBlockSize`, and removing redundant casts
  in the end allocator did not resolve their remaining differences; those
  trials are not retained.
- `replenish_shields` promotes its short ship argument to a retained integer
  index after the damaged-power-plant guard. Use that index for the shield
  and object-type arrays. This recovers the original 176-byte span and the
  shield-update loop; nine instructions still differ in the prologue, guard,
  and initial argument promotion. Moving the shield-value locals
  to function scope worsens the loop allocation. A separate recharge divisor,
  reversed index/counter declarations, and a parameter `register` hint do not
  resolve the guard. These trials are not retained.
- After the sound-flush declaration correction, `DecodeShapeFrame` became
  sixteen bytes short, before the clipping-scope correction described above.
  Grouping the destination offset, separating the bitmap
  assignment from its addition, and retaining an explicit row offset do not
  recover the original extra register copy. Explicit promoted clipping bounds
  also leave this function unchanged and perturb later graphics functions.
  After the scope correction, a separate raw-row right-limit integer and
  reversing the row-length/X addition still leave its eight differing
  instructions unchanged. These trials are not retained.
- Shared declarations affect optimized CL 4.20 register allocation even in
  units that do not use those declarations. Removing damage's private tables
  and former named literals from `globals.h` changes unrelated functions:
  `DIBsetPalette` becomes exact, while `ComputeVectorDelta` exchanges its
  source and destination registers and loses its exact match. Check the
  complete report after correcting header visibility. The damage split gives
  57 newly exact functions and 55 formerly exact functions with differences;
  its verified data layout does not imply every code comparison improves.
- Correcting the sound-flush declaration likewise changes unrelated
  allocation: the complete report gains 27 exact functions and loses 16,
  including the previous target-lock and cockpit-damage matches. Four
  targeted routines recover their original sizes, but `DecodeShapeFrame`
  and `replenish_shields` become sixteen bytes short and
  `AllocateNearHeapBlockByFlags` becomes sixteen bytes long. The net result
  is eleven additional exact functions, one fewer size mismatch, and
  32 fewer bytes of total absolute size error. These remaining differences
  must be resolved in source; they are not reasons to omit original arguments.
- `AppendCommMenuChoice` compares existing choices through the global count,
  then saves that count's post-increment result in a 32-bit index for the two
  shared stores. This preserves the original branch-specific count loads and
  gives an exact 128-byte span. A short index copied before the shared stores
  let CL hoist its load across the reuse branch, removing seven instruction
  bytes and shortening the aligned span by sixteen. The earlier explicit
  `else`, shared-store label, and `register` trials did not fix that lifetime;
  keeping the post-increment at the index capture does. No other function's
  report changes with this correction.
- `prepare_mission` retains the player's mission index as a word through
  `load_ship`, `set_objects_data`, and `Set_up_ship_info`. Use that index for
  the mission-record fields rather than caching a record pointer and later
  reloading the global index. The initial-wingman loop reads only the low
  word of each record's pilot value, then sign-extends it for `is_alive`.
  A `short` local reproduces that load and both pilot-range comparisons.
  The carrier search writes its current index in the limit test as well as
  after incrementing it. Keeping both observed stores restores its 416-byte
  span at 96.23%, up from 85.98% and a sixteen-byte deficit. Separate player
  and loop indices preserve the other functions' allocation; reusing one
  local perturbed three neighbouring functions. Four scheduling differences
  remain, and fourteen instructions still differ at byte level.
- `free_ship` indexes `aObjectResourceSlots` and `aObjectTypeData` directly
  instead of retaining pointers to their selected records. The original
  keeps the resource byte offset on its stack and the type byte offset in
  EDI, adding the array bases at each use. Direct typed subscripts reproduce
  this allocation and the reloads, restore sixteen bytes, and give an exact
  comparison for the complete 432-byte function.
- `BuildMap` likewise indexes mission objectives directly instead of
  retaining a record pointer. Hazard dispatch is a switch with asteroid
  and mine cases. Initialize the navigation-point pointer immediately
  after `SetTextContext` in source: CL schedules its load just before
  `InitializeTextContextFromFont`, matching the original. Initializing
  before text setup or in the loop header leaves scheduling differences.
  These choices recover 32 bytes and match the complete 928-byte span,
  including its switch tables and alignment.
- `init_ship` retains the record's full-width object type through its calls.
  Its normal and hazard paths form an `if`/`else`, with an immediate return
  after successful ship setup and a shared final return for failed setup or
  a hazard. The hazard path never initializes the returned short: the original
  reads its stack slot at `0x0040C937`. Preserve that native behavior; the
  SDL branch retains its existing explicit -1 result. The dead-pilot test
  compares against eight with `JG`, rather than nine with `JGE`. Together
  these choices recover the 16-byte frame, saved EBP, and complete 336-byte
  function exactly.
- Removing the cached pointers in `free_ship` also changes the formation
  helper's first array-index calculation: its formation-index and spot loads
  exchange registers. `set_formation_position` loses its exact match with
  three differing instructions; its size and mnemonic score remain unchanged.
  Reordering its locals, caching its spot, and renaming its record parameter
  did not recover the original load order. These trials are not retained.
- The launcher's missing sixteen bytes are still unresolved. The original
  holds `'-'` in EBX and uses immediate zero stores; CL instead holds zero
  in EBX. Moving the command local outside the loop or using `strcmp` did
  not change that choice; a direct conditional-expression switch was worse.
  Volatile zero stores and an explicit leading default case also leave the
  same output; neither probe is retained.
  Similarly, reversing comparison/addition operands or renaming the stream
  parameter did not alter the one-byte differences in the `/Od` DSP stream
  functions. These trials are not retained. Distinguish such body differences
  from the last function's linker padding: `ix_dsp_free` and
  `ix_thread_get_audio_chunk_size` already have matching instruction bodies,
  but their object-end alignment contributes size differences to the report.
- The two game-side audio-list removal routines retain comparison-operand
  differences despite 100% mnemonic scores. Reversing equality operands,
  separating declaration order from initialization order, and changing the
  parameter names to `wave`/`sound` all produced the same instructions.
  An explicit pointer to the pilot-table slot likewise did not improve
  `add_statistics` and changed allocation in neighboring functions.
  These trials were reverted; binary-comp confirmed that the restored
  executable was identical to the previously verified build.
- `PostMission` divides the saved kill count by five before incrementing the
  mission count. Its badge tests compare directly against five and 25. The
  wingman's statistics call passes an immediate one; the alternate call
  passes the random mission count and a conditional kill count. Keep the
  random kill result unsigned through argument promotion: the original
  emits `MOVZX EAX,AX`. These choices recover the complete function, including
  its switch tables and deliberate first-mission fallthrough.
- The mission-score routines declare the objective index before the score
  pointer and initialize the accumulated score before calculating that
  pointer. Both now compare exactly. `UpdateSeries` calls
  `PlayersMissionScore` first, retains that word result, then directly
  compares the result of `FullMissionScore` with it. The successful-series
  branch precedes failure, with immediate flag stores in each branch.
  Compare the saved short ship type against the full-width ship enum rather
  than narrowing both operands. This restores its missing sixteen bytes
  and gives an exact comparison.
- `GameFlow` skips both debriefing and the promotion rank increment after
  ejection; the original jump at `0x0040F71B` lands at `0x0040F7CE`. A failed
  promotion roll still reaches debriefing at `0x0040F7AE`. Keep promotion
  eligibility as one short-circuit expression and the ending dispatch as
  a switch with direct calls. The generic ending passes `!bSeriesFailed`:
  the original `CMP`/`SBB`/`NEG` sequence produces one for a zero flag.
  Declaring the flown mission before the flown series also recovers the
  original series register and mission stack slot. The 1,168-byte span
  and all mnemonics now match; twelve instructions still differ because
  the preflight zero and minus-one registers are exchanged.
- `free_viewport` searches its allocation table using `viewport->allocation`
  while retaining a separate allocation local through the release calls.
  CL eliminates the repeated field read but keeps the original register
  copy and search-loop allocation. This reduces differing instructions
  from 50 to six at the same size; loop comparison direction and four
  equality-operand orders remain different. Reversing their source operands
  did not change the output. Explicit additions instead of compound
  assignments in `add_statistics` likewise did not improve its allocation;
  those trial forms are not retained.

- The AIFF and WAV sample loaders retain the file pointer in the outer scope
  and declare their end and chunk pointers in the subsequent chunk-walking
  block. This reproduces their original -4, -8, and -12 stack slots under
  `/Od`; merely reordering the outer declarations does not. Keep explicit
  default/break cases in both switches. Their otherwise unreachable jumps
  are present in the original spans and account for five bytes in each.
  The AIFF payload's byte-swap loop reads unsigned words. Its channel-count
  swap masks after the left shift, while the bits-per-sample swap masks
  before the right shift. These expressions recover the different original
  byte-register sequences; cast the channel count to unsigned before the
  left shift to preserve the operation for negative input without signed
  shift overflow. Both complete loader spans, 1,263 and 953 bytes, now match.
  The AIFF expression correction also restores the following WAV loop's
  comparison order. A `for` loop, reversed comparison operands, alternate
  pointer casts, and an additive byte-swap form did not improve these
  routines and are not retained.
- The extended-number converter masks the exponent's second input byte
  with `0xFF`, producing the original `MOV CH,CL` and direct low-byte load.
  Its signed-result dispatch has an explicit `else`, preserving the
  unreachable jump after the negative return. Keep the low mantissa in the
  outer scope, the exponent in the next scope, and the value, high mantissa,
  and byte pointer in the innermost scope. Their EBP offsets are then -4,
  -8, -16, -20, and -24, matching every original local slot without adding
  storage. These scopes reproduce allocation; they do not prove the original
  source's block structure. Binary-comp's explicit 378-byte body comparison
  improves from 45 differing instructions to 10 of 116, with unchanged
  length and 100% mnemonic similarity. The remaining differences are the
  mantissa byte-load and shift order: the original high word reads offsets
  2, 3, 4, 5 and its low word reads 8, 7, 6, 9.
  Regrouping OR expressions, volatile input reads, unsigned-int casts, and
  reordered low-word terms did not improve those accesses. Additive assembly
  of the words increased the count to 16; signed input bytes with explicit
  masks added 36 body bytes. These trials are not retained. Its reported
  size also includes following import stubs and alignment;
  `docs/EXPORT.md` records the separate body comparison.
- `close_behind` keeps its range parameter in memory by qualifying that
  parameter as volatile. This recovers the original global load into AX and
  the complete 32-byte span; the shared prototype and calling convention
  are unchanged. The qualifier is a matching declaration choice, not proof
  of the original source spelling. The remaining `CMP` and conditional jump
  have reversed operands and direction. Reversing the source comparison,
  with or without the qualifier, does not fix them. A saved target-range
  local also failed and perturbed `scan_for_enemy`; it is not retained.
- `set_up_screen_viewport` selects static geometry with switch cases four
  and five. CL emits the original lower-bound `JL`, upper-bound `JLE`, and
  packet-geometry fall-through; equivalent if/goto forms place the static
  geometry first. Store the height directly in `nScreenHeight`, then divide
  that word for the vertical center. This keeps the geometry pointer in ECX
  and the height in DX, eliminating the extra ESI save/restore and recovering
  the original 336-byte span. Both screen dimensions already have short
  declarations, so their stores need no pointer casts. All mnemonics now
  match; the origin-Y load/store pair in each branch still uses CX where
  the original uses DX. A separate origin-Y local does not alter that result.
- `sort_viable_target_list` still lacks the original spill of the next outer
  index. A volatile short plus chained inner-index assignment recovers an
  eight-byte frame, but places the word at a different offset, caches an
  extra distance pointer, and leaves the span sixteen bytes short. Explicit
  candidate-distance storage does not change the retained code; ordinary
  nested for loops change register allocation without recovering the spill.
  These trials are not retained.
- `get_player_input` copies each twelve-byte input sample with an inlined
  `memcpy`. This also recovers the earlier guard's original load, test,
  increment, and store, raising similarity from 86.44% to 96.61% without
  changing its 224-byte span. Eleven instructions still differ: comparison
  operand direction, early guard-decrement scheduling, and the copy's final
  load/store order. Explicit guard, destination-pointer, and sampled-field
  locals did not improve the result and could perturb neighbouring functions.
  Reversing comparison operands or moving the early decrement after the copy
  also failed to recover the original sequence; those trials are not retained.
- The flight-control mouse coordinates passed to `SetMousePosition` stay
  32-bit through the division and increment. Narrowing them to `short` first
  inserts conversions absent from the original. The target-lock key case
  handles a new press before the repeated-key `else` path. Together with
  restricting the later event-modifier ejection fallback to SDL, these changes
  recover `HandleSpaceFlightControls`' 3,184-byte span at 95.69% similarity.
  The native ejection path tests only `GetControlKeyState`'s result; the
  fallback added in commit `e28f61c` remains available in the modern port.
  Register allocation, viewport-mode increment arguments, and several cached
  global loads still differ. Reversing the previous/current-key comparison
  did not recover the original initial byte register and is not retained.
- `UpdateArcadeScoreDisplay` tests both countdowns with `<= 0`, preserving
  the original zero comparison and `TEST AX,AX` after decrement. Using `< 1`
  emits different jumps and an extra instruction byte that increases the
  aligned span by sixteen bytes. The complete 224-byte function now matches.
- `objective_lost` indexes its mission record directly, without copying the
  objective parameter or type into redundant locals. Its own comparison is
  unchanged, but the resulting unit allocation restores exact matches in
  `target_locking` and `place_damage_on_cockpit`. A cached record pointer and
  a widened index did not improve the objective routine. Neither did a signed
  state local with a nonzero test. In `onboard_explosion`, direct array access,
  explicit promoted indices, a cached collision radius, and moving the byte
  offset calculation all retained the incorrect twelve-byte frame and reduced
  similarity. These trials are reverted; its original frame is sixteen bytes.
- `select_new_release_weapon` retains the starting cycle index as a signed
  word, even though its running index and weapon count are signed bytes.
  The original sign-extends DL into DI and later compares AX with DI.
  Correcting that one local raises similarity from 87.88% to 94.90% while
  preserving the 336-byte span. The preferred-type search still reuses a
  cached count where the original reloads memory, and several comparisons
  have reversed operands. Changing the search to `do`/`while` or using plain
  `char` for the count did not help; reading the count through a signed-byte
  pointer or moving the current-weapon capture into its test made it worse.
  Those trials are not retained.
- `DrawScrambleActor` retains the script-control mask as an unsigned byte
  and adds the base frame to its signed-byte frame before sign-extending the
  draw argument. A 16-bit frame sum omits the original byte wrapping.
  Each screen coordinate combines its background offset with the actor's
  compound position update. This recovers the exact coordinate and draw-call
  sequence and raises similarity from 81.94% to 97.22%, with the original
  256-byte span. Fourteen instructions still differ because the script index
  and control mask exchange BL and DL, moving the initial index load past
  the scale calculation. Descriptive local renames and removing the frame
  mask's redundant cast did not help; a promoted actor-index local was worse.
  Separate world-coordinate and screen-coordinate additions also gave a
  different schedule and are not retained.
- `evaluate_damage` still moves the front-armour contribution after the
  other armour terms. A short running accumulator, explicit word casts on
  each quotient, and changing the source addition order all produced the
  same differing instruction sequence. These trials are not retained.
- `wing_status` saves the current campaign mission number in an int local
  after its no-death guard, then uses it in both comparisons. Repeating the
  arithmetic in each condition makes CL reverse the shared comparison and
  final branch. The saved value recovers the complete 64-byte span exactly.
- `AllocateViewport` calculates width directly from the two viewport fields
  before saving the left edge. This keeps the original `SUB CX,AX` and delays
  the left-edge stack store until after the display-mode comparison. Pass
  the clear-colour word unchanged to `ClearViewport`; narrowing it to a byte
  adds an absent `MOVZX AX,AL`. These two changes retain the 320-byte span,
  raise similarity from 84.16% to 87.13%, and reduce differing instructions
  from 99 to 55 of 101. The original also reloads the row-table field in its
  loop and uses a signed, initially guarded comparison against height plus
  two. Applying those forms together changed register allocation and added
  sixteen bytes. Moving the offset initialization before the second allocation
  improved scheduling but did not recover the size. Widening top increased
  the local frame; widening height, removing the temporary allocation pointer,
  and removing unsigned promotion casts did not improve the retained result.
  These broader trials are not retained.
- Earlier constellation probes removed the initializer's promoted
  particle-index local, used separate short counters, or initialized height
  before width. None recovered its saved height and register-held particle
  counter; the storage and cache correction above does. Narrowing the draw routine's
  random index to a short introduced a stack spill and reduced its match.
  These trials are not retained. Negating the combined valid-index condition
  in `ix_dsps_prepare` also left its reversed comparison unchanged.
- A second switch in `animate_shape` reproduces the original 32-bit scale
  command tests and raises mnemonic similarity to 99.04%, but is not retained:
  it changes the following `hit_asteroid` from exact to nine differing
  instructions and enlarges `object_collision` by sixteen bytes, including
  its extra four-byte spill. The animation's scale stores still merge where
  the original keeps two stores, and the object index and type exchange
  registers. Saving a real scale-change word improves the delay-register
  allocation but leaves those regressions. Promoted indices, explicit delay
  assignment, subtraction and product rewrites, default-case order, a
  descriptive type-local rename, and typed animation-array access do not
  resolve them. A guarded decrement loop and a register hint in `hit_asteroid`
  also leave its exchanged registers unchanged. The previous reconstruction is
  retained with only the independently proven unsigned-scale correction.

- Giving `objective_lost` an explicit mission-ship index, as either an int
  or a short, leaves its allocation unchanged. Reversing the command/opcode
  equality in `FindSceneAnimationCommand` likewise leaves its compared-byte
  order unchanged. Saving `nTargetRange` in a short before `Mveer_away`'s
  proximity test does not change its reversed loads and comparison. These
  trials are not retained.

Do not hand-write thunks, vtable dispatch, exception tables, or other compiler
glue. `make audit-compiler-glue` enforces this. A real function annotation must
immediately precede its definition; validate annotations with
`make audit-addresses`.

## Edit loop

```sh
make compare-func FUNC=FunctionName
make report FILTER=source_name
make verify
```

Stop at 90% similarity unless a clear evidence-backed improvement remains.
Run the Win32 build through DREAMM rather than Wine; the original depends on
Windows 95 DirectDraw and DirectSound behavior.
- The one-byte comparison differences in `ix_dsp_set_master_volume` and
  `ix_thread_handle_file_chunk` survive volatile global reads, volatile local
  indices, and long indices in place of int indices. None of those probes
  changed their comparison order or size, so they are not retained.
  `ix_sound_is_playing` also has a matching instruction body; its reported
  four-byte shortage comes from object-end alignment, like the thread and
  DSP final functions discussed above.
- `ReadRequiredPilotField` still loads and pushes its Y argument through EDX
  where the original uses EAX. An unsigned cursor-X local produces the same
  output; moving the X expression directly into `SetTextCursor` worsens the
  comparison from two to eight differing instructions. Both probes are
  reverted, preserving the original load scheduling recovered so far.
- Reversing the comparison operands in `ix_dsp_set_master_volume`,
  `ix_thread_handle_file_chunk`, `ix_thread_service_streams`, and
  `ix_system_find_free_voice` does not improve their current output.
  Declaring the master-volume loop index in the `for` initializer and
  naming the free-voice index `voiceIndex` likewise leaves their comparisons
  unchanged. A volatile input-pointer local in the extended-number converter,
  combined with original-order low-mantissa terms, still leaves ten differing
  body instructions. These probes are not retained.
- A volatile byte-offset local in `onboard_explosion` recovers a sixteen-byte
  local frame but drops the original saved EBP, leaving its effective stack
  slots wrong. Byte differences increase from 49 to 58 of 61 instructions
  despite the mnemonic score rising from 81.97% to 88.52%. Assigning that
  offset within the velocity-copy address expression still leaves 58
  differences and drops similarity to 75.41%. Neither probe is retained.
  Limiting the volatile read to the later position calculation still drops
  the saved EBP and leaves 58 differences. Chaining the offset store into
  a real velocity offset, caching the radius before the vector copy, and
  moving the offset calculation after the scale update do not improve the
  original 49-difference result. The latter restores four saved registers
  but hoists the debris index before `set_objects_data`, leaving 54
  differences. An inlined `memcpy` emits the same copy as struct assignment.
  These further probes are also reverted.
