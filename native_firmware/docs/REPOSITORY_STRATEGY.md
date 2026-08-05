# Repository Strategy

SHAeR uses one private Git repository as the source of truth for firmware, companion app, simulator, themes, docs, deployment scripts, and release packaging.

## Layout

```text
README.md        project entry point
LICENSE          private project license
VERSION          current semantic version
CHANGELOG.md     human-readable release history
docs/            architecture, hardware, workflow, recovery
firmware/        C++ आदि Vasi OS runtime
ui/              shared UI package boundary
themes/          theme package boundary
services/        Python/background service code
database/        schema and migration notes
sync_app/        future sync-specific package boundary
simulator/       browser visual simulator
assets/          theme packs and bundled assets
scripts/         Pi setup, install, update, package scripts
tests/           C++ and Python tests
tools/           maintenance tooling notes
build/           generated build output, not committed
releases/        generated release archives, not committed
```

## Versioning

SHAeR follows Semantic Versioning:

```text
MAJOR.MINOR.PATCH[-prerelease]
```

Examples:

```text
0.1.0-alpha.1
0.1.0-beta.1
1.0.0
1.0.1
2.0.0
```

Firmware tags should use:

```text
v0.1.0-alpha.1
```

## Release Branches

Release candidates stabilize on `release/<version>`. Only bug fixes, docs, migration fixes, packaging fixes, and verification changes should land on a release branch.

## Changelog

`CHANGELOG.md` is updated for every release. `scripts/generate_changelog.py` can draft entries from recent Conventional Commits, but the final changelog should be reviewed by a human.

## GitHub

The GitHub repository should be private until the hardware and license strategy are intentionally changed.

Recommended description:

```text
आदि Vasi OS and companion software for SHAeR, a retro-futuristic personal audio archive.
```
