# SHAeR Design Language V1

Status: V1 freeze draft.

## Design Fidelity Rule

The supplied Figma designs are the source of truth.

- Do not reinterpret layouts.
- Do not improve spacing unless the physical TFT makes the original spacing unreadable.
- Do not replace icons when a supplied icon exists.
- Do not invent animations.
- If an implementation detail is unclear, infer the smallest possible behavior that preserves the appearance of the design.
- Visual fidelity is more important than creative interpretation.

## Emotional Contract

Every screen should feel like a beautifully engineered object waking up, not an app shouting for attention.

| Moment | Emotion |
|---|---|
| Boot | Anticipation and curiosity |
| Home | Discovery and composure |
| Now Playing | Calm immersion |
| Local Library | Archive warmth |
| Memory Mode | Intimacy and personal history |
| Charging | Warmth and craftsmanship |
| Settings | Nostalgia and control |
| Recovery | Capable and reassuring |
| Errors | Playful, never alarming |
| Shutdown | Slow goodbye |

## Typography

- Use small, legible text designed for 240 x 320.
- Never use viewport-scaled type.
- Each theme may use its own font family, but must provide fallbacks.
- Minimum readable body size on device: 10 px equivalent.
- Avoid dense paragraphs on device. Use short rows and status phrases.

## Spacing

- Controls must be finger/encoder readable even when the visual language is compact.
- Keep repeated list rows stable in height.
- No nested decorative cards.
- Avoid layout shift during album art loading, network recovery, and charging transitions.

## Color

- Themes define their own palette through ThemeAPI v1.
- Battery saver may dim or reduce animation, but it must not turn every theme into the same design.
- Indian Raga V1 palette is deep indigo, lavender, and burnt sienna. Do not use the earlier red/yellow treatment.

## Shadows And Depth

- Use restrained depth.
- Retro UI themes may use bevels or panel shadows where historically appropriate.
- Soft themes may use painterly layering.
- Operational recovery screens stay flatter and clearer.

## Animation Philosophy

- Motion exists to explain state, not to decorate every frame.
- Preferred rich target: 24 to 30 fps depending on theme.
- Battery saver target: 12 fps equivalent, fewer animated elements.
- Critical battery: minimal motion.
- Playback visualizers must never threaten audio stability.

## Transitions

- Every theme supplies transition style data.
- Navigation transitions should be short and tactile.
- Modal recovery transitions may block input briefly.
- Power state transitions are slower and more ceremonial.
- Shutdown always uses the slow goodbye language.

## Popup Style

- Popup copy is short and human.
- Errors describe what happened and what SHAeR will do next.
- Blocking popups require OK.
- Recovery popups should offer a safe path, such as Local Library.

Example Spotify drop:

```text
Signal slipped
Your local archive is ready.
OK
```

## Loading Philosophy

- Loading states must show progress or a living status within 500 ms.
- Spotify/network loading never blocks Home.
- Cover art may appear late with a stable placeholder.
- Boot must reach Home in 6 to 8 seconds.

## Error Philosophy

- Never blame the user.
- Never show raw stack traces outside recovery/log export.
- Use plain words first, diagnostic code second.
- Every error must map to a recovery action or log entry.

## Battery Saver Philosophy

Battery saver is a posture, not a punishment. It keeps SHAeR beautiful while doing less:

- Lower display brightness.
- Cap frame rate.
- Reduce animated elements.
- Prefer local cached data.
- Delay non-urgent sync.
- Keep audio stable before visuals.

## Theme Worlds

Each theme must have unique layout, chrome, motion, popup, loading, AOD, and charging treatment. A color swap is not a theme.

Required V1 theme worlds:

- Archive Dark.
- Bombay Ticket.
- Indian Raga.
- Windows XP.
- Japanese Punk.
- Ghibli Garden.
