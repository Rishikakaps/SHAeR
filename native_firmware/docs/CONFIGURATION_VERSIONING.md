# SHAeR Configuration Versioning V1

Status: Required before hardware testing.

## Rule

Primary settings live in SQLite and must always be schema-versioned. Firmware must never assume an unversioned `settings.db`.

## Tables

| Table | Purpose |
|---|---|
| `schema_meta` | Stores `schema_version` |
| `schema_migrations` | One row per applied migration |
| `settings` | Current key/value settings |
| `setting_history` | Append-only history of changed settings |
| `boot_state` | Crash recovery and safe-mode bookkeeping |

## Current Version

Current schema: `2`

| Version | Migration | Adds |
|---|---|---|
| 1 | `create_settings` | Base settings table |
| 2 | `boot_state_and_defaults` | Boot recovery, setting history, default runtime keys |

## Rollback

Before any migration, firmware creates:

```text
settings.db.rollback
```

If migration fails, firmware must:

1. Stop booting normal mode.
2. Preserve both the failed DB and rollback DB.
3. Enter Safe Mode if the settings DB can still open.
4. Show recovery diagnostics if settings cannot open.

## Future Migration Rule

Every new setting must define:

| Field | Required |
|---|---|
| Schema version | Yes |
| Default value | Yes |
| Migration from previous version | Yes |
| Rollback expectation | Yes |
| User-visible impact | Yes |
| Hardware impact | Yes if setting affects power, audio, display, or radio behavior |

