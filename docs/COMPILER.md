# Compiler contract

Reference target: `WC1.EXE`, MD5 `b20a68b7e45f837e59f7e31bab2e2020`, built
1996-09-24 with image base `0x00400000`.

The linker stamp, pre-VC5 image layout and startup, and static CRT identify
Microsoft Visual C++ 4.20. The exact executable configuration lives in the
Makefile; change it only when `binary-comp` demonstrates an improvement across
the affected compilation unit.

## Profiles

| Sources | Language and flags |
| --- | --- |
| Game core | C, `/MTd /Og /Oi /Ot /Oy /Ob1 /Gs` |
| `src/ix/` | C++, `/MTd /Od /Oi` |
| `src/sound.c` | Core profile compiled as C++ with `/TP` |
| `src/pilot.cpp` | Core profile plus target-specific `/GX` |

Evidence that constrains these settings:

- Debug-heap strings, `_CrtCheckMemory`, `_mtinit`, and `tidtable.c` prove the
  static debug multithreaded CRT (`/MTd`).
- Core functions normally omit EBP and schedule registers; `ix` functions use
  EBP, stack temporaries, and a shared epilogue.
- Duplicate literals at `0x0046B6E0` and `0x0046B71C` prove string pooling was
  disabled; do not add `/Gf` or deduplicate literals in source.
- Core operand-size prefixes reflect 16-bit DOS-era types. Use `short` where
  the original uses 16-bit operations; this is not a compiler flag.
- The console allocation wrapper proves `pilot.cpp` needs construction cleanup.
  Other units must not gain C++ exception handling.
- `src/sound.c` directly calls `ix` member functions and matches only as C++.

Linking uses `/SUBSYSTEM:WINDOWS /ALIGN:4096` without `/ENTRY`, so LINK
supplies `WinMainCRTStartup` itself. The command line lists the core objects,
`ixlog.obj`, then `kernel32`, `user32`, `gdi32`, `advapi32`, `winmm`, the five
`ix` libraries, `dsound` and `ddraw`; LIBCMTD and OLDNAMES follow as default
libraries. Base relocations remain enabled. `docs/ORDER.md` records the
evidence for each choice.
