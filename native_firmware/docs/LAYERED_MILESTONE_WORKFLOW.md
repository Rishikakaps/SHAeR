# Layered Milestone Workflow

This document is mandatory for all SHAeR development. The repository is developed in strictly incremental layers. A later layer must not begin until the current layer is complete, validated, documented, committed, and tagged.

Reliability matters more than speed. The goal is hardware-ready firmware and software that can move from breadboard to perfboard with confidence.

The active implementation tracks and milestone order are defined in `docs/DEVELOPMENT_TRACKS.md`. The first runnable prototype targets the 2.4" IPS TFT SPI display, EC11 navigation, SD-card MP3 browsing, PCM5102A playback, volume control, and safe shutdown.

## Development Order

| Layer | Name |
|---:|---|
| 0 | Foundation |
| 1 | Hardware Abstraction Layer |
| 2 | Core Services |
| 3 | Navigation Engine |
| 4 | UI Framework |
| 5 | Theme Engine |
| 6 | Individual Themes |
| 7 | Music Library |
| 8 | Audio Playback |
| 9 | Spotify |
| 10 | Voice Recording |
| 11 | Notification System |
| 12 | Companion App |
| 13 | Secret Features |
| 14 | Performance Optimisation |
| 15 | Breadboard Validation |
| 16 | Perfboard Validation |
| 17 | Final Enclosure Validation |

Do not skip layers. Do not merge unfinished work into later layers. Do not read or modify future-layer documents unless the current layer explicitly depends on them.

## Required Checkpoints Per Layer

Every layer has five checkpoints.

### Checkpoint 1: Architecture

- Read only documents required for the current layer.
- Produce an implementation plan.
- Explain dependencies.
- Explain expected outputs.
- Wait for approval if architecture changes are required.

### Checkpoint 2: Implementation

- Write production code.
- Keep modules independent.
- Use clean interfaces.
- Avoid duplicated logic.
- Compile continuously.

### Checkpoint 3: Software Validation

Run the checks appropriate to the layer:

- Compiler.
- Static analysis where available.
- Unit tests.
- Integration tests.
- Runtime validation.
- Memory leak checks where applicable.

No failing tests are permitted.

### Checkpoint 4: Breadboard Hardware Validation

Produce a physical test procedure before permanent assembly. Include:

- Required components.
- GPIO pins.
- Expected voltage.
- Expected behavior.
- Expected display output.
- Expected encoder response.
- Expected audio output.
- Failure modes.
- How to diagnose failures.

Software compilation is not proof that hardware works.

### Checkpoint 5: Completion

Only after all checks succeed:

- Commit to Git.
- Create a milestone tag.
- Update project documentation.
- Summarize completed work.
- List remaining risks.
- Propose the next layer.

Never continue automatically after a completed layer.

## Hardware Validation Programs

Every hardware component must have its own independent validation program before it can be integrated into normal firmware.

Required examples:

- Display test.
- Encoder test.
- DAC test.
- Amplifier test.
- Battery monitor test.
- Charging test.
- Button test.
- Storage test.
- Spotify network test.
- Wi-Fi reconnect test.
- Power loss recovery test.

Each program must be executable independently from the repository root or from a clearly documented hardware validation command.

## UI Rule

Do not redesign UI during implementation. Provided design assets are the source of truth. If implementation constraints require a design change, explain the constraint before changing the design.

## Testing Rule

No feature is complete until it has passed:

- Compilation.
- Unit tests.
- Integration tests.
- Runtime tests.
- Breadboard validation.
- Recovery testing.
- Long-duration testing when applicable.

Every checkpoint report must explicitly state the result of each applicable validation area:

```text
Compile
Build: PASS / FAIL
Compiler: gcc/g++/clang version
Warnings: count
Errors: count

Unit Tests
Result: PASS / FAIL
Passed: n/n

Runtime Test
Desktop runtime launch: PASS / FAIL
Desktop runtime exit: PASS / FAIL

Raspberry Pi Test
Deploy to Pi: PASS / FAIL / NOT APPLICABLE
Compiled on Pi: PASS / FAIL / NOT APPLICABLE
Executed on Pi: PASS / FAIL / NOT APPLICABLE

Git
Commit: hash
Tag: tag name / NOT APPLICABLE
```

If a validation area is not applicable to the checkpoint, the report must say why.

## Git Rule

- Commit after every successful checkpoint.
- Tag every completed layer.
- Never lose a working state.
- Every commit must leave the repository buildable.
- Checking out any commit on the correct target platform and running `make` must succeed.
- If a commit cannot satisfy `make` on the current platform because it targets Raspberry Pi-only headers, `make check` must still pass on development machines and `make pi` must pass on Raspberry Pi OS before the checkpoint is claimed.
- If interrupted, resume from the most recent successful commit.
- Never regenerate completed work.

Layer tags use this format:

```text
layer-00-foundation
layer-01-hal
layer-02-core-services
layer-03-navigation
layer-04-ui-framework
layer-05-theme-engine
layer-06-individual-themes
layer-07-music-library
layer-08-audio-playback
layer-09-spotify
layer-10-voice-recording
layer-11-notifications
layer-12-companion-app
layer-13-secret-features
layer-14-performance
layer-15-breadboard-validation
layer-16-perfboard-validation
layer-17-final-enclosure-validation
```

Checkpoint commits should use Conventional Commits, for example:

```text
docs(workflow): define layer gate
feat(hal): add display interface
test(hal): add encoder breadboard validation
```

Checkpoint tags use this format:

```text
layer-00-checkpoint-01-architecture
layer-00-checkpoint-02-implementation
layer-00-checkpoint-03-software-validation
layer-00-checkpoint-04-breadboard-validation
layer-00-checkpoint-05-completion
```

## Failure Recovery

If work is interrupted:

1. Reconnect.
2. Inspect Git status.
3. Resume from the most recent successful commit.
4. Do not restart the project.
5. Do not regenerate completed work.

If a layer has known bugs, stop. Fix the current layer before moving forward.
