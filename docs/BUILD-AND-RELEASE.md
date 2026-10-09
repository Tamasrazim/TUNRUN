# Build, Test, and Release Plan

Status: the initial native application shell is implemented in source. The Windows workflow is responsible for verifying that the current commit configures, compiles and passes unit tests. Flight, procedural generation, saves, packaging and the installer remain unimplemented.

## Toolchain
- Windows x64 and Visual Studio 2022 / MSVC.
- CMake 3.21 or newer.
- C++20.
- raylib 5.5, fetched from the pinned commit in `CMakeLists.txt`.
- Inno Setup only when the installer milestone begins.

## Build locally

From a Windows shell with Visual Studio 2022 Build Tools and CMake installed:

```powershell
cmake --preset windows-dev
cmake --build --preset windows-dev
ctest --preset windows-dev
```

The GitHub Actions workflow performs a clean x64 Release configuration/build/test on every push to `main`. A green CI run is required before calling the build verified.

## CI and branch policy

There are no pull-request workflow triggers. The repository workflow is direct-to-`main` only. Every push must configure/build/test the code and run repository integrity and secret-scanning checks. If a workflow fails, fix it with a follow-up commit on `main`; do not bypass the failed result or claim tests passed without checking them.

As code grows, CI should add formatting/static analysis, unit tests for save validation and economy transactions, procedural seed regression tests, malformed-input tests, asset path checks, and Windows package checks. Any write permission given to a workflow must be separately justified; current build/test workflows use read-only repository permissions.

## Security gates

- Never commit tokens, passwords, signing keys, user save files, dumps or machine-specific paths.
- Treat seeds, saves, replays, archives and imported files as hostile input; enforce size/range limits and prevent path traversal.
- Keep ordinary gameplay offline. Any updater or online functionality requires separate threat review, signed metadata/package verification and a safe rollback path.
- Track dependency versions, source, licenses and redistribution rights. raylib's license is recorded in `THIRD-PARTY-NOTICES.md`.
- Use compiler/linker hardening compatible with MSVC; confirm the actual clean-build output before stating that hardening is active.
- Publish release checksums and sign release artifacts when a trusted signing identity is available.

## Planned release artifacts
- `TUNRUN-Setup.exe` — Windows installer.
- `TUNRUN-Portable.zip` — portable package.
- Source archive / tagged release.
- SHA-256 checksums and versioned release notes.

## Release checklist
- Clean Windows x64 build/test.
- Procedural seed regression suite passes.
- Mouse/keyboard/gamepad matrix passes.
- Save corruption/recovery/migration fixtures pass.
- Required assets and license notices are included.
- Installer and portable package launch outside the development environment.
- Play, pause, resume, quit, restart, save, load, update, and uninstall tested.
- Version strings and artifact checksums match the published release.
