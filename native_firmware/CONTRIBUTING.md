# Contributing To SHAeR

SHAeR is currently a private firmware project. Treat `main` as releasable and keep all work traceable through branches and pull requests.

All work must follow `docs/LAYERED_MILESTONE_WORKFLOW.md`. Do not skip layers, do not combine unfinished layers, and do not continue after a completed layer until the next layer is explicitly approved.

## Branches

| Branch | Purpose |
|---|---|
| `main` | Stable, releasable code |
| `develop` | Integration branch for upcoming firmware |
| `feature/<short-name>` | New feature work |
| `fix/<short-name>` | Bug fixes |
| `hardware/<short-name>` | Hardware bring-up changes |
| `release/<version>` | Release stabilization |

## Commit Convention

Use Conventional Commits:

```text
feat(renderer): add charging animation descriptor
fix(power): preserve settings before update
docs(os): clarify cold boot home contract
test(theme): validate theme manifests
chore(repo): update release packaging
```

## Definition Of Done

Every meaningful change should include:

- Active layer and checkpoint.
- Explicit checkpoint report using `docs/CHECKPOINT_REPORT_TEMPLATE.md`.
- Compile result, compiler version, warning count, and error count.
- Architecture impact, if any.
- Unit/integration tests.
- Runtime validation.
- Raspberry Pi compile/runtime status when applicable.
- Breadboard validation procedure when hardware-facing.
- Documentation update.
- Migration impact.
- Memory/power/performance note if relevant.
- Rollback expectation if it touches install/update/storage.
- Git commit after the checkpoint succeeds.
- Milestone tag after the layer succeeds.

Every commit must leave the repository buildable. On development machines, `make check` must pass. On Raspberry Pi OS, `make pi` must pass before Pi-facing work is claimed complete.
