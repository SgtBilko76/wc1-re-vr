/* Comparison metadata only; this file is never compiled or included by the
 * game. These declarations describe contiguous compiler-emitted literals,
 * not named globals. binary-comp locates each complete byte sequence uniquely
 * in the rebuilt image and uses its range to verify pointers into the pool.
 *
 * disk.c: graphics-mode names, packet-error messages, the unreferenced
 * "(OVR)"/empty strings, packet-load tags, and both disk-insertion prompts.
 * The range includes the final prompt's terminating zero and all internal
 * alignment bytes, but excludes the following object's linker padding.
 */
unsigned char abDiskStringLiterals[459]; /* 0x004696a8 */

/* damage.c: component and severity names, report headings, and both distinct
 * status-format literals. The complete pool also resolves the short "Ok"
 * pointer; verify-unit-data independently checks every position in its unit.
 */
unsigned char abDamageStringLiterals[266]; /* 0x0046a7c4 */

/* music.c: the first diagnostic-literal run, from FadeMusic through
 * flight_trigger. The following unused queue_start literal is not recovered.
 * Includes alignment within this run; the rest of the unit is incomplete.
 */
unsigned char abMusicDiagnosticLiterals[288]; /* 0x0046ab78 */

/* music.c: the second queue_start literal through the soundFX diagnostic.
 * The unused queue_start copy before it occupies a separate original span.
 */
unsigned char abMusicQueueDiagnosticLiterals[92]; /* 0x0046aca8 */

/* music.c: these two used literals have an unrecovered copy of the wave-name
 * format before each of them. Keep their ranges separate until that source
 * form is established. */
unsigned char abMusicSoundEffectDiagnosticLiteral[16]; /* 0x0046ad10 */

/* The 12-byte wave-name format is followed by 80 bytes of screen allocation
 * and memory-stack diagnostics. This complete run is long enough for unique
 * lookup and also checks their adjacency across the compilation-unit edge. */
unsigned char abMusicAndScreenLiterals[92]; /* 0x0046ad2c */
