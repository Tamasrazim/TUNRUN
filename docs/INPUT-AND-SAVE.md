# Input and Save Specification

## Input contract

Keyboard, mouse, and gamepad must work in **both gameplay and every UI screen**. Device input is translated by one Input Service into named actions. UI and flight each have a clearly active input context.

A device change updates visual prompts, but must not reset screen state, discard slider values, move selection unexpectedly, or leave a control held.

## Custom Seed Entry

Seed Entry is available from Seed Lab and the Custom Seed Run menu entry. It accepts 1–64 ASCII letters, digits, spaces, underscores, or hyphens. Text is lowercased and repeated whitespace is collapsed before a stable 64-bit hash is derived; a `0x`-prefixed value with 1–16 hexadecimal digits, or exactly 16 hexadecimal digits, is interpreted literally. The chosen non-zero root seed and run serial are saved before the run starts, so the selected course identity can be reconstructed after restart. Invalid characters, empty input, overlong text, zero, or a failed save show an inline error instead of silently substituting another seed. Keyboard typing/paste, mouse controls, and a gamepad character picker are supported.

## Default controls

Mappings are defaults and must be remappable later.

| Action | Keyboard | Mouse | Gamepad |
|---|---|---|---|
| Steer horizontally/vertically | WASD | Relative movement steers toward a persistent in-tunnel aim point while mouse-flight mode is active | Left stick |
| Boost | Left Shift | Not bound in the current prototype | Right trigger (analog threshold) |
| Energy dash | Space | Right button (optional configurable binding) | A / Cross |
| Brake / precision mode | Left Ctrl | Not bound in the current prototype | Left trigger (analog threshold) |
| Switch FPP / TPP | V | Configurable secondary button | Y / Triangle |
| Pause | Esc | UI pause button; capture ends | Menu / Start |
| Confirm / interact | Enter / E | Left click | A / Cross |
| Back / cancel | Esc / Backspace | Right click only within a UI context | B / Circle |
| UI navigation | Arrows / Tab | Pointer and click | D-pad / left stick |
| Mouse sensitivity | Left/right arrows | Click/drag the visible logarithmic slider; pointer capture is not required | D-pad left/right or left stick with rate-limited repeat |

All bindings must be configurable with conflict detection, reset-to-defaults, and clear labels for the currently active device. The mouse-sensitivity setting can be adjusted with a logarithmic Settings slider or bounded keyboard/controller steps, persists to the local profile, and has a reset-options action that does not erase ships or progression. Platform-specific face-button labels should be rendered correctly where supported.

## Mouse implementation requirements

### UI state

- The normal system cursor is visible and interactive on Main Menu, Hangar, Mode Select, Seed Entry, Settings, Pause, Results, dialogs, and exit confirmation. On Windows, Raw Input is captured and the cursor hidden/clipped only while a run is active and mouse flight is enabled.
- Every visible button must respond to mouse click. Sliders support click and drag; dropdowns, tabs, and confirmation dialogs must work.
- UI hit testing must use the same logical coordinate system as layout. DPI scale, fullscreen, borderless mode, resizing, and resolution changes must not create offset click zones.
- Hover styling is never the only indication of selection.

### Gameplay state

- Windows builds use relative mouse deltas from Raw Input (WM_INPUT). Flight steering must never recenter the cursor with repeated SetCursorPos warps.
- The native cursor is hidden over the client area and clipped to the client bounds only while gameplay mouse steering is active and the game window is focused. Clip bounds are refreshed as the window resizes.
- Mouse capture is released before pause, menu, settings, results, focus loss, or shutdown.
- A lost-focus event clears accumulated deltas and held mouse-button states before pausing.
- Returning from pause re-enters gameplay input explicitly and restores only the requested gameplay capture state.
- Sensitivity, smoothing, horizontal inversion, vertical inversion, dead-zone/response curve, and mouse-steering enable/disable are configurable.
- If mouse steering is disabled, mouse remains available for UI and keyboard/gamepad flight is unaffected. Settings exposes both the mouse-flight toggle and sensitivity slider.

The implementation must not depend on setting the OS cursor to the center every frame. The UI must never listen to relative gameplay deltas as pointer coordinates.

## Gamepad implementation requirements

- Detect at startup and support hot-plug/disconnect.
- Left stick has configurable dead-zone and response curve. Triggers are treated as analogue values.
- Menu focus is always visible and remains stable during a device change.
- D-pad and stick navigation cannot double-activate a button.
- Long holds repeat at a controlled rate; a single press invokes one action.
- Disconnect clears held actions and returns UI navigation to keyboard/mouse without crashing.
- Controller prompts update to a connected controller's layout.

## UI state rules

UI navigation state is not the same as gameplay state. Use an explicit UI screen stack (e.g. Settings opened over Pause) and an explicit input context. Closing Settings returns to Pause, not directly to the run. Pause always releases flight controls. Focus loss pauses a run and releases mouse capture.

Button activation should be edge-triggered, debounced, and idempotent. A click used to close a modal cannot also click the control behind it.

## Save location and visibility

Use a per-user local directory:

    %LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.json
    %LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.bak

Set the Windows Hidden attribute on the profile directory and profile files where supported. Hidden is a convenience, not encryption or anti-tamper protection. Save access must be limited to the current user's local profile.

## Runtime profile v3

Current writes use a strict flat JSON object with exactly 18 fields: the prior profile fields plus `bestScore`, `bestCombo`, and `checksum`. It stores display settings, mouse steering and sensitivity, selected/unlocked ships, Aether Shards, Singularity Cores, run/crash counts, best distance, career-best score and combo, root seed, and run serial. The parser rejects duplicate, missing, or unexpected fields; validates types and numeric ranges; rejects non-finite values; checks the FNV-1a checksum; and caps profile input at 64 KiB.

Schema v1 files contain 15 fields without a checksum; schema v2 files contain 16 fields with the checksum. Both are accepted only after version-appropriate parsing (including checksum verification for v2), then migrated to v3. A valid primary is copied to `profile.bak` before migration; if loading from an older backup, that backup is kept intact. Writes use a same-directory temporary file, flush before replacement, and atomically replace the primary. If both copies are invalid, TUNRUN asks for explicit reset and renames damaged files with a `.corrupt-` suffix rather than overwriting them.

The runtime profile still does not store planned timestamps, campaign checkpoints, achievements, ghosts, full input bindings, or mode-specific records. Those require explicit migrations and tests before they are added. The profile is local progression data, not an anti-cheat boundary.

## Planned full-game save schema

The full-game profile is intended to store:
- settings and key/controller mappings;
- selected ship, unlocked ships, cosmetics, and unlock history;
- Aether Shards and Singularity Cores;
- stage and campaign progress;
- best scores, distances, times, achievements, and statistics;
- recent seed runs, generator version, ruleset, and compact replay metadata;
- optional local ghost references;
- timestamps and canonical integrity metadata.

Do not store transient pointer coordinates, active mouse capture, or stuck pressed-state flags.

## Safe-write protocol

1. Validate the in-memory profile.
2. Serialise to a temporary file in the same directory.
3. Flush file data to disk.
4. Atomically replace the primary file.
5. Keep the previous known-good profile as backup.
6. Mark save success only after the replacement succeeds.

On load, validate schema, required fields, ranges, unlock balance, and checksum/integrity metadata. Try backup recovery if the primary profile is corrupt. If both are invalid, preserve the corrupt files for diagnosis and offer a clear recovery/reset path rather than crashing.

Schema migrations must be explicit and tested. A failed save must not deduct currency or tell the player a purchase was saved.

## Save triggers

Save after changed settings, successful purchases/unlocks, checkpoint/progression updates, end of run, achievement changes, and clean shutdown. Rate-limit frequent statistics saves to avoid excessive disk writes. Gameplay should continue safely if a write temporarily fails, but show a nonintrusive warning and retry.
