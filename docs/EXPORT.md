# Original-code exports

`code-full/` is the comparison input. Files are keyed by entry address, for
example `FUN_0041D0C0.disassembled.txt`, regardless of the function name.

For an annotated function, export directly from the retail PE:

```sh
make export-asm
make export-asm EXPORT_ASM_FLAGS=--discover
make export-asm EXPORT_ASM_FLAGS=--clean
```

This path uses `/* Function start: 0xADDR */` annotations and produces the
disassembly needed by `compare-func`, `report`, and `order`.

The seeded Ghidra list and automatic discovery both miss unreferenced routines.
`config/original-boundaries.map.txt` preserves verified entry points that lack
source annotations, using the MSVC map format accepted by `binary-comp`.
It is recovered metadata, not an original linker map. `make export-asm` loads
it even for clean exports. Remove an entry when its source annotation supplies
the boundary, so the export takes the implemented function's name.

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
