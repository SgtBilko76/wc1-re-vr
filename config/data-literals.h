/* Comparison metadata only; this file is never compiled or included by the
 * game. These declarations describe contiguous literal pools and table runs,
 * not additional named globals. binary-comp locates each complete byte sequence
 * uniquely in the rebuilt image and uses its range to verify pointers into it.
 *
 * disk.c: graphics-mode names, packet-error messages, the unreferenced
 * "(OVR)"/empty strings, packet-load tags, and both disk-insertion prompts.
 * The range includes the final prompt's terminating zero and all internal
 * alignment bytes, but excludes the following object's linker padding.
 */
unsigned char abDiskStringLiterals[459]; /* 0x004696a8 */

/* nav.c: mission names, map labels, legend formats, and viewport titles.
 * The complete unit gate also checks the three distinct "err" literals and
 * all 26 pointer targets, including the shared colour variables. */
unsigned char abNavStringLiterals[360]; /* 0x00468758 */

/* title.c: opening text and all nineteen credit strings, including their
 * internal alignment. The separate Secret Missions array is a named global.
 * The complete unit gate also checks the opening-text and credit pointers. */
unsigned char abTitleStringLiterals[1062]; /* 0x00468adc */

/* joystick.c: the five calibration prompts and three separate j.cal literals.
 * The unit gate resolves this literal-only section from COFF and linked code
 * relocations and verifies its position between title and space state. */
unsigned char abJoystickCalibrationLiterals[238]; /* 0x00468f04 */

/* cockpt.c: the writable objective-message literal, missile-lock message,
 * weapon/navigation labels, arrival messages, and final empty HUD string.
 * The partial unit gate checks all offsets and both stored string pointers. */
unsigned char abCockpitStringLiterals[293]; /* 0x0046928c */

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

/* dib.c: the complete literal pool, from the two frame-rate formats through
 * the DirectDraw error names. Includes repeated diagnostics and all internal
 * alignment bytes; the initialized state preceding it remains incomplete. */
unsigned char abDIBDiagnosticLiterals[3620]; /* 0x0046b1d8 */

/* killbrd.c: rank names, mutable room labels, and chalk-board formats.
 * The complete pool resolves the one-space label and distinct empty strings;
 * verify-unit-data also checks every offset and pointer in the whole unit. */
unsigned char abRoomStringLiterals[328]; /* 0x0047052c */

/* gr.c: shape, raster, streamer, and registry diagnostics, including the
 * four-character shape version and the streamer's separate empty string.
 * The graphics unit gate checks their complete layout after the tables. */
unsigned char abGraphicsDiagnosticLiterals[334]; /* 0x00470d18 */

/* The shared direction-frame, cockpit-light, bar, layout, and pilot-hand
 * tables, followed by ten unidentified -400 words. These remain in globals.c
 * until their original ownership is established. This run includes alignment
 * between the tables; it excludes the unresolved gaps before its first table. */
unsigned char abSharedRendererTableRun[1572]; /* 0x0046db28 */
