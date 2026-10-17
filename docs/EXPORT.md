# Original-code exports

`code-full/` is the comparison input. Files are keyed by entry address, for
example `FUN_0041D0C0.disassembled.txt`, regardless of the function name.

For an annotated function, export directly from the retail PE:

```sh
make export-asm
make export-asm EXPORT_ASM_FLAGS=--discover
make export-asm EXPORT_ASM_FLAGS='--clean --discover'
```

This path uses `/* Function start: 0xADDR */` annotations and produces the
disassembly needed by `compare-func`, `report`, and `order`.
Keep `--discover` when overriding the default export flags: discovered CRT
and thunk boundaries also delimit the byte spans of adjacent source functions.

Keep `nop` out of `padding_mnemonics` in `config/checkValuesCapstone.json`.
The original high-score routine contains a reachable `NOP` at `0x00426BAD`.
Filtering it creates a gap in the decoded control flow and drops the following
18 instructions, including calls to `GetHighScoreEntry` and `DrawFormattedText`.
Two CRT routines also contain reachable NOPs. The current policy filters only
`int3`; normal return and function-boundary handling excludes trailing padding.
Run `make export-asm` after changing this policy to refresh existing exports.

The seeded Ghidra list and automatic discovery both miss unreferenced routines.
`config/original-boundaries.map.txt` preserves verified entry points that lack
source annotations, using the MSVC map format accepted by `binary-comp`.
It is recovered metadata, not an original linker map. `make export-asm` loads
it even for clean exports. Remove an entry when its source annotation supplies
the boundary, so the export takes the implemented function's name.

The map also retains four compiler-generated DSP array initializer entries.
They deliberately have no source annotations and are checked separately by
`make verify-array-initializers` (included in `make verify`). The function
report therefore counts 1,521 source entries after removal of their four
handwritten stand-ins and recovery of the two graphics diagnostic functions
`__copyline` and `__wipeline`; the separate gate covers the 74 generated
code bytes.

`make verify-unit-data`, also included in `make verify`, checks the complete
initialized-data sections recovered for barracks (632 bytes), fade (2),
debug (34), disk (523), ship (286), init (550), logic (447), trainsim (511),
damage (390), view (289), ending (457), input (6), comm (520), video (10),
joycal (42), killbrd (1,508), and gr (2,030). It also checks gr's complete
56-byte read-only section, including the registry key and value arrays and
their alignment. The read-only block must retain its COFF storage flags.
It delegates byte matching to binary-comp, allowing one address shift per
block, and verifies object sizes, global positions, and pointer relocations.
String-content fallback is disabled so duplicate or empty literals cannot
conceal incorrect pointer targets. Resource pointers outside their unit must
resolve to the named global at the original field offset. These checks prove
relative layout; final absolute addresses remain a whole-executable task.
The default check also verifies the seven adjacent units from barracks through
logic as one 2,487-byte run, including all 13 inter-object alignment bytes.
It also checks damage and view as a 681-byte run, including their two alignment
bytes, and ending through joycal as a 1,050-byte run, including 15 alignment
bytes. The room and graphics units form another 3,542-byte run, including
four alignment bytes. These checks catch correct individual blocks linked
at incorrect relative positions.
The same gate checks the uninitialized blocks owned by pilot (4 bytes),
sound (8), and damage (4). These must be genuine COFF `.bss` sections with no raw data
or relocations, and their linker symbols must belong to the expected object
at the recovered relative offsets. Binary-comp compares the loader-visible
zero-filled bytes. These checks establish local storage and layout;
they do not claim that the surrounding uninitialized region matches.
`make verify-barracks-data` retains the barracks-only check.

Three explicitly marked partial-section checks cover `game.c`: two scalar
state runs of 112 and 21 bytes, and the 6,652-byte run of scripts, mission
records, maneuver tables, and display literals. Each permits only one address
shift and checks its declarations and relocation set. They also require the
current containing COFF section size of 6,788 bytes. The unresolved four bytes
between the two state runs prevents a complete-unit layout claim; these
checks do not silently treat that gap as matched.

Two more partial-section checks cover `screens.c`: its 288-byte character-data
run and its 970-byte medal, animation, and literal run. They verify all 27
declarations and 14 pointer relocations and require a 1,258-byte containing
COFF section. The intervening 40 zero bytes remain unrecovered, so these
checks do not establish a complete-unit match. The second run includes the
full 50-byte debriefing movement table and all 262 bytes of literal storage,
with each repeated format at its original relative position.

`config/data-literals.h` contains comparison metadata for anonymous literal
spans. It is never compiled or included by game code. The data-check targets
append it to `out/globals-data-audit.c`; source-level global audits continue
to use the separate `out/globals-audit.c`. Binary-comp's unique-byte-sequence
lookup locates the complete disk literal pool and verifies pointers into it,
including three-character graphics-mode names and the empty string. Its
ordinary string-content fallback cannot resolve those short strings.
The damage literal pool similarly establishes the two-character `"Ok"`
severity string's position; the separate unit-data check enforces all offsets.
Four separately checked spans cover 408 bytes of used music literals and
the following 80 bytes of screen diagnostics. They preserve original newlines
and trailing spaces; the three unused literals between these runs remain
unrecovered. The 296-byte sound-descriptor table is checked directly through
the source declaration. These partial checks do not
establish the complete music section or its absolute placement.

The DirectDraw metadata span covers all 3,620 bytes from its first frame-rate
format through its last error-name terminator. This checks the relative order
of duplicate diagnostics as well as their text and alignment. Its eleven
initialized variables are checked individually; two unresolved zero words
still prevent a complete-unit layout claim for `dib.c`.

LINK 4.20 omits private C data symbols from its public map. The build preserves
that map as `out/WC1.link.map`, then `bin/appendStaticSymbols.py` copies it to
`WC1.map` with the four private damage symbols, seven private view symbols,
five private music symbols, eight private ending symbols, two private
comm symbols, two private joycal symbols, ten private dib symbols,
eighteen private game symbols, seventeen private screens symbols,
twenty-two private killbrd symbols, and three private gr symbols added.
Their addresses come from the object's COFF symbols, public section anchors,
and linked `DIR32`
references with PE `HIGHLOW` relocations. Conflicting anchors or unresolved
symbols fail the build. This step changes only comparison metadata; it does
not modify the executable or infer addresses from the original binary.

The room literal metadata covers all 328 bytes from the one-space label
through the final chalk-board format. It resolves short and empty strings
by their positions in that complete pool. The kill-board unit gate checks
these same bytes together with the initial campaign/pilot records, animation
sequences, menu state, all intervening alignment, and all 24 pointer targets.
The source initializer audit serializes `ShortPoint` as two signed halfwords;
its size alone is insufficient to distinguish the coordinate fields.

The graphics literal metadata covers 334 bytes after the palette translation
and absolute sine/cosine tables. It includes the four-character shape version
and the streamer's separate empty string. The unit gate checks this entire
pool and all three tables together as one 2,030-byte initialized section.

Keep address annotations outside initializers so the linked-data inventory
can see them. For the 7,830-byte object-type table, the annotation precedes
the declaration: a semicolon in an interior comment ends the small inventory
parser's initializer scan early. The display-name pool and all 21 animation
arrays also have external annotations. Binary-comp locates the complete
1,064-byte animation run uniquely, resolving the duplicate short sequences
by their positions within the run. This enables full table and resource-field
pointer checks without adding comparison-only declarations to game code.

A per-function `EXACT` result covers only that entry's
span up to the next exported function, even if the next function has no source
implementation. Cross-check `make compare-functions` address shifts before
treating the report as complete code coverage. Review discovered entries
against the raw bytes: a jump table can also look like an instruction stream.

For literal identity of the complete files, including headers and relocations:

```sh
binary-comp byte-compare --fail-on-difference data/full/WC1.ORI.EXE WC1.EXE
```

For a complete refresh, run binary-comp's
[`ExportToCompile.java`](https://github.com/gg-sl-oss/binary-comp/blob/main/ghidra_scripts/ExportToCompile.java)
from Ghidra and point it at `code-full/`. That export also preserves Ghidra
names and writes the decompiled files, `globals.h`, and `strings.txt` required
by call/global audits.

| Command | Required input |
| --- | --- |
| `make compare-func`, `report`, `order` | Disassembly exports, linked `WC1.EXE`, `WC1.map` |
| `make verify-calls` | Decompiled exports too |
| `make verify-globals` | `code-full/globals.h` too |
| `make compare`, `compare-functions` | Retail and rebuilt executables |
