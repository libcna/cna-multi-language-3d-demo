# CNA Starfield Courier — Game Architecture Contract

This contract defines what counts as an implementation of the shared game.
It deliberately does not define a headless serialization or stdout protocol.

## Canonical architecture

Every port must be a CNA/XNA application in its target language. For C++, the
authoritative type is:

```cpp
class StarfieldGame final : public Microsoft::Xna::Framework::Game
```

The equivalent type in a future binding must participate in that binding's
real CNA/XNA lifecycle. The required flow is:

```text
Game::Run()
  Initialize() / LoadContent()
  Update(GameTime) -> CNA keyboard -> live gameplay state
  Draw(GameTime)   -> that same live gameplay state
```

The game class may own small domain types such as `Player`, `Hazard`, and
`Collectible`, and may delegate focused calculations to ordinary helpers. It
must remain the lifecycle and ownership boundary. These are not acceptable:

- an independent generic game engine under the CNA game;
- a separately runnable authoritative headless simulator;
- custom input or snapshot transport between a simulator and CNA;
- a custom renderer abstraction used instead of XNA graphics APIs;
- precomputed visual positions that differ from collision positions.

## Lifecycle ownership

- Construction configures the XNA `GraphicsDeviceManager`, 1280×720 back
  buffer, fixed timestep, and window.
- `Initialize` establishes the initial run state and camera.
- `LoadContent` creates graphics/content resources through XNA-style APIs.
- `Update(GameTime)` calls `Keyboard::GetState`, handles quit/restart, advances
  player movement and heading, hazards, collectibles, collision, score, timer,
  win/loss, and camera.
- `Draw(GameTime)` applies the perspective camera, depth state, effect, world
  matrices, and HUD projection to render the fields owned by that game.
- The executable enters ordinary `Game::Run()`. Finite graphical tests may
  request exit after a number of real drawn frames, but may not replace the
  lifecycle with a simulator loop.

## Shared state and behavior

The exact constants, positions, colors, camera, controls, update order,
collision radii, scoring, and terminal rules are normative in
[`game-spec.md`](game-spec.md). Later ports may express their domain data
differently, but their visible state transitions must match those rules.

There is no canonical serialized `Snapshot`, scenario name set, stable stdout
field list, or six-field compatibility prefix. Test diagnostics are local to a
port and are not the game architecture.

## API boundary

Game-facing production code is limited to the XNA 4.0-style surface exposed by
CNA or an official CNA binding. Suitable concepts include `Game`, `GameTime`,
`GraphicsDeviceManager`, `Keyboard`, `Keys`, `Vector3`, `Matrix`,
`GraphicsDevice`, `BasicEffect`, XNA vertex types, depth/rasterizer states,
render targets, and sprite/content APIs.

The game must not include or call CNAEXT, SDL, OpenGL, OpenGL ES, EGL, Vulkan,
DirectX, EasyGL, or other renderer internals. Renderer selection belongs to CNA
build/runtime configuration. The unchanged game must run with EasyGL/OpenGL33
and EasyGL/OpenGLES3.

## Testing contract

Tests must fail when gameplay results are wrong, not only when a process exits
nonzero. Unit access may call the same private gameplay step owned and used by
the real game class; it must not reproduce that step in a test-only engine.
At least one integration test must run the actual graphical executable through
`Game::Run()` and inspect a real rendered frame. Screenshot comparisons must
use pixels read from the actual game render, never generated reference data.

A port is complete only after its real window, input, simulation, rendering,
terminal states, restart, and required renderer configurations have been
validated. Numeric agreement without a CNA game is insufficient.
