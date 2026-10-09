# Save Schema and Persistence Contract

## Location

Primary:
`%LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.json`

Backup:
`%LOCALAPPDATA%\Tamasrazim\TUNRUN\profile.bak`

Temporary write:
`profile.json.tmp` in the same directory.

Set the Windows Hidden attribute on the directory and save files where supported. Hidden is a convenience only; this is not encryption, anti-cheat, or tamper protection. Use the current Windows user profile and do not require administrator privileges.

## Runtime schema v1

The running implementation stores a strict flat JSON object with exactly 15 fields: `schemaVersion`, `showFps`, `reduceMotion`, `mouseSteering`, `fullscreen`, `mouseSensitivity`, `selectedShip`, `unlockedShips`, `aetherShards`, `singularityCores`, `totalRuns`, `totalCrashes`, `bestDistance`, `rootSeed`, and `runSerial`. The parser caps input at 64 KiB, rejects duplicate or unexpected keys, validates types/ranges, and rejects non-finite numeric values. The runtime v1 file does not yet include timestamp or checksum fields; those belong to a future explicit migration.

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

The example timestamp is illustrative, and the example digest is a placeholder—not a valid save file. Runtime profile v1 does not yet serialize timestamps or digests. Canonicalisation, hashing, versioning, and migration fixtures must be implemented and tested before integrity verification is enabled.

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

Each schema version needs a migration function and fixture tests. Migrations must be one-way and transactional: retain a backup of the pre-migration file and only replace it after the migrated profile validates. Test upgrades from every previously released schema version.

## Persistence tests

See [QA and Acceptance](./QA-AND-ACCEPTANCE.md). In particular, interrupted writes, corrupted primary/backup, failed purchases, invalid fields, hidden attribute behavior, and uninstall preservation are release blockers.
