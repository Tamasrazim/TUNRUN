# Security and Privacy Requirements

Status: requirements for implementation; most runtime controls cannot be verified until code exists.

## Scope and trust boundaries

TUNRUN is intended to be an offline-first Windows game. Inputs are not trusted merely because they originate locally. Treat user-controlled or externally sourced content as untrusted:

- save/profile files and backup files;
- manually entered seeds and challenge codes;
- replay/ghost recordings and future imported challenges;
- archive entries, models, textures, audio, fonts and other assets;
- update manifests and downloaded packages, if an updater is ever approved;
- command-line arguments, environment variables, filenames, and paths.

The renderer, procedural generator, UI and AI should not require elevated privileges. The game must not install drivers, modify security settings, or execute downloaded content.

## Data minimization

- Do not add telemetry, advertising identifiers, analytics, or account sign-in by default.
- Do not make network requests during ordinary offline gameplay.
- Store only game settings and progression needed for operation.
- Keep save data under `%LOCALAPPDATA%\\Tamasrazim\\TUNRUN\\`; apply the Windows hidden attribute as a convenience only, never as a security boundary.
- Do not store credentials or authentication tokens in the profile.
- Diagnostics should redact usernames and full local paths by default. Any opt-in diagnostic export must explain what it contains.

## File and parser safety

- Use bounded parsing. Enforce maximum input bytes, string length, collection count, nesting depth, replay duration and asset dimensions before allocation or processing.
- Validate schema version, numeric finiteness, integer ranges, enum values, IDs, and cross-field invariants. Reject NaN/infinity and overflow-prone values.
- Write saves using a temporary file in the same directory, flush/close, then replace atomically where the platform supports it. Preserve a last-known-good backup and report recovery clearly.
- Never deserialize arbitrary C++ objects or invoke code from save/replay data.
- Normalize and canonicalize paths before file access. Reject absolute paths and traversal components where an item is expected to remain under an approved root.
- For archive extraction, reject path traversal, links/reparse points, duplicate conflicting paths and decompression bombs; bound total extracted bytes and file count.
- Keep asset reads inside approved game-data roots unless a clearly documented user-import feature is added.

## C++ and Windows implementation

- Prefer RAII, standard containers, bounds-checked parsing, and explicit ownership of resources.
- Treat compiler warnings as actionable; use supported MSVC security checks such as `/W4`, `/sdl`, and Control Flow Guard where compatible with the build.
- Enable appropriate Windows executable protections (ASLR, DEP/NX and CFG when supported); confirm actual linker flags in build output rather than assume defaults.
- Avoid unsafe string and buffer APIs, unchecked size arithmetic, raw owning pointers, and catch-all error handling that silently discards failures.
- Keep worker-thread results tagged to a run/generation ID so stale results cannot mutate a new run.
- Do not launch shell commands with user-controlled values. Avoid executing external tools at runtime.

## Networking and updates

Networking is disabled by default in the design. If an updater, online feature or downloaded content is later approved, it must receive a separate threat review before implementation:

- HTTPS is transport protection, not proof that a release is authentic.
- Verify signed release metadata and package signatures against a trusted public key embedded in the application or established through a secure release process.
- Prevent downgrade/rollback to vulnerable versions, and fail closed on invalid signatures, malformed manifests, redirects to unexpected hosts, or unexpected content.
- Never run installers or packages directly from a temporary download without verification.
- Updates must be optional, auditable, recoverable, and must not overwrite or delete saves.
- Document exactly what is sent over the network and obtain consent for any nonessential collection.

## Procedural and gameplay integrity

- Use deterministic, independent random streams for course structure, obstacle timing, rewards, and cosmetic effects.
- Do not treat a course seed as a secret. A seed provides reproducibility, not authentication.
- Validate generated sections before adding them to the live world; impose retry and memory limits.
- Economy changes are transactions: validate preconditions, commit once, then show success. UI click handling must be idempotent to prevent duplicated rewards or purchases.

## Repository and release hygiene

- Never commit tokens, passwords, private keys, signing keys, private save files, dumps or unreviewed binaries.
- Pin third-party GitHub Actions to full commit SHAs where practical; grant workflows only the permissions they require.
- Workflow tokens should be read-only unless a job explicitly requires writes.
- Do not enable an automated dependency service that opens pull requests unless the repository workflow is deliberately changed; review dependencies manually and update them with commits on `main`.
- Keep a third-party register for code, fonts, music, sound effects, models and textures. Record origin, version, license, redistribution rights and modifications.
- Publish checksums for release downloads. Sign releases when a suitable signing identity is available.
- This repository also runs a pinned Gitleaks Action against the full Git history on pushes to `main` and on a schedule. It complements, but does not replace, GitHub's native secret scanning and push-protection controls. Verify native settings separately in GitHub.
- Dependabot pull requests are intentionally not configured because this project uses direct commits to `main`; dependency updates must be reviewed and committed manually.

## Minimum security acceptance gate

Before a first playable build:
1. Malformed and oversized save/replay input tests pass.
2. Path traversal and archive extraction tests pass.
3. Corrupt-save recovery tests pass without silent progression loss.
4. No secrets or local user data are in tracked files or release packages.
5. A clean Windows build uses the documented compiler hardening flags.
6. Network/update functionality is absent unless a separate review and signed-verification implementation are complete.
