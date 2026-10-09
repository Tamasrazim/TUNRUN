# UI and Interaction Specification

## Principles

- All screens work with mouse, keyboard, and gamepad.
- The active screen owns input context. Gameplay input cannot leak into menus.
- Every control has visible idle, hover, focused, pressed, disabled, and error states where applicable.
- Controller/keyboard focus is visible even when the mouse is used last.
- UI must remain readable at supported window sizes and Windows display scaling.
- UI can be operated without timing-sensitive pointer movement.

## Screen inventory

### Main Menu
Play, Hangar, Modes, Seed Lab, Records / Statistics, Controls, Settings, Credits, Exit. The menu tightens row sizing at the minimum supported window height so every action remains visible. Continue is shown only when a valid profile contains resumable progress.

### Hangar
Rotatable ship preview, name, handling stats, selected/unlocked/locked states, unlock requirements, resource balance, purchase confirmation, cosmetic preview, and equip action.

### Mode Select
The current build exposes Custom Seed Run and Practice Preview. Campaign and a separate Endless mode are explicitly marked in development; selecting either displays what is unavailable instead of silently doing nothing. Seed Challenge, Daily Run, Rival Run, and Ghost Race remain planned.

### Seed Entry
Text input with normalization, validation, copy/share function, generator version, ruleset, and a clear random-seed option. Invalid input displays an inline error; it must not crash or silently change to a different seed.

### Generation/Loading
Shows generation progress and a status message. CPU generation is asynchronous; the game must not freeze the UI while waiting. Cancel returns safely to the previous menu.

### In-Run HUD
Speed, hull, energy, current score/multiplier, nearby hazard cues, pause prompt, optional distance/time, and minimal contextual warning. All HUD elements respect UI scale and safe margins.

### Pause
Resume, Restart Same Seed, Controls, Settings, and Return to Main Menu. Restart and leaving the run both require explicit confirmation because the current score and unbanked pickups are discarded. Opening Controls or Settings keeps the run paused; Back returns to Pause.

### Controls
A dedicated input reference is reachable from the main menu and Pause. It lists keyboard steering, boost, precision, dash, camera and pause controls; relative mouse steering and sensitivity; and controller stick/trigger/button mappings. Returning from Pause keeps the run paused.

### Settings
Input mapping, mouse sensitivity/inversion/steering toggle, gamepad dead-zone/response curve, FOV, FPP/TPP camera distance, audio, graphics, motion/comfort options, UI scale, language-ready text layout, and reset-to-defaults confirmation. Mouse sensitivity has a visible logarithmic track/thumb and supports click or drag.

### Results
Run seed (copyable as a fixed-width hexadecimal value), generator version, final distance and elapsed in-run time, score, collision summary, course hash, resources earned, a precise new-personal-record notice when score or combo improves, rewards, replay/ghost save option, retry seed, new random run, and return to Hangar. Seed Lab also offers Copy Seed and confirms when the clipboard has been updated.

### Records / Statistics
The current profile screen shows best distance, career-best score, best gate combo, completed runs, crashes, Aether Shards, Singularity Cores, ships unlocked, and active ship. Best performance by mode, seed history, clean passages, ship usage, mode-specific totals, and optional local ghost records remain planned.

### Dialogs
Confirmation, insufficient currency, save error, save recovery, controller disconnected, and generation error. A dialog captures focus and blocks pointer clicks to controls behind it.

## Navigation model

Use an explicit screen stack and explicit UI focus id. Entering a child screen remembers its parent and selected control. Back restores the prior screen and focus. Pause → Settings → Back must return to Pause, not to gameplay.

Pointer hover may move focus when the user is using the mouse. A keyboard/gamepad input switches to navigation focus but must not reset the selected item. Mouse movement should not continually steal controller focus when it is within a configurable small-movement threshold.

One action event can activate at most one control. Button state transitions are edge-triggered. Modal close and underlying control activation cannot happen from a single click.

## Layout and accessibility

- Use scalable layout units or a consistent logical resolution mapped to the current window.
- Recompute hitboxes and render positions from the same layout model.
- Support windowed, borderless, and fullscreen resizing without stale click coordinates.
- Keep important controls inside safe margins.
- Text must support wrapping without clipping and use a tested minimum readable size.
- Focus indicators must not rely on colour alone.
- Reduced motion disables nonessential animation and camera shake.
- Use clear labels and avoid icon-only controls without accessible labels/tooltips.

## Input-device prompts

The last active device selects prompt style: keyboard keycaps, mouse icons, or controller glyphs. A hot-plug/disconnect event updates prompts and clears stale held actions. Face-button labels should reflect detected controller layout where available; otherwise use neutral labels such as Confirm and Back.

## Visual direction

High-speed monochrome sci-fi: deep graphite tunnel, silver/icy-white geometry, clear silhouette contrast, restrained luminous cues, minimal HUD, and readable silhouettes. Colour may be used sparingly for critical status/warnings if accessibility tests demonstrate it is needed, but the core art direction remains monochrome.

The game UI must not imitate the README SVG as a substitute for a real game interface. It can share shapes, line weights, and typography while serving gameplay readability first.

## UI acceptance criteria

- Every screen and modal is navigable with all three input families.
- Clicking every visible control activates the correct control at supported DPI/resolutions.
- UI focus survives input-device switches and nested screens.
- Pausing, settings, focus loss, and results release gameplay mouse capture.
- Save/transaction errors are visible and actionable.
- No UI element is clipped, unreachable, or operable only by mouse.
