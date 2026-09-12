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
procedural colored 3D geometry, and a playable CNA game loop. The remaining
language directories are not graphical ports yet and are tracked as such.

## Development status

The repository contains the initial CNA-backed C++ graphical reference and
prototype code awaiting replacement. Only a build and finite-frame EasyGL
smoke test has been verified so far; no non-C++ language is claimed graphical.

## Binding status

The following statements are deliberately precise:

- The optional CMake integration consumes the real CNA tree from `../cna`.
  `starfield_cna` uses CNA's C++ XNA-compatible `Game`, `BasicEffect`, typed
  primitive drawing, input, and `GraphicsDevice::Clear`. The C program is not
  graphical yet; its ABI smoke test remains supplemental evidence only.
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
cmake -S . -B build/cna-demo -DSTARFIELD_ENABLE_CNA=ON -DCNA_ROOT=../cna
cmake --build build/cna-demo --target starfield_cna
./build/cna-demo/cpp/starfield_cna
```

For a finite initialization/render smoke test:

```sh
./build/cna-demo/cpp/starfield_cna --frames 1
```

To build and run the CNA-backed C++ and C demos (requires `../cna`):

```sh
cmake -S . -B build/cna-demo -DSTARFIELD_ENABLE_CNA=ON -DCNA_ROOT=../cna
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

The finite-frame run verified CNA initialization and the EasyGL renderer on
Linux. A passing headless scenario is never evidence that a language binding
or graphical renderer is in use; graphical status is maintained in `plan.md`.

## License

MIT. See `LICENSE`.