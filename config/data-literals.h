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
