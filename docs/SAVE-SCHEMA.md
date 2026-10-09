# Save Schema and Persistence Contract

## Location

Primary:
`%LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.json`

Backup:
`%LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.bak`

Temporary write:
`profile.json.tmp` in the same directory.

Set the Windows Hidden attribute on the directory and save files where supported. Hidden is a convenience only; this is not encryption, anti-cheat, or tamper protection. Use the current Windows user profile and do not require administrator privileges.

## Runtime schema v3

Current writes use a strict flat JSON object with exactly 18 fields: the 15 fields from the earlier profile format, plus `bestScore`, `bestCombo`, and `checksum`. Career-best score and combo are updated at run completion; the current run score remains transient until then. The parser caps input at 64 KiB, rejects duplicate or unexpected keys, validates types and ranges, rejects non-finite values, and verifies checksums against the exact source-version payload. The checksum is a 16-character lowercase hexadecimal FNV-1a digest over the canonical payload for that schema version. It detects accidental value corruption; it is **not** cryptographic authentication, encryption, or anti-cheat protection.

Schema v1 saves (15 fields without a checksum) and schema v2 saves (16 fields with a checksum) are accepted and upgraded to v3. The parser verifies v2 using its original canonical field set before adding v3-only career records. The current primary is copied to `profile.bak` before the upgraded v3 primary is atomically replaced. When recovering from an older backup, the backup is not overwritten during migration. If the migration write fails, the valid profile remains loaded in memory and migration is retried on a later save.

## Planned full-game canonical schema

The following remains a planning example. Fields may be extended through explicit schema migrations; meanings and invariants must not change silently.

```json
{
  "schemaVersion": 1,
  "gameVersion": "0.1.0-dev",
  "createdUtc": "2026-01-01T00:00:00Z",
  "updatedUtc": "2026-01-01T00:00:00Z",
  "selectedShip": "driftwing",
  "ships": {
    "unlocked": ["driftwing"],
    "equippedCosmetics": {},
    "unlockHistory": []
  },
  "wallet": {
    "aetherShards": 0,
    "singularityCores": 0,
    "lastTransactionId": 0
  },
  "progress": {
    "campaignStage": 1,
    "completedStages": [],
    "stageStars": {},
    "achievements": []
  },
  "records": {
    "endlessBestDistance": 0.0,
    "endlessBestScore": 0,
    "seedRuns": [],
    "ghostFiles": []
  },
  "settings": {
    "inputPreset": "default",
    "mouseSteering": true,
    "mouseSensitivity": 1.0,
    "invertMouseX": false,
    "invertMouseY": false,
    "gamepadDeadZone": 0.15,
    "cameraMode": "tpp",
    "fov": 80.0,
    "screenShake": 0.25,
    "reducedMotion": false,
    "masterVolume": 0.8
  },
  "integrity": {
    "format": "canonical-json-sha256",
    "digest": "computed-by-implementation"
  }
}
```

The example timestamp is illustrative, and the example digest is a placeholder—not a valid save file. The planned SHA-256 integrity object is a future full-game format and is separate from the runtime v2 FNV-1a accidental-corruption check.

## Validation rules

- Reject unknown unsupported schema versions with a recovery message rather than interpreting fields unpredictably.
- Clamp numeric settings to documented legal ranges.
- Reject negative balances, invalid ship ids, duplicate unlocks, invalid stage numbers, invalid timestamps, and impossible record values.
- Currency balances and unlock history must be transaction-consistent.
- Never trust a profile's selected ship without checking that the ship is unlocked and available in the current build.
- Do not load arbitrary file paths from ghost references; restrict them to a known user-data directory.
- Keep a recoverable copy of corrupt input for diagnostics when safe.

## Safe save transaction

1. Validate the in-memory profile and all pending changes.
2. Serialize to canonical UTF-8 JSON in a temporary file within the save directory.
3. Flush file contents and metadata as supported by the platform.
4. Preserve the last known-good primary as backup.
5. Atomically replace the primary with the temporary file using the appropriate Windows replacement primitive.
6. Apply the hidden attribute where supported.
7. Confirm success to the UI only after the replacement succeeds.

On load, validate the primary; if invalid, try the backup. If both fail, show a recovery/reset choice without silently overwriting both files.

## Economy transaction rules

A purchase should include a unique transaction id, item id, cost, previous balance, new balance, and result. The item unlock and wallet deduction become one logical profile mutation followed by one safe save. Prevent re-entrant/duplicate activation while a transaction is pending.

If a save fails, do not claim a purchase persisted. Preserve enough in-memory state to retry and show a warning. Do not blindly re-apply the same transaction after recovery.

## Save timing

Save after settings changes (debounced), purchases/unlocks, checkpoints, stage completion, achievement changes, and run results. Statistics updates may be batched. Do not write the file every frame.

## Migration

Each future schema version needs a migration function and fixture tests. The v1-to-v2 migration is implemented and fixture-tested; it retains the validated pre-migration primary as backup before atomic replacement. Future migrations must remain one-way and transactional, with an upgrade fixture for every shipped version.

## Persistence tests

See [QA and Acceptance](./QA-AND-ACCEPTANCE.md). In particular, interrupted writes, corrupted primary/backup, failed purchases, invalid fields, hidden attribute behavior, and uninstall preservation are release blockers.
