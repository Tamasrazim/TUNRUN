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
Play, Hangar, Modes, Records, Statistics, Settings, Credits, Exit. Continue is shown only when a valid profile contains resumable progress.

### Hangar
Rotatable ship preview, name, handling stats, selected/unlocked/locked states, unlock requirements, resource balance, purchase confirmation, cosmetic preview, and equip action.

### Mode Select
Campaign, Endless, Seed Challenge, Daily Run, Practice, Rival Run, and Ghost Race. Disabled or unavailable modes explain why.

### Seed Entry
Text input with normalization, validation, copy/share function, generator version, ruleset, and a clear random-seed option. Invalid input displays an inline error; it must not crash or silently change to a different seed.

### Generation/Loading
Shows generation progress and a status message. CPU generation is asynchronous; the game must not freeze the UI while waiting. Cancel returns safely to the previous menu.

### In-Run HUD
Speed, hull, energy, current score/multiplier, nearby hazard cues, pause prompt, optional distance/time, and minimal contextual warning. All HUD elements respect UI scale and safe margins.

### Pause
Resume, Restart Run (with confirmation where progress is lost), Settings, Controls, Return to Hangar, and Quit. Opening settings keeps the run paused; closing settings returns to Pause.

### Settings
Input mapping, mouse sensitivity/inversion/steering toggle, gamepad dead-zone/response curve, FOV, FPP/TPP camera distance, audio, graphics, motion/comfort options, UI scale, language-ready text layout, and reset-to-defaults confirmation.

### Results
Run seed, generator version, distance/time, score, hull/collision summary, resources earned, records improved, rewards, replay/ghost save option, retry seed, new random run, and return to Hangar.

### Records / Statistics
Best performance by mode, seed history, longest distance, clean passages, ship usage, resource totals, and optional local ghost records.

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
