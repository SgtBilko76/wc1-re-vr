# Matching patterns

The original disassembly and extracted strings are authoritative. Decompiled
code, operational labels, and other releases are only leads. Use `binary-comp`
for similarity decisions.

## Source choices that affect matching

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
- Preserve visible local-variable order, branch direction, fall-through, and
  epilogue placement. Equivalent C often emits different instructions.
- In `/Od` code, keep explicit pointer locals when the original stores them.
- Repeat identical literals at each use site; string pooling was disabled.
  Distinguish literals from named arrays: `CreateMainWindow` references the
  same writable class-name array three times, plus a separate title array.
  Four independent literals reproduce the text but not the original storage.
  Both low-memory branches in `LoadOriginFxDrivers` likewise reference one
  100-byte writable message array before its memory-reservation tables.
  Repeating the message at the call sites adds a duplicate to the literal
  pool and loses the original declaration order.
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
  definitions. The overlay pointer in `pilot.cpp` and the three private wave
  and damage-display variables in `sound.c` naturally produce `.bss` in
  declaration order. Keeping them in the temporary C globals unit instead
  produces linker common symbols with different positions. Restore proven
  ownership and verify object storage as well as zero values; zero-filled
  memory by itself cannot establish a matching layout.
- Put a global's address comment before its declaration or after its final
  semicolon. A comment inside a multiline initializer is recognized by the
  layout audit but missed by the linked-data comparison's simpler parser.
  Put it before the declaration when interior comments contain semicolons,
  as in `aObjectTypeData`; those can terminate the simpler parser's scan.
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
  it beforehand or caching the view index changes allocation and shortens
  the routine. The frame swap saves the empty frame first. Direct indexing
  recovers sixteen missing bytes, but the viewport bound reads and register
  allocation remain unresolved. Chained field assignments and reusing the
  percentage argument did not improve the match; copying the frame bytes
  with `memcpy` also enlarged the local frame and is not retained. Removing
  the length local leaves this function's instructions unchanged but alters
  register allocation in later mission functions. Check the entire unit
  when changing locals, not just the edited function.
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
  These choices recover most of the routine; its current 96-byte frame still
  exceeds the original 92 bytes because the compiler spills the mass pointer.
  An explicit mass pointer, a `register` qualifier, and a shared scale local
  did not recover the original allocation and are not retained.
- Check argument widths at callers even when the callee ignores an argument.
  `GetTransformedShapeBounds` takes a short flip value: widening it to `int`
  adds a sign extension in `easy2see` that the original does not contain.
  The solid-colour sprite functions likewise take a short colour value even
  though `SetSolidColourTranslation` uses only its low byte. Their callers
  extend byte colours into AX before pushing them.
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
  These forms recover the original 640-byte span and 94.90% mnemonic
  similarity; 34 of 196 instructions still differ in the byte comparison,
  principally in clipping registers and scheduling.
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
- `CaptureSpriteBackground` and `RestoreSpriteBackground` also read both
  coordinate words into short locals before promotion. Keep separate command
  pointer advances for the two words. Restoration calculates the run end
  inside the second horizontal-bound test; capture retains its preceding
  assignment. Together these forms recover restoration's original size and
  reduce the pair's differing instructions to 157/220 and 161/224. Capture's
  mnemonic score falls slightly despite fewer byte differences. The original
  spills the run length while retaining clipping values in registers; this
  allocation remains unresolved. Narrowing the run-length declaration's
  scope, adding a register hint to the skip, caching promoted bounds, and
  duplicating the cursor updates did not improve the match.
- `ReadTextInput` dispatches both the input byte and the input mode with
  switches. This retains the zero-extended key for character classification
  and places Enter/Escape and uppercase/numeric branches after their defaults.
  Save the text pointer before the viewport pointer, then copy the viewport
  through `pCurrentTextContext` after installing the input buffer. Using the
  cached viewport pointer omits an original reload. The key itself holds the
  translated character; no separate character temporary is needed.
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
- Signedness matters even for 32-bit fields. In `IxStream`, `pending` and
  `lockLen` are signed counts, while the consumed-byte delta is unsigned.
  The original tests `pending` with a signed comparison and updates it
  through a load/subtract/store sequence. Treating all these values as
  unsigned lets CL 4.20 replace that sequence with different memory arithmetic.
- An `/Od` format check can retain a jump through an empty default branch.
  `ix_dsps_alloc` has two such branches for its default eight-bit and mono
  formats. Omitting the `else` bodies removes ten original code bytes.
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
