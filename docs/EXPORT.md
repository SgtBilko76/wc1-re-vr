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
report therefore counts 1,519 source entries after removal of their four
handwritten stand-ins; the separate gate covers the 74 generated code bytes.

`make verify-unit-data`, also included in `make verify`, checks the complete
initialized-data sections recovered for barracks (632 bytes), fade (2),
debug (34), disk (523), ship (286), init (550), logic (447), trainsim (511),
damage (390), view (289), ending (457), input (6), comm (520), video (10),
and joycal (42).
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
bytes. These checks catch correct individual blocks linked at incorrect
relative positions.
The same gate checks the uninitialized blocks owned by pilot (4 bytes),
sound (8), and damage (4). These must be genuine COFF `.bss` sections with no raw data
or relocations, and their linker symbols must belong to the expected object
at the recovered relative offsets. Binary-comp compares the loader-visible
zero-filled bytes. These checks establish local storage and layout;
they do not claim that the surrounding uninitialized region matches.
`make verify-barracks-data` retains the barracks-only check.

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

LINK 4.20 omits private C data symbols from its public map. The build preserves
that map as `out/WC1.link.map`, then `bin/appendStaticSymbols.py` copies it to
`WC1.map` with the four private damage symbols, seven private view symbols,
five private music symbols, eight private ending symbols, two private
comm symbols, and two private joycal symbols added. Their addresses come
from the object's COFF symbols, public section anchors, and linked `DIR32`
references with PE `HIGHLOW` relocations. Conflicting anchors or unresolved
symbols fail the build. This step changes only comparison metadata; it does
not modify the executable or infer addresses from the original binary.

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
