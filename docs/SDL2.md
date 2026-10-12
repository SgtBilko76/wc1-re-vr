# SDL2 port

The native target reuses the reconstructed game and `ix` audio library.
`src/sdl/` owns host windows, input, timing, paths, audio, and presentation;
native objects stay under `out-modern/` and never enter the MSVC comparison
build.

WC1 still renders a 320x200 indexed frame. Both backends present it in a
centered 4:3 viewport and use that viewport for pointer mapping. `--enhanced`
selects the optional OpenGL 3.2 space-object renderer; the original software
path remains the default and fallback.

Kilrathi Saga data is fully supported. DOS support includes compressed packet
resources, `MUSIC.MID`, `WINGLDR.TIM`, AdLib music, and synthesized OriginFX
effects. In-flight sampled and synthesized effects use camera-relative stereo
panning; synthesized effects retain distance falloff.

Kilrathi Saga wave effects receive 6 dB of mix headroom so full-scale and
overlapping weapon sounds do not mask the streamed music. The DOS OriginFX
path retains its separate synthesized music-and-effects mix.

Development builds use ASan and UBSan. Release builds select
`MODERN_RELEASE=1`.

The GUI also recognizes the Super Wing Commander Mac demo. Its experimental
Enyo 1 session includes startup, the animated title menu, carrier conversations,
briefing, combat, autopilot, landing, and endings. SDL2 presents the Mac artwork
and audio while WC1 supplies shared flight and mission logic. OpenGL rendering,
joystick layouts, rumble, and cockpitless view work with this path too.
`make modern-swc` builds the separate asset viewer. See [SWC.md](SWC.md) for
setup, controls, and remaining work.

## Graphical launcher

`make modern-gui` builds `out-modern/wc1-modern-gui` (`.exe` on Windows).
The launcher opens by default without arguments. With arguments, use `--gui`
to show it before startup; otherwise the existing command-line path is used.
`make modern` continues to build the SDL-only executable.

The Slint window follows the WC2 launcher: title banner, installation picker,
graphics and gameplay controls, joystick controls, and Cancel/Launch buttons.
The controls map to:

| Control | Command-line equivalent |
| --- | --- |
| Enhanced OpenGL | `--enhanced` |
| EGA 16-colour dithering (WC1 only) | `--ega` |
| Cockpitless view | `-c` (campaign flight; the training simulator keeps its cockpit) |
| Cheat mode (invulnerability and no collisions) | `--cheats` |
| Rumble | `--joystick-rumble` |
| Control mode | `--joystick-mode=original`, `4button-2axis`, or `4button-4axis` |
| Axis layout | `--joystick-axes=` with a layout listed below |

The axis selector is enabled for four-axis controls. Saved settings seed the
controls, explicit command-line options override them, and accepted choices
take precedence. Options are applied after the dialog so EGA and rumble can
be unchecked. WC1's SDL frame-rate text output
is a stub, so the launcher does not expose `-f`. WC2's balanced-difficulty option
has no WC1 counterpart.

For WC1, the selected directory must contain `GAMEDAT/MODULE.000`, `CAMP.000`, and
`INSTALL.DAT`. Validation reads byte 7 of `MODULE.000`, matching `SdlUsingDosData`:
1 identifies DOS compression, while other values identify Kilrathi Saga's raw
packets. Saga data also requires readable `PREFLITE.STR`, `POSFLITE.STR`, and
`MISSION.STR` in the sibling `STREAMS` directory. Names are matched without
case sensitivity. These are startup checks, not a complete game-data audit.
Invalid selections disable Launch; folder-picker errors stay in the dialog.

For SWC, select the **SuperWing DEMO** directory containing `CMFs/Data.CMF`,
`Spaceflight.CMF`, and `PCShipV00.CMF`. Keep the executable's native resource fork
or place its extracted raw fork at `SuperWingCommanderDemo.rsrc` in that directory.
The GUI checks archive headers and palette availability, identifies the Mac demo,
and starts its title menu, carrier visit and Enyo 1 flight. OpenGL, all joystick
modes and axis layouts, rumble, and cockpitless view are available. EGA dithering
uses WC1's DOS palette conversion table, so it stays disabled for Mac data.
`--swc-demo DIRECTORY` selects SWC from the command line, including when the
directory also contains WC1 data. With `--gui`, it preselects that directory and
keeps SWC selected. Otherwise the GUI gives `GAMEDAT` priority over `CMFs` in
mixed directories, and the command-line launcher defaults to WC1. Game selection
belongs to the launcher; shared WC1 mission loading never probes SWC archives.
The runtime `--check` modes and GUI auto-launch/snapshot automation have been
removed. Passing `--check` reports an error before opening a window.

For example, the demo accepts the same enhancement switches:

```sh
./wc1-modern --swc-demo "/path/to/SuperWing DEMO" --enhanced \
    --joystick-mode=4button-4axis --joystick-rumble
```

SWC keeps its 320x240 logical coordinates and centered 4:3 viewport, including
high-DPI windows. Its default flight renderer uses the recovered software
rasterizer. `--enhanced` selects SDL's OpenGL backend and draws ship sprites
and VDU polygons at output resolution with cached textures and nearest sampling;
scenes and movies use the same SDL renderer. This is separate from WC1's
indexed-frame OpenGL backend. An unavailable OpenGL driver reports a startup
error. EGA dithering is ignored when `--swc-demo` is selected.

`src/sdl/slint/` is a separate CMake project using C++20, CMake 3.21+, and Rust
1.88+. It pins Slint 1.16.1 and Native File Dialog Extended 1.3.0 and links them
statically with the existing native game objects. Linux needs Fontconfig and
D-Bus development packages, plus a working XDG desktop portal for browsing.
Windows retains WC2's static Slint import fix. On macOS, the folder picker is
initialized after the native event loop starts so SDL can take focus on launch.
The GUI changes the working directory only on acceptance; cancellation exits
before SDL or the game is initialized.

Release jobs build this integrated executable and package it as `wc1-modern`.
The MSVC reference executable and comparison flags are unchanged.

### Cheat mode

The GUI's Cheat mode checkbox is available for DOS, Kilrathi Saga, and the SWC
Mac demo. It enables the original developer unlock, prevents player damage,
and disables collision response involving the player. `--cheats` enables the
same combination without the GUI; with `--gui`, it checks the box initially
and can still be unchecked before launch. SWC reapplies the choice whenever
the mission is recreated, including New Game after title-screen combat.

The original instruction evidence is `main` at `0x004274e0`, which recognizes
the case-sensitive `Origin` argument and then accepts `-k` (no damage) and
`-b` (no player collision response), and `CheckLauncherAndConfig` at
`0x00401d10`, which sets all three flags when `ReadCheaterFlagFromRegistry`
(`0x00442600`) reads a nonzero `Cheater` value. The GUI uses this latter
combination. WC1 keeps its original `CHEATER` callsign behavior when creating
or loading a pilot with the unlock enabled.

SWC's original CODE_05 `inflict_damage` +`0x0d14` and CODE_02
`object_collision` +`0x4c48` also guard player damage and collision response
with separate flags. The SDL demo uses their existing WC1 equivalents.
Only the SDL launcher and SWC host change; the Win32 reference code is untouched.
Legacy command-line, `WINGCMDR.CFG`, and registry-emulation cheat settings
remain independent: unchecking the GUI option does not remove those settings.

### Launcher settings

Clicking Launch saves the absolute game directory and all graphics, gameplay,
and joystick controls in `launcher.ini`. The next GUI launch restores them;
Cancel or closing the window leaves the saved settings unchanged. The INI lives
alongside the SDL port's existing `wc1-modern.cfg`, in the per-user directory
returned by `SDL_GetPrefPath("Origin Systems", "WC Kilrathi Saga")`:

| Platform | Default INI path |
| --- | --- |
| macOS | `~/Library/Application Support/Origin Systems/WC Kilrathi Saga/launcher.ini` |
| Linux | `~/.local/share/Origin Systems/WC Kilrathi Saga/launcher.ini` |
| Windows | `%APPDATA%\Origin Systems\WC Kilrathi Saga\launcher.ini` |

On Linux, `XDG_DATA_HOME` replaces `~/.local/share` when set. The file uses UTF-8
and one `[launcher]` section:

```ini
[launcher]
game_directory="/absolute/path/to/WC1"
enhanced_renderer=0
ega_dither=0
cockpitless_view=0
cheat_mode=0
joystick_rumble=0
joystick_mode=0
joystick_axes=0
```

Boolean options use `0` or `1`. `joystick_mode` uses `0` for original, `1` for
four buttons/two axes, and `2` for four buttons/four axes. `joystick_axes` uses
`0` through `6` in the order of the axis-layout table below. Directory values
must be absolute; surrounding double quotes are optional, and backslashes are
literal. Blank lines and lines beginning with `;` or `#` are ignored.

Missing files, unknown keys, and invalid values leave the corresponding
defaults in place. A remembered directory is checked normally before Launch is
enabled. Saving replaces the INI after writing a temporary file; a write failure
is reported on standard error and does not prevent launching. Delete the INI
to reset GUI preferences.

Explicit arguments such as `--joystick-mode=original` or `--swc-demo DIRECTORY`
take precedence over saved values. The GUI can still change those defaults
before launch. SWC selection is detected from the directory unless explicitly
requested with `--swc-demo`; the INI does not force a game type. Direct launches
without the GUI do not read or write this file.

## Joystick input

`--joystick-mode=original` preserves the original two-axis, two-button layout.
The `4button-2axis` and `4button-4axis` modes add WCAT-style controls. The
four-axis mode accepts:

| `--joystick-axes=` | Extra-axis behavior |
| --- | --- |
| `twin-stick-roll` | Extra X rolls; extra Y is relative throttle |
| `twin-stick-yaw` | Extra X yaws; extra Y is relative throttle |
| `hotas-yaw` | Primary X rolls; rudder yaws; final axis is throttle |
| `hotas-roll` | Primary X yaws; rudder rolls; final axis is throttle |
| `linear-throttle` | Third axis is throttle; keyboard controls roll |
| `rudder-yaw` | Third axis yaws |
| `rudder-roll` | Third axis rolls |

Use `--joystick-debug` for event diagnostics and `--joystick-rumble` for
supported feedback. The README documents the common controller bindings.
These modes also apply to SWC. In its menus and scenes, the D-pad browses,
A confirms, B/Back goes back or skips, and Start pauses. See
[the demo controls](SWC.md#run) for its flight bindings.
Escape and P toggle pause during demo gameplay. Escape skips cutscenes and
goes back from carrier panels; P pauses cutscenes. Backspace also handles menu
back and scene skipping. Its window title stays **Super Wing Commander**.

## Commands

```sh
make modern
make modern-gui
make run-modern
make run-modern-dos
make run-modern-mission SERIES=1 MISSION=0
```

The Windows v0.0.12 crash offset `0x1a9da` reported in
[issue #14](https://github.com/neuromancer/wc1-re/issues/14) is the object-class
lookup in `free_ship` (original entry `0x0040BC70`). The carrier-return
sequence frees slots 1 and 2 even when a nav transition has marked them empty
with type `-1`. Native builds must skip that sentinel before converting it to
an enum and indexing the object-type table. Capital ships still require
cleanup when their `shapeSet` is null. The subsequent `landing` animation
(original entry `0x00408650`) must index its canopy and damage-comment tables
as pointer arrays, whose entries grow from four to eight bytes on 64-bit
hosts. Both fixes are confined to `SDL_PORT`.
