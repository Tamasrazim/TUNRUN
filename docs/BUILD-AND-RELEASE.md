# Build, Test, and Release Plan

## Current status

TUNRUN is an offline native C++20/raylib prototype, not a finished commercial release. Implemented systems include seeded curved 3D tunnel geometry, four gate families, moving mine hazards (generator v5, interleaved 27 units from neighboring gate planes), Aether Shard/Singularity Core pickups, keyboard/gamepad translation plus independent ship rotation, a UI-only mouse pointer, eight ship profiles, local profile schema v3 with backup/recovery, score/combo records, Seed Lab validation, and automated regression tests.

The Windows workflow performs a clean x64 Release build and unit tests. It then creates a portable development ZIP with the executable, license notices, current build metadata, and a SHA-256 file manifest. This is a development distribution—not a signed installer or a promise of release readiness.

## Toolchain

- Windows x64 with MSVC; the local `windows-dev` preset targets Visual Studio 2022.
- CMake 3.21 or newer and C++20.
- raylib 5.5 from the immutable commit pinned in `CMakeLists.txt`.
- The CI workflow runs on GitHub-hosted Windows and checks configuration, compile, tests, integrity, and secret scanning.
- Inno Setup remains a later milestone for a signed Windows installer.

## Build locally

From a Windows shell with Visual Studio 2022 Build Tools and CMake installed:

```powershell
cmake --preset windows-dev
cmake --build --preset windows-dev
ctest --preset windows-dev
./scripts/package-windows.ps1 -BuildDirectory build/windows-dev/Release -OutputDirectory build/package -Version "0.1.0-dev" -CommitSha "local-build"
```

The package script validates its paths, requires the executable and license/docs, includes optional `assets/` and `resources/` directories when present, and emits `TUNRUN-Windows-x64-Portable.zip`, `build-info.json`, and a SHA-256 package manifest. CI uploads the staged package directory, so the Actions artifact opens directly to the portable contents instead of nesting one ZIP inside another. The default CI build directory is `build/Release`.

## Distribution size and assets

The portable package now bakes an original 12-sheet UI material library at 4096 × 2048 pixels, 24-bit RGB per sheet. The sheets cover brushed metal, carbon weave, ceramic armour, circuit traces, etched surfaces, honeycomb, and other screen-specific surfaces. They are used behind the live 3D front-end, command buttons, hangar hologram, telemetry panels, dialogs, and the in-flight HUD. Each screen streams one material sheet at a time; changing screens releases the previous GPU texture, so the roughly 288 MiB of source material pixels is package content, not a permanent VRAM requirement.

The maps are generated deterministically by `scripts/build-ui-materials.py` from original procedural patterns. The build records each sheet's resolution and SHA-256 in `assets/ui/catalog.json`; the overall package manifest hashes all shipped files. No third-party imagery is bundled. A source checkout can still launch without baked maps and falls back to vector panels, while the official portable package includes the complete material library.

**The size target must come from content the game really loads.** Do not add zero-filled archives, duplicate textures, unused libraries, or fake dependencies merely to inflate the download. Future size growth should come from original or properly licensed ship/tunnel models, materials, animation, sound effects, music, and other implemented game content.

## CI and branch policy

All repository changes are pushed directly to `main`; no branch or PR workflow is used. Every push must configure/build/test the code and run repository-integrity and secret-scanning checks. If a workflow fails, fix it with a follow-up commit on `main`; do not bypass failed results or claim tests passed without checking them.

## Security and release requirements

- Never commit tokens, passwords, signing keys, user saves, dumps, or machine-specific paths.
- Treat seeds, saves, replays, archives, and imported files as hostile input; enforce size/range limits and prevent path traversal.
- Keep ordinary gameplay offline. Updater or online functionality needs separate threat review, signed metadata/package verification, and safe rollback.
- Track dependency source/version and redistribution rights. raylib's license is recorded in `THIRD-PARTY-NOTICES.md`.
- Publish SHA-256 checksums; sign release artifacts once a trusted signing identity is available.
- Do not label the development ZIP a finished game or installer.

## Remaining release gates

- Complete real-device input, fullscreen/focus, graphics, performance, and accessibility testing.
- Add original or properly licensed production-quality models, materials/textures, sound effects, and music; integrate them into the actual game rather than bundling unused files.
- Finish game modes, progression/campaign work, replay/ghost support, options, and onboarding.
- Test save migration/recovery across clean Windows installations and interrupted writes.
- Build and launch the portable package outside the development environment; later build/test installer, uninstall, and upgrade flows.
- Check version strings and artifact hashes; create signed/versioned release notes and source archives.
