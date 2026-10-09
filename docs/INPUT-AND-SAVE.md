# Input and Save Specification

## Input contract

Keyboard, mouse, and gamepad must work in **both gameplay and every UI screen**. Device input is translated by one Input Service into named actions. UI and flight each have a clearly active input context.

A device change updates visual prompts, but must not reset screen state, discard slider values, move selection unexpectedly, or leave a control held.

## Default controls

Mappings are defaults and must be remappable later.

| Action | Keyboard | Mouse | Gamepad |
|---|---|---|---|
| Steer horizontally/vertically | WASD or arrows | Relative movement steers while mouse-flight mode is active | Left stick |
| Boost | Left Shift | Left button (optional configurable binding) | Right trigger |
| Energy dash | Space | Right button (optional configurable binding) | A / Cross |
| Brake / precision mode | Left Ctrl | Wheel click (optional binding) | Left trigger |
| Switch FPP / TPP | V | Configurable secondary button | Y / Triangle |
| Pause | Esc | UI pause button; capture ends | Menu / Start |
| Confirm / interact | Enter / E | Left click | A / Cross |
| Back / cancel | Esc / Backspace | Right click only within a UI context | B / Circle |
| UI navigation | Arrows / Tab | Pointer and click | D-pad / left stick |
| Settings slider | Arrows / +/- | Drag/click slider | D-pad / stick with focus |

All bindings must be configurable with conflict detection, reset-to-defaults, and clear labels for the currently active device. Platform-specific face-button labels should be rendered correctly where supported.

## Mouse implementation requirements

### UI state

- The normal system cursor is visible and interactive on Main Menu, Hangar, Mode Select, Seed Entry, Settings, Pause, Results, dialogs, and exit confirmation.
- Every visible button must respond to mouse click. Sliders support click and drag; dropdowns, tabs, and confirmation dialogs must work.
- UI hit testing must use the same logical coordinate system as layout. DPI scale, fullscreen, borderless mode, resizing, and resolution changes must not create offset click zones.
- Hover styling is never the only indication of selection.

### Gameplay state

- Mouse steering uses relative motion. The input backend should use Windows Raw Input (WM_INPUT) or an equivalent native relative-motion path; it must not recenter the cursor with repeated SetCursorPos warps.
- The cursor is hidden/captured only while gameplay mouse steering is active and the game window is focused.
- Mouse capture is released before pause, menu, settings, results, focus loss, or shutdown.
- A lost-focus event clears accumulated deltas and held mouse-button states before pausing.
- Returning from pause re-enters gameplay input explicitly and restores only the requested gameplay capture state.
- Sensitivity, smoothing, horizontal inversion, vertical inversion, dead-zone/response curve, and mouse-steering enable/disable are configurable.
- If mouse steering is disabled, mouse remains available for UI and keyboard/gamepad flight is unaffected.

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

## Save schema

The versioned profile stores:
- schema version and last successful write timestamp;
- settings and key/controller mappings;
- selected ship, unlocked ships, cosmetics, and unlock history;
- Aether Shards and Singularity Cores;
- stage and campaign progress;
- best scores, distances, times, achievements, and statistics;
- recent seed runs, generator version, ruleset, and compact replay metadata;
- optional local ghost references.

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
