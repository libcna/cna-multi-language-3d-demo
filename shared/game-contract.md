# CNA Starfield Courier — Cross-Language Acceptance Contract

This document defines when an implementation qualifies as a port of the real
Starfield Courier game.

> A language implementation is a port only if running it starts an actual CNA
> application and produces the real, playable 3D game.

Graphics are part of conformance. A standalone mathematical simulation, stdout
state machine, binding compile probe, or CNA window that only clears the
framebuffer is not a port. Matching gameplay numbers without running and
rendering the game through CNA is also insufficient.

## Authority hierarchy

1. [`game-spec.md`](game-spec.md) is the authoritative gameplay and visual
   design specification. It defines what Starfield Courier looks like and how
   it behaves.
2. The C++ CNA/XNA application under [`../cpp/`](../cpp/) is the executable
   reference against which later ports are compared.
3. `c/`, `cs/`, `java/`, `ts/`, `python/`, `rust/`, `go/`, `swift/`, `ruby/`,
   and `common-lisp/` are reserved for independent implementations through the
   appropriate CNA binding or public CNA C ABI.

The repository does not define a portable custom game engine, renderer, input
layer, snapshot transport, or game-specific ABI for all languages to
reimplement. Internal source organization may differ by language, but CNA must
remain the application and rendering foundation.

## Required playable application

Every completed port must:

- initialize the real CNA runtime;
- create a real CNA/XNA game and window;
- enter the CNA game loop or the equivalent lifecycle exposed by the public
  CNA binding or C ABI;
- receive real keyboard input through CNA/XNA;
- maintain the live player, sector, collectible, hazard, timer, score, camera,
  win/loss, and restart state used by that lifecycle;
- update player movement and heading, hazard motion, collection, collisions,
  sector progression, win, loss, and restart;
- render actual perspective 3D geometry through CNA, including every arena,
  player craft, collectible, active hazard, extraction gate, boundary marker,
  and environment landmark;
- render the HUD and terminal-state presentation;
- implement the heading-relative chase camera and all gameplay constants from
  `game-spec.md`;
- build, launch, accept input, and be playable as its normal execution mode.

Simulation and rendering must consume the same authoritative positions and
state. A hazard position calculated separately for drawing is non-conformant,
even if a screenshot appears plausible.

## CNA integration requirement

A port must use its language's real CNA-facing API. Use a native CNA language
binding where one exists. Where a language is intended to consume CNA through
the public C ABI, use that ABI through the language's appropriate FFI.

Do not invent a custom cross-language gameplay API to bypass missing binding or
ABI functionality. If the real binding or C ABI lacks a capability required to
create the window, run the lifecycle, read input, render the specified 3D game,
or present its HUD, the port remains `BLOCKED` until that capability is fixed in
the appropriate CNA or binding repository. A fallback headless simulator does
not reduce or satisfy that blocker.

## XNA 4.0 graphics boundary

Game-facing graphics architecture targets the XNA 4.0-style API exposed by
CNA. Production game code must not rely on:

- CNAEXT;
- direct OpenGL or OpenGL ES;
- Vulkan or DirectX calls;
- EasyGL or other renderer internals;
- SDL rendering;
- a custom renderer abstraction used to bypass CNA.

Renderer selection belongs to CNA and its build/runtime configuration. The
same game implementation must remain renderer-independent and be validated
with the renderer configurations required by `game-spec.md` and project
planning.

## Behavioral and visual parity

Later ports need not reproduce C++ source structure line for line. They must
match the specification and C++ reference in observable behavior and visuals,
including:

- world dimensions and sector layouts;
- player start positions, heading, movement speed, turning speed, and boost;
- collectible count, locations, appearance, and collection behavior;
- hazard count, geometry, paths, speeds, and collision dimensions;
- extraction-gate locations, locking, and progression behavior;
- perspective field of view, chase-camera offset, aspect ratio, and near/far
  planes;
- arena, player, hazard, collectible, gate, landmark, and HUD geometry/colors;
- HUD semantics, score progression, sector timing, and audio behavior;
- keyboard controls;
- win, loss, terminal-state, and full restart behavior.

## Validation contract

Graphical validation is mandatory. A port cannot be marked complete until its
normal executable has launched the real CNA application and the complete game
has been exercised. Where practical, validation must include:

- deterministic initial scenes for every sector;
- known player and camera positions;
- screenshots or framebuffer captures produced by the running port;
- comparison with reference captures, including all required renderers;
- renderer-independent state checks at deterministic gameplay checkpoints;
- checks that visible player, collectible, hazard, and gate positions equal the
  positions used by gameplay and collision;
- gameplay scenarios covering movement, turning, boost, collection, hazard
  collision, timeout, sector progression, win, loss, and restart;
- meaningful final-state assertions rather than exit-code-only tests.

Pure numerical tests are useful supplemental coverage for deterministic helper
logic. They are not the product, are not a portable conformance protocol, and
cannot replace real lifecycle, input, audio, graphical, and playability checks.
Generated or synthetic reference images that did not come from the running game
are not acceptable screenshot evidence.

## Port status vocabulary

- `EMPTY` — the language directory is reserved but contains no implementation.
- `IN PROGRESS` — real CNA integration has begun, but the graphical playable
  port or its parity validation is incomplete.
- `BLOCKED` — a verified missing CNA, binding, or ABI capability prevents the
  graphical playable port. The exact missing API and evidence must be recorded.
- `DONE` — the real graphical CNA port builds, launches, is playable, and passes
  the required behavioral and visual parity checks.

A headless simulator can never qualify as `DONE`.

## Repository status after prototype cleanup

| Language | Status |
| --- | --- |
| C++ | Current reference work; its milestone status is governed by project planning |
| C | EMPTY |
| C# | EMPTY |
| Java | EMPTY |
| TypeScript | EMPTY |
| Python | EMPTY |
| Rust | EMPTY |
| Go | EMPTY |
| Swift | EMPTY |
| Ruby | EMPTY |
| Common Lisp | EMPTY |

The empty directories are intentional placeholders. Each future port must be
started from scratch against the finished graphical C++ reference; deleted
prototype code is not a foundation to restore or refactor.
