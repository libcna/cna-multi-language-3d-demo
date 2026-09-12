# CNA Cross-Language 3D Demo

This repository specifies one small 3D game whose C++ reference is being
integrated with CNA. Its C directory is a consumer of CNA's existing C ABI,
while the other language directories contain equivalent demonstrations. It is
a demonstration project, not a production game: the shared gameplay should
remain small, readable, and easy to port.

## Project goals

- Use the CNA library, the C++ reimplementation of the XNA 4.0 API.
- Use only the XNA 4.0 API surface for game-facing code. CNA-specific graphics
  extensions (`CNA.Ext`) are deliberately out of scope.
- Use EasyGL as the primary Linux desktop renderer and validate OpenGLES with
  the same renderer-independent XNA-style game source.
- Keep every implementation visually and behaviorally equivalent.
- Demonstrate CNA's C ABI and ports to C#, Java, TypeScript, Python, Rust, Go,
  Swift, Ruby, and Common Lisp.
- License all project code under the MIT License.

## Game concept

**CNA Starfield Courier** is a compact third-person 3D game. The player pilots
a small courier drone through a bounded asteroid arena, collects three energy
cells, avoids slow-moving hazards, and reaches the extraction ring. A run has
one scene, one camera, a tiny HUD, deterministic movement, collision checks,
and a win/lose/restart loop. No networking, procedural world generation, or
content pipeline beyond the minimum demonstration assets is planned.

The reference behavior, input mapping, coordinate conventions, procedural
geometry, camera, and visual verification rules are recorded in `plan.md` and
`shared/game-spec.md`.

## Repository layout

Each language directory contains, or is being prepared to contain, the same
demo and a short build/run guide:

`cpp`, `c`, `cs`, `java`, `ts`, `python`, `rust`, `go`, `swift`, `ruby`, and
`common-lisp`.

The C++ version is the visual reference. With `STARFIELD_ENABLE_CNA=ON`,
`cpp/starfield_cna` is an XNA-shaped `Microsoft.Xna.Framework.Game` subclass
linked to CNA's `cna_runtime`; it creates a BasicEffect, perspective camera,
procedural colored 3D geometry, a compact indicator HUD, and a playable CNA
game loop with heading-relative controls. The remaining language directories
are not graphical ports yet and are tracked as such.

## Development status

The CNA-backed C++ graphical reference is implemented and runtime-verified on
Linux with EasyGL's `OPENGL33` and `OPENGLES3` profiles. Its deterministic
gameplay tests use real assertions, and its CNA tests validate expected game
states plus a 1280x720 frame containing the required arena, grid, boundary,
player, hazard, and HUD colors. No non-C++ language is claimed graphical; C
remains a clear-only CNA lifecycle/ABI smoke test.

## Binding status

The following statements are deliberately precise:

- The optional CMake integration consumes the real CNA tree from `../cna`.
  `starfield_cna` uses CNA's C++ XNA-compatible `Game`, `BasicEffect`, typed
  primitive drawing, input, perspective matrices, depth state, and render
  targets. The C program is not graphical yet; its ABI smoke test remains
  supplemental evidence only.
- The optional C# probe consumes the real `CNA.XnaCompat` project from
  `../cna-cs` and compiles an XNA-shaped `Microsoft.Xna.Framework.Vector3`.
  The headless C# gameplay runner is still independent of that binding.
- Java, TypeScript, Python, Rust, Go, Swift, Ruby, and Common Lisp currently
  have contract-compatible headless ports only. They do not yet import or
  link their corresponding CNA binding repositories.
- Therefore matching snapshots prove only gameplay-contract conformance; they
  do not prove that a binding or a renderer is being used.

The optional `STARFIELD_ENABLE_CNA` CMake configuration consumes the local CNA
tree from `../cna`, builds the real `cna_runtime` and `cna_c_api` targets, and
provides both CNA game runners plus a direct `<CNA/C/cna.h>` ABI-version smoke
test. The C# project has a separate
`UseCnaCs=true` compile probe for the local `cna-cs` `CNA.XnaCompat` facade;
the probe proves the binding compiles, while the headless gameplay remains
independent of it.

## Build and run the C++ graphical reference

The verified CNA-enabled build uses the neighboring CNA checkout:

```sh
cmake -S . -B build/cna-demo \
  -DSTARFIELD_ENABLE_CNA=ON -DCNA_SOURCE_DIR=../cna \
  -DCNA_GRAPHICS_RENDERER=OPENGL33 \
  -DCNA_GRAPHICS_RENDERERS="OPENGL33;OPENGLES3"
cmake --build build/cna-demo --target starfield_cna
./build/cna-demo/cpp/starfield_cna
```

For a finite initialization/render smoke test:

```sh
SDL_VIDEODRIVER=offscreen \
  ./build/cna-demo/cpp/starfield_cna --frames 1 --validate-frame
```

Run the assertion-backed C++ gameplay and graphical tests:

```sh
ctest --test-dir build/cna-demo -L cpp --output-on-failure
```

Capture a deterministic scenario frame (PPM keeps the runner dependency-free):

```sh
SDL_VIDEODRIVER=offscreen CNA_GRAPHICS_RENDERER=OPENGL33 \
  ./build/cna-demo/cpp/starfield_cna --scenario win \
  --validate-frame --screenshot build/cna-demo/starfield-win.ppm

SDL_VIDEODRIVER=offscreen CNA_GRAPHICS_RENDERER=OPENGLES3 \
  ./build/cna-demo/cpp/starfield_cna --scenario win \
  --validate-frame --screenshot build/cna-demo/starfield-win-gles.ppm
```

To build and run the CNA-backed C++ and C demos (requires `../cna`):

```sh
cmake -S . -B build/cna-demo -DSTARFIELD_ENABLE_CNA=ON -DCNA_SOURCE_DIR=../cna
cmake --build build/cna-demo --target starfield_cna cna_starfield_c cna_c_api_consumer_smoke
build/cna-demo/cpp/starfield_cna --frames 3
build/cna-demo/c/cna_starfield_c --frames 3
build/cna-demo/c/cna_c_api_consumer_smoke
```

To compile the C# XNA-shaped binding probe (requires `../cna-cs`):

```sh
dotnet build cs/Starfield.csproj -c Release \
  -p:UseCnaCs=true -p:CnaCsRoot="$PWD/../cna-cs"
```

The finite-frame runs verified CNA initialization, rendering, readback, and
expected scenario state on Linux. The graphical runner supports `startup`,
`collection`, `hazard`, `win`, `loss`, and `restart`; scripted mode freezes the
result for deterministic capture and does not replace normal interactive play.
A passing headless scenario is never evidence that a language binding or a
graphical renderer is in use; graphical status is maintained in `plan.md`.

## License

MIT. See `LICENSE`.
