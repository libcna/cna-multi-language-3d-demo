# CNA Cross-Language 3D Demo

This repository defines one small third-person 3D game to demonstrate CNA,
the C++ reimplementation of the XNA 4.0 API, and its future language bindings.
The C++ reference is the only completed implementation.

## Current status

| Language | Status |
| --- | --- |
| C++ graphical CNA/XNA reference | **DONE** |
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

The empty language directories are reserved locations, not ports. A future
port is only complete when it implements the real game through CNA or that
language's CNA binding/ABI; a standalone numerical simulator does not qualify.

## C++ architecture

The authoritative application is
`starfield::StarfieldGame final : Microsoft::Xna::Framework::Game`.
`Initialize` resets the live game state, `LoadContent` creates the XNA
`BasicEffect` and optional capture resources, `Update(GameTime&)` reads
`Keyboard::GetState()` and owns all gameplay changes, and `Draw(GameTime)`
renders those same live player, hazard, collectible, camera, score, and state
fields.

There is no independent `Game`, renderer abstraction, snapshot transport,
scenario engine, or headless C++ executable. `starfield_game` is only a static
library packaging the actual `StarfieldGame` class for the application and its
tests; it cannot run independently. `starfield_cpp` is the CNA/XNA game.

Game-facing production code uses public XNA 4.0-style CNA APIs only. It does
not use CNAEXT, SDL, OpenGL/GLES/EGL headers, or renderer internals. EasyGL's
OpenGL33 and OpenGLES3 profiles are selected by CNA configuration without any
renderer branch in the game source.

## Game

**CNA Starfield Courier** is now a compact three-sector campaign. In each
bounded 20×20 arena, collect three gold energy cells, avoid moving hazards,
and enter the green extraction gate before that sector's 60-second timer
expires. Starport has crossing patrols, Ion Basin adds a vertical sweeper and
an orbital drone, and Solar Forge combines all three enemy motion types at
higher speed. Each sector has its own palette, layout, gate, and boundary
landmarks. The scene has a
perspective heading-relative chase camera, visible floor grid and boundaries,
procedural meshes, depth testing, and a compact HUD showing cells, time, run
state, sector, and score. Collection, loss, victory, and restart have short original
sound cues loaded through XNA `SoundEffect`. A quiet CC0 ambient space loop
plays continuously through XNA `Song` and `MediaPlayer`.

Controls:

- `W`/Up and `S`/Down move forward and backward relative to the craft heading.
- `A`/Left and `D`/Right turn.
- Space boosts movement.
- `R` or Enter restarts.
- Escape exits.

The exact shared world and behavior are in [shared/game-spec.md](shared/game-spec.md)
and the port architecture contract is in
[shared/game-contract.md](shared/game-contract.md).

## Build and run

The C++ game requires the neighboring CNA checkout. A default configure builds
CNA into the C++ game; CNA is not an optional frontend:

```sh
cmake -S . -B build/cna-game -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCNA_SOURCE_DIR=../cna \
  -DCNA_GRAPHICS_RENDERER=OPENGL33 \
  -DCNA_GRAPHICS_RENDERERS="OPENGL33;OPENGLES3"
cmake --build build/cna-game --target starfield_cpp starfield_cpp_tests
./build/cna-game/cpp/starfield_cpp
```

Run all C++ behavior and graphics checks:

```sh
ctest --test-dir build/cna-game -L cpp --output-on-failure
```

Capture and validate frames rendered by the actual game lifecycle:

```sh
SDL_VIDEODRIVER=offscreen CNA_GRAPHICS_RENDERER=OPENGL33 \
  ./build/cna-game/cpp/starfield_cpp --smoke-frames 1 \
  --validate-frame --screenshot build/cna-game/starfield-opengl33.ppm

SDL_VIDEODRIVER=offscreen CNA_GRAPHICS_RENDERER=OPENGLES3 \
  ./build/cna-game/cpp/starfield_cpp --smoke-frames 1 \
  --validate-frame --screenshot build/cna-game/starfield-opengles3.ppm
```

For renderer QA, append `--start-sector 1`, `2`, or `3` to capture a specific
environment. This still runs that sector through ordinary `Game::Run()`; it is
not a separate simulation path.

Smoke mode still calls ordinary `Game::Run()` and exits from `Draw` after the
requested number of frames. It validates real CNA rendering; it is not a
headless simulation path or a gameplay protocol.

The build copies the runtime content beside `starfield_cpp`. Audio provenance,
checksums, and license details are in
[`cpp/content/audio/LICENSE.md`](cpp/content/audio/LICENSE.md).

## License

MIT. See [LICENSE](LICENSE).
