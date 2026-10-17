# Matching patterns

The original disassembly and extracted strings are authoritative. Decompiled
code, operational labels, and other releases are only leads. Use `binary-comp`
for similarity decisions.

## Source choices that affect matching

- Confirm arguments at call sites, return width from register use, and calling
  convention from the terminating `RET`. Core functions are normally
  `__cdecl`; `ix` methods receive the implicit object in ECX.
- Prefer `short` when the original emits 16-bit operations.
- Preserve visible local-variable order, branch direction, fall-through, and
  epilogue placement. Equivalent C often emits different instructions.
- In `/Od` code, keep explicit pointer locals when the original stores them.
- Repeat identical literals at each use site; string pooling was disabled.
- Use typed fields and arrays. Keep offset-based field names until their intent
  is proven.
- Use the matching CRT routine when the binary calls or inlines it.
- Check the original section before adding `const` to a table. The hazard and
  explosion-debris tables occupy writable data even though their reconstructed
  users only read them; the barracks menu regions occupy read-only data.
- Put a global's address comment before its declaration or after its final
  semicolon. A comment inside a multiline initializer is recognized by the
  layout audit but missed by the linked-data comparison's simpler parser.
- Keep direct array-field accesses when the original repeatedly uses an index
  plus an absolute field address. A cached structure pointer can produce the
  same mnemonics with shorter displacements, changing the function's size.
  `ConfigureScrambleActor`, `NavMapPointInsideReservedArea`, and
  `ReserveNavMapArea` recover their original sizes with direct field accesses.
- Check argument widths at callers even when the callee ignores an argument.
  `GetTransformedShapeBounds` takes a short flip value: widening it to `int`
  adds a sign extension in `easy2see` that the original does not contain.

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
