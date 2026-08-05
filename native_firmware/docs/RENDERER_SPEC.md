# SHAeR Renderer Spec V1

Status: V1 freeze draft.

## Purpose

RendererAPI v1 lets SHAeR draw the same UI through desktop SDL, Raspberry Pi framebuffer/SPI, and headless tests. UI logic must not depend on the rendering backend.

## Required Interface

```cpp
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void BeginFrame(const FrameInfo& frame) = 0;
    virtual void DrawText(const TextDrawCommand& command) = 0;
    virtual void DrawImage(const ImageDrawCommand& command) = 0;
    virtual void DrawAlbumArt(const AlbumArtDrawCommand& command) = 0;
    virtual void DrawVinyl(const VinylDrawCommand& command) = 0;
    virtual void DrawProgressBar(const ProgressBarDrawCommand& command) = 0;
    virtual void DrawPopup(const PopupDrawCommand& command) = 0;
    virtual void DrawTransition(const TransitionDrawCommand& command) = 0;
    virtual void Present() = 0;
};
```

Current starter code has a smaller interface. The next renderer implementation should expand toward this spec without changing application state ownership.

## Coordinate System

- Logical canvas: 240 x 320 portrait for device.
- Desktop simulator may scale integer multiples.
- All theme layout should use logical coordinates.
- Renderer handles scaling, clipping, and pixel alignment.

## FrameInfo

Required fields:

- Width.
- Height.
- Theme ID.
- Firmware state.
- Battery profile.
- Target FPS.
- Reduce motion flag.
- Timestamp.

## Text

DrawText supports:

- Font token from theme.
- Size token.
- Color token.
- Alignment.
- Max width.
- Optional marquee only where allowed by power policy.

Text must clip safely and never overlap controls.

## Images And Album Art

DrawImage supports cached theme assets and UI icons.

DrawAlbumArt supports:

- Placeholder.
- Cached cover art.
- Rounded or square mask from theme.
- Low-resolution preview while loading.

The renderer must keep layout stable when cover art appears late.

## Vinyl And Progress

DrawVinyl is a theme-friendly primitive for playback worlds that use disc, tape, meter, waveform, or archive motifs.

DrawProgressBar supports:

- Determinate progress.
- Buffer state.
- Crossfade state.
- Recording duration.

## Popups

DrawPopup must support:

- Blocking modal.
- Non-blocking toast.
- Title.
- Body.
- Primary action.
- Optional secondary action.
- Theme-defined chrome.

## Transitions

DrawTransition receives a TransitionPlan:

- From screen.
- To screen.
- Style.
- Duration.
- Blocking flag.
- Power-saving flag.
- Reason.

Renderer may simplify transition under battery saver but must preserve state meaning.

## Backends

Required V1 backends:

- ConsoleRenderer for logs/tests.
- SDLRenderer for desktop development.
- PiFramebufferRenderer or PiSpiRenderer for hardware.
- TestRenderer for navigation/render model tests.

## Performance Rules

- UI thread must never block on image decode or network.
- Album art decode is background work.
- Renderer must obey power FPS budget.
- Present should be the only place that flushes display changes.
- Dirty rectangles are preferred for SPI display.

