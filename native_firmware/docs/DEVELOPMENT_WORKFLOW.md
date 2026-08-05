# Development Workflow

SHAeR is developed as a Git-first embedded product. The repository is the single source of truth.

All development must follow `docs/LAYERED_MILESTONE_WORKFLOW.md`. Do not begin a later layer until the active layer has completed all five checkpoints, has passing validation, has a Git commit, and has a milestone tag.

## Daily Flow

```text
Mac
-> edit code
-> make check
-> git commit
-> push to private GitHub
-> Raspberry Pi git pull
-> make
-> sudo make install
-> sudo systemctl restart shaer
```

Source files should never be manually copied to the SD card. The Pi should always receive changes through Git and the install/update scripts.

## Branches

| Branch | Purpose |
|---|---|
| `main` | Releasable firmware and companion code |
| `develop` | Integration branch for the next version |
| `feature/<name>` | New behavior |
| `fix/<name>` | Bug fixes |
| `hardware/<name>` | Hardware bring-up and HAL changes |
| `release/<version>` | Release stabilization |

## Commits

Use Conventional Commits:

```text
feat(core): add boot timing marker
fix(update): restore settings backup after failed build
docs(power): update 18650 runtime estimate
test(nav): cover sleep wake transition
chore(repo): package release archive
```

## Code Rules

- Work on only one layer at a time.
- Read only documentation required by the active layer.
- Keep `docs/CURRENT_MILESTONE.md` accurate at checkpoint boundaries.
- Keep hardware access behind HAL interfaces.
- Keep services event-driven.
- Keep themes data-driven.
- Keep SQLite schemas versioned.
- Add tests for manager or state-machine changes.
- Update docs when behavior, wiring, storage, deployment, or recovery changes.

## Hardware Validation

Every hardware component requires an independent validation program and a breadboard test procedure before it is integrated into the normal firmware path.

Use `docs/BREADBOARD_VALIDATION_TEMPLATE.md` for physical validation procedures.

## Pi Update Flow

On the Raspberry Pi:

```bash
cd /opt/shaer
sudo ./update.sh
```

The update script backs up settings and databases, pulls from Git, builds, installs, restarts the service, and rolls back if the build or service restart fails.

## Data Survival

Persistent user data belongs in `/var/lib/shaer`. Logs belong in `/var/log/shaer`. The install script must not delete either location.
