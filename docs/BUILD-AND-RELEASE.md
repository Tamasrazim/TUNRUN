# Build, Test, and Release Plan

Status: planned; these commands are targets until the source tree and toolchain are implemented and verified.

## Proposed development environment

- Windows x64.
- C++20 compiler supplied by a supported Visual Studio Build Tools / Visual Studio installation.
- CMake 3.21 or newer.
- Git.
- raylib 5.5, fetched/configured through the project build system.
- Inno Setup for the Windows installer when the installer stage is reached.

Exact minimum tool versions must be checked in CI and listed in the release notes when the first build is validated.

## Intended source layout

```text
TUNRUN/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── NOTICE.md
├── docs/
├── src/
│   ├── app/
│   ├── input/
│   ├── flight/
│   ├── camera/
│   ├── procedural/
│   ├── collision/
│   ├── obstacles/
│   ├── ai/
│   ├── economy/
│   ├── save/
│   ├── ui/
│   └── audio/
├── assets/
│   ├── models/
│   ├── materials/
│   ├── audio/
│   └── fonts/
├── tests/
│   ├── unit/
│   ├── procedural/
│   ├── input/
│   └── save/
├── tools/
│   └── seed-inspector/
└── packaging/
    ├── windows/
    └── installer/
```

This is the target layout, not a claim that these source files exist yet. The implementation may adjust names while keeping responsibilities clear.

## Local build targets

Once CMake is created, the intended usage is:

```powershell
cmake --preset windows-dev
cmake --build --preset windows-dev
ctest --preset windows-dev
```

A release preset should produce a distributable configuration without absolute machine-specific paths. Do not document a command as verified until it has passed on a clean Windows environment.

## Continuous integration gates

Every push to `main` must run the repository integrity workflow. The workflow currently validates local Markdown links and required policy files; it does not claim to compile a game that has not been implemented. When source code lands, extend the `main`-only CI to run:
1. Configure and compile with warnings enabled and security-relevant compiler checks.
2. Formatting and static analysis.
3. Unit tests for input mapping, save validation/migration, economy transactions, and deterministic random streams.
4. Procedural regression seeds and boundary cases.
5. Headless section/seed batch validator where possible.
6. Artifact, asset-path, dependency and license audits.
7. Windows packaging checks after packaging exists.

No workflow is triggered by pull requests. Direct pushes to `main`, manual runs, and scheduled maintenance are the intended workflow. Give write access only to trusted maintainers; direct-to-main is not a reason to skip tests or review.

Long-running random-seed soak tests can run on a scheduled workflow or a release-candidate workflow, while deterministic regression seeds run for every change.

## Security gates before implementation and release

- Never commit access tokens, passwords, private keys, signing keys, personal save files, crash dumps, or machine-specific paths.
- Keep the runtime offline by default. Any later network/update feature requires an explicit threat review, HTTPS, authenticated metadata, signature verification, rollback protection, and a safe failure path before it ships.
- Treat saves, seed strings, replay/ghost files, imported content, and archive entries as untrusted input. Bound sizes and counts, validate schemas and numeric ranges, and reject path traversal.
- Build from pinned/reviewed dependencies and maintain a dependency/asset license inventory.
- Build release executables with supported compiler mitigations; sign release artifacts when a signing identity is established, and publish cryptographic checksums.
- Run automated security analysis when source code exists. Do not mark scanning as active merely because it is documented; inspect the workflow results and repository security settings.

## Release artifacts

Planned outputs:
- `TUNRUN-Setup.exe` — Windows installer.
- `TUNRUN-Portable.zip` — portable package.
- Source archive / tag.
- Checksums for distributed files.
- Versioned release notes describing fixes and known limitations.

The installer should support upgrade and uninstall without silently deleting saves. The portable build uses the same per-user save location unless an explicitly documented portable-save option is implemented.

## Release checklist

- Clean configure/build/test.
- Procedural seed regression suite passes.
- Mouse/keyboard/gamepad matrix passes.
- Save corruption/recovery and migration fixtures pass.
- No runtime dependence on the repository checkout or developer tools.
- All required assets and licenses/notices are included.
- Installer and portable package launch on a clean Windows machine.
- Play, pause, resume, quit, restart, save, load, update, and uninstall tested.
- Version strings and artifact checksums match the published release.
- Known issues are documented; release is not called complete if a critical input or save defect remains.
