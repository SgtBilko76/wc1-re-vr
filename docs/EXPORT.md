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

`make verify-barracks-data`, also included in `make verify`, checks the whole
632-byte initialized-data block and the object section's size. It delegates
byte matching to binary-comp, allowing one address shift for all six internal
menu pointers. String-content fallback is disabled so duplicate literals
cannot conceal incorrect pointer targets. This verifies relative layout;
the block's final absolute address remains a whole-executable matching task.

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
